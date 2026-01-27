#include <Arduino.h>
#include <IRremote.hpp>

/* =========================
   IR RECEIVER CONFIG
   ========================= */
#define IR_PIN 2

/* =========================
   MOTOR DRIVER (L298N)
   ========================= */
#define ENA 5
#define IN1 8
#define IN2 9

#define ENB 6
#define IN3 10
#define IN4 11

#define MAX_SPEED 150
#define MIN_SPEED 50

/* =========================
   HC-SR04
   ========================= */
#define TRIG_PIN 4
#define ECHO_PIN 3

/* =========================
   DISTANCE ZONES (cm)
   ========================= */
#define STOP_DISTANCE_CM      30
#define SLOWDOWN_DISTANCE_CM  60
#define REQUIRED_CONFIRMATIONS 3

/* =========================
   IR COMMANDS
   ========================= */
#define CMD_FORWARD  0x16
#define CMD_BACKWARD 0x0C
#define CMD_STOP     0x45

/* =========================
   MOTION STATE
   ========================= */
enum MotionState { STOPPED, FORWARD, BACKWARD };
MotionState currentMotion = STOPPED;

/* =========================
   STATE VARIABLES
   ========================= */
bool obstacleDetected = false;
int distanceCm = -1;
int stopConfirmCount = 0;
int slowConfirmCount = 0;

unsigned long lastDistanceCheck = 0;
const unsigned long DISTANCE_CHECK_INTERVAL = 120;

/* =========================
   SETUP
   ========================= */
void setup() {
  Serial.begin(9600);
  delay(200);

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  stopMotors();
  Serial.println("[READY] System initialized");
}

/* =========================
   MAIN LOOP
   ========================= */
void loop() {

  /* ---------- DISTANCE CHECK (FORWARD ONLY) ---------- */
  if (millis() - lastDistanceCheck >= DISTANCE_CHECK_INTERVAL) {
    lastDistanceCheck = millis();
    distanceCm = measureDistance();

    if (currentMotion == FORWARD && distanceCm > 0) {

      // ---- STOP ZONE ----
      if (distanceCm <= STOP_DISTANCE_CM) {
        stopConfirmCount++;
        slowConfirmCount = 0;

        if (stopConfirmCount >= REQUIRED_CONFIRMATIONS) {
          Serial.println("[SAFETY] FORWARD obstacle confirmed → STOP");
          stopMotors();
          obstacleDetected = true;
        }
      }

      // ---- SLOWDOWN ZONE ----
      else if (distanceCm <= SLOWDOWN_DISTANCE_CM) {
        slowConfirmCount++;
        stopConfirmCount = 0;

        if (slowConfirmCount >= REQUIRED_CONFIRMATIONS) {
          int pwm = map(distanceCm,
                        STOP_DISTANCE_CM,
                        SLOWDOWN_DISTANCE_CM,
                        MIN_SPEED,
                        MAX_SPEED);

          pwm = constrain(pwm, MIN_SPEED, MAX_SPEED);
          analogWrite(ENA, pwm);
          analogWrite(ENB, pwm);

          Serial.print("[SLOWDOWN] PWM = ");
          Serial.println(pwm);
        }
      }

      // ---- CLEAR ZONE ----
      else {
        stopConfirmCount = 0;
        slowConfirmCount = 0;
        obstacleDetected = false;
      }
    }
  }

  /* ---------- IR CONTROL ---------- */
  if (IrReceiver.decode()) {

    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
      IrReceiver.resume();
      return;
    }

    uint8_t cmd = IrReceiver.decodedIRData.command;
    IrReceiver.resume();

    Serial.print("[IR] Command: 0x");
    Serial.println(cmd, HEX);

    switch (cmd) {

      case CMD_FORWARD:
        if (obstacleDetected) {
          Serial.println("[BLOCKED] Obstacle ahead → forward blocked");
          stopMotors();
        } else {
          moveForward();
        }
        break;

      case CMD_BACKWARD:
        moveBackward();
        break;

      case CMD_STOP:
        stopMotors();
        break;
    }
  }
}

/* =========================
   HC-SR04 MEASUREMENT
   ========================= */
int measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 18000);
  if (duration == 0) return -1;

  return duration / 58;
}

/* =========================
   MOTOR CONTROL
   ========================= */
void moveForward() {
  Serial.println("[MOTOR] FORWARD");

  analogWrite(ENA, MAX_SPEED);
  analogWrite(ENB, MAX_SPEED);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  currentMotion = FORWARD;
}

void moveBackward() {
  Serial.println("[MOTOR] BACKWARD");

  analogWrite(ENA, MAX_SPEED);
  analogWrite(ENB, MAX_SPEED);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  currentMotion = BACKWARD;
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  currentMotion = STOPPED;
  stopConfirmCount = 0;
  slowConfirmCount = 0;
  obstacleDetected = false;

  Serial.println("[MOTOR] STOPPED");
}

