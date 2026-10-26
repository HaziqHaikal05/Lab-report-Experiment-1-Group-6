#include <Wire.h>
#include <Servo.h>
#include <math.h>
// RFID + MPU6050 + Servo Door Lock
// An authorized RFID card is required first.
// Then, a deliberate movement of the MPU6050 unlocks the servo.
const byte MPU_ADDR = 0x68;
const byte SERVO_PIN = 9;
const byte GREEN_LED_PIN = 4;
const byte RED_LED_PIN = 3;

const int LOCKED_ANGLE = 0;
const int UNLOCKED_ANGLE = 90;

const unsigned long GESTURE_TIMEOUT_MS = 12000;
const unsigned long UNLOCK_DURATION_MS = 4000;
const unsigned long DENIED_DURATION_MS = 2000;

// Increase these thresholds to require stronger movement.
const float GYRO_THRESHOLD_DPS = 90.0;
const float ACCEL_THRESHOLD_G = 0.40;

Servo doorServo;

enum AccessState {
  IDLE,
  WAITING_FOR_MOVEMENT,
  UNLOCKED,
  DENIED
};

AccessState state = IDLE;
unsigned long stateStarted = 0;

float baselineAccel = 1.0;

bool writeMPU(byte reg, byte value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readMPU(float &ax, float &ay, float &az,
             float &gx, float &gy, float &gz) {

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(MPU_ADDR, (byte)14) != 14) {
    return false;
  }

  int16_t rawAX = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t rawAY = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t rawAZ = (int16_t)((Wire.read() << 8) | Wire.read());

  Wire.read();
  Wire.read();  // Ignore temperature

  int16_t rawGX = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t rawGY = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t rawGZ = (int16_t)((Wire.read() << 8) | Wire.read());

  // Convert accelerometer readings to g.
  ax = rawAX / 16384.0;
  ay = rawAY / 16384.0;
  az = rawAZ / 16384.0;

  // Convert gyroscope readings to degrees per second.
  gx = rawGX / 131.0;
  gy = rawGY / 131.0;
  gz = rawGZ / 131.0;

  return true;
}

void denyAccess(const char *message) {
  state = DENIED;
  stateStarted = millis();

  doorServo.write(LOCKED_ANGLE);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, HIGH);

  Serial.println(message);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  doorServo.attach(SERVO_PIN);
  doorServo.write(LOCKED_ANGLE);

  // Wake up MPU6050 and set gyroscope range to +/-250 degrees/second.
  if (!writeMPU(0x6B, 0x00) ||
      !writeMPU(0x1B, 0x00)) {

    Serial.println("ERROR: Check MPU6050 wiring!");

    while (true) {
      digitalWrite(RED_LED_PIN, HIGH);
      delay(300);
      digitalWrite(RED_LED_PIN, LOW);
      delay(300);
    }
  }

  delay(500);

  Serial.println("READY: Scan your RFID card.");
}

void loop() {

  // Receive commands from the laptop Python program.
  while (Serial.available() > 0) {
    char command = Serial.read();

    if (command == 'A') {

      float ax, ay, az, gx, gy, gz;

      if (readMPU(ax, ay, az, gx, gy, gz)) {
        baselineAccel = sqrt(ax * ax + ay * ay + az * az);
      }

      state = WAITING_FOR_MOVEMENT;
      stateStarted = millis();

      doorServo.write(LOCKED_ANGLE);
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);

      Serial.println("CARD OK: Move the MPU6050 firmly to unlock.");

    } else if (command == 'D') {

      denyAccess("ACCESS DENIED: Unauthorized RFID card.");
    }
  }

  unsigned long now = millis();

  if (state == WAITING_FOR_MOVEMENT) {

    if (now - stateStarted >= GESTURE_TIMEOUT_MS) {

      denyAccess("ACCESS DENIED: No strong movement detected.");

    } else {

      float ax, ay, az, gx, gy, gz;

      if (readMPU(ax, ay, az, gx, gy, gz)) {

        float accelMagnitude =
            sqrt(ax * ax + ay * ay + az * az);

        float gyroMagnitude =
            sqrt(gx * gx + gy * gy + gz * gz);

        bool strongRotation =
            gyroMagnitude > GYRO_THRESHOLD_DPS;

        bool strongAcceleration =
            fabs(accelMagnitude - baselineAccel)
            > ACCEL_THRESHOLD_G;

        // Unlock only when movement exceeds either threshold.
        if (strongRotation || strongAcceleration) {

          state = UNLOCKED;
          stateStarted = millis();

          doorServo.write(UNLOCKED_ANGLE);

          digitalWrite(GREEN_LED_PIN, HIGH);
          digitalWrite(RED_LED_PIN, LOW);

          Serial.println(
              "ACCESS GRANTED: Strong movement detected!"
          );
        }
      }

      delay(20);
    }
  }

  else if (state == UNLOCKED) {

    if (now - stateStarted >= UNLOCK_DURATION_MS) {

      doorServo.write(LOCKED_ANGLE);
      digitalWrite(GREEN_LED_PIN, LOW);

      state = IDLE;

      Serial.println("DOOR LOCKED: Ready for next card.");
    }
  }

  else if (state == DENIED) {

    if (now - stateStarted >= DENIED_DURATION_MS) {

      digitalWrite(RED_LED_PIN, LOW);
      state = IDLE;
    }
  }
}