#include <Arduino.h>
#include <SoftwareSerial.h>

/* =========================
   BLUETOOTH CONFIGURATION
   ========================= */
#define BT_RX 3   // HC-05 TX
#define BT_TX 2   // HC-05 RX (use voltage divider)

SoftwareSerial mySerial(BT_RX, BT_TX);

/* =========================
   L298N MOTOR DRIVER PINS
   ========================= */
// Motor A (Left motor)
#define ENA 5
#define IN1 8
#define IN2 9

// Motor B (Right motor)
#define ENB 6
#define IN3 10
#define IN4 11

/* =========================
   MOTOR STATE
   ========================= */
uint8_t motorSpeed = 70;   // default speed
bool motorsStopped = true;

/* =========================
   SETUP
   ========================= */
void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  Serial.println("=================================");
  Serial.println(" BLUETOOTH CONTROLLED RC CAR ");
  Serial.println("=================================");

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors();

  Serial.print("[INIT] Default PWM: ");
  Serial.println(motorSpeed);

  Serial.println("[READY] Waiting for Bluetooth commands...");
  Serial.println("---------------------------------");
}

/* =========================
   MAIN LOOP
   ========================= */
void loop() {

  if (mySerial.available()) {
    char cmd = mySerial.read();

    Serial.print("[BT] Received: ");
    Serial.println(cmd);

    /* ---------- SPEED CONTROL ---------- */
    if (cmd >= '0' && cmd <= '9') {
      // Map 0–9 → 0–255
      motorSpeed = map(cmd - '0', 0, 9, 0, 255);

      Serial.print("[PWM] Speed set to: ");
      Serial.println(motorSpeed);

      // Apply immediately if motors are running
      analogWrite(ENA, motorSpeed);
      analogWrite(ENB, motorSpeed);
      return;
    }

    /* ---------- MOVEMENT COMMANDS ---------- */
    switch (cmd) {

      case 'F':
        Serial.println("[ACTION] MOVE FORWARD");
        moveForward();
        break;

      case 'B':
        Serial.println("[ACTION] MOVE BACKWARD");
        moveBackward();
        break;

      case 'R':
      case 'L':
        Serial.println("[ACTION] STOP MOTORS");
        stopMotors();
        break;

      case 'S':
        // Button released / confirm → ignore
        Serial.println("[INFO] Command terminator (S)");
        break;

      default:
        Serial.println("[WARNING] Unknown command");
        break;
    }
  }
}

/* =========================
   MOTOR CONTROL FUNCTIONS
   ========================= */

void moveForward() {
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  motorsStopped = false;

  Serial.print("[MOTOR] Forward | PWM = ");
  Serial.println(motorSpeed);
}

void moveBackward() {
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  motorsStopped = false;

  Serial.print("[MOTOR] Backward | PWM = ");
  Serial.println(motorSpeed);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  motorsStopped = true;

  Serial.println("[MOTOR] STOPPED");
}

