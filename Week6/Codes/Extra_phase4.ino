/*
Display video stream from an ESP32-CAM on a web page,
with dual-axis servo panning (Horizontal on GPIO 13, Vertical on GPIO 14).
*/
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// Replace with your network credentials
const char* ssid = "Hhhh";
const char* password = "haikalhaziq";

#define PART_BOUNDARY "123456789000000000000987654321"

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;
int stream_port_number;

// Servo configurations
const int servo_h_pin = 13; // Horizontal pan
const int servo_v_pin = 14; // Vertical tilt

const int servo_min_position = 3700;
const int servo_max_position = 7000;
const int servo_speed = 150;

int servo_h_position = servo_min_position;
bool servo_h_direction = true;

int servo_v_position = servo_min_position;
bool servo_v_direction = true;

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if (fb->width > 400) {
        if (fb->format != PIXFORMAT_JPEG) {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted) {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) break;

    // --------- Dual-Axis Servo Movement Handling -----------
    // Horizontal sweep
    ledcWrite(servo_h_pin, servo_h_position);
    if (servo_h_direction) {
      servo_h_position += servo_speed;
      if (servo_h_position > servo_max_position) servo_h_direction = false;
    } else {
      servo_h_position -= servo_speed;
      if (servo_h_position < servo_min_position) servo_h_direction = true;
    }

    // Vertical sweep (moving at half speed for offset pattern)
    ledcWrite(servo_v_pin, servo_v_position);
    if (servo_v_direction) {
      servo_v_position += (servo_speed / 2);
      if (servo_v_position > servo_max_position) servo_v_direction = false;
    } else {
      servo_v_position -= (servo_speed / 2);
      if (servo_v_position < servo_min_position) servo_v_direction = true;
    }
  }
  return res;
}

static esp_err_t web_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  char webPage[200] = "";
  strcat(webPage, "<!doctype html><html><head><title>ESP32-CAM Pan-Tilt</title></head><body><img id='stream' src='http://");
  
  char address[20] = "";
  sprintf(address, "%d.%d.%d.%d:%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], stream_port_number);
  strcat(webPage, address);
  strcat(webPage, "/stream'></body></html>");
  
  return httpd_resp_send(req, (const char *)webPage, strlen(webPage));
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_uri_t index_uri = {.uri = "/", .method = HTTP_GET, .handler = web_handler, .user_ctx = NULL};
  httpd_uri_t stream_uri = {.uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL};
  
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
  }
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
  stream_port_number = config.server_port;
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  // Initialize PWM for both servos on Core v3.x syntax (50Hz, 16-bit)
  ledcAttach(servo_h_pin, 50, 16);
  ledcAttach(servo_v_pin, 50, 16);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  
  startCameraServer();
  Serial.print("Camera ready. Access it at: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(10000);
}