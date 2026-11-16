#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>

// ==========================================
// WIFI SETTINGS
// ==========================================

const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASS = "WIFI_PASSWORD";

// ==========================================
// WEB SERVER
// ==========================================

WebServer server(80);

// ==========================================
// BUILT-IN FLASH LED SETTINGS
// ==========================================

const int FLASH_LED = 4;

// LED stays on for 300 milliseconds
const unsigned long LED_DURATION = 300;

unsigned long ledStartTime = 0;
bool ledActive = false;

// ==========================================
// CAMERA RESOLUTIONS
// ==========================================

static auto loRes =
    esp32cam::Resolution::find(320, 240);

static auto midRes =
    esp32cam::Resolution::find(350, 530);

static auto hiRes =
    esp32cam::Resolution::find(800, 600);

// ==========================================
// CAMERA IMAGE FUNCTION
// ==========================================

void serveJpg()
{
    auto frame = esp32cam::capture();

    if (frame == nullptr) {
        Serial.println("CAPTURE FAIL");
        server.send(503, "", "");
        return;
    }

    Serial.printf(
        "CAPTURE OK %dx%d %db\n",
        frame->getWidth(),
        frame->getHeight(),
        static_cast<int>(frame->size())
    );

    server.setContentLength(frame->size());
    server.send(200, "image/jpeg");

    WiFiClient client = server.client();
    frame->writeTo(client);
}

// ==========================================
// CAMERA RESOLUTION HANDLERS
// ==========================================

void handleJpgLo()
{
    if (!esp32cam::Camera.changeResolution(loRes)) {
        Serial.println("SET-LO-RES FAIL");
    }

    serveJpg();
}

void handleJpgHi()
{
    if (!esp32cam::Camera.changeResolution(hiRes)) {
        Serial.println("SET-HI-RES FAIL");
    }

    serveJpg();
}

void handleJpgMid()
{
    if (!esp32cam::Camera.changeResolution(midRes)) {
        Serial.println("SET-MID-RES FAIL");
    }

    serveJpg();
}

// ==========================================
// PERSON DETECTION LED HANDLER
// ==========================================

void handlePersonDetected()
{
    Serial.println("PERSON DETECTED BY PYTHON!");

    // Turn ON built-in flash LED
    digitalWrite(FLASH_LED, HIGH);

    ledStartTime = millis();
    ledActive = true;

    Serial.println("FLASH LED ON");

    server.send(
        200,
        "text/plain",
        "Person detection received"
    );
}

// ==========================================
// SETUP
// ==========================================

void setup()
{
    Serial.begin(115200);
    Serial.println();

    // Configure built-in flash LED
    pinMode(FLASH_LED, OUTPUT);
    digitalWrite(FLASH_LED, LOW);

    // Configure camera
    {
        using namespace esp32cam;

        Config cfg;

        cfg.setPins(pins::AiThinker);
        cfg.setResolution(hiRes);
        cfg.setBufferCount(2);
        cfg.setJpeg(80);

        bool ok = Camera.begin(cfg);

        Serial.println(
            ok ? "CAMERA OK" : "CAMERA FAIL"
        );
    }

    // Connect WiFi
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi Connected!");

    Serial.print("Camera IP: http://");
    Serial.println(WiFi.localIP());

    // ==================================
    // CAMERA ROUTES
    // ==================================

    server.on(
        "/cam-lo.jpg",
        handleJpgLo
    );

    server.on(
        "/cam-hi.jpg",
        handleJpgHi
    );

    server.on(
        "/cam-mid.jpg",
        handleJpgMid
    );

    // ==================================
    // PERSON DETECTION ROUTE
    // ==================================

    server.on(
        "/person-detected",
        HTTP_POST,
        handlePersonDetected
    );

    server.begin();

    Serial.println("ESP32-CAM Server Started");
    Serial.println("Waiting for person detection...");
}

// ==========================================
// MAIN LOOP
// ==========================================

void loop()
{
    // Process incoming HTTP requests
    server.handleClient();

    // Automatically turn OFF LED
    if (ledActive) {

        if (millis() - ledStartTime >= LED_DURATION) {

            digitalWrite(FLASH_LED, LOW);

            ledActive = false;

            Serial.println("FLASH LED OFF");
        }
    }
}
