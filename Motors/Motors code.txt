#include <Arduino.h>
#include <IRremote.hpp>

/* =========================
   IR RECEIVER CONFIGURATION
   ========================= */
#define IR_PIN 2     // IR receiver output pin

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
   MOTOR SPEED CONTROL
   ========================= */
// PWM value (0–255)
// 70 / 255 ≈ 27% duty cycle
// ~2–2.5V effective from ~7.4–8.4V supply
#define MOTOR_SPEED 70

/* =========================
   IR COMMAND DEFINITIONS
   ========================= */
#define CMD_FORWARD  0x16
#define CMD_BACKWARD 0x0C
#define CMD_STOP     0x45

/* =========================
   STATE / SAFETY VARIABLES
   ========================= */
unsigned long lastCommandTime = 0;
const unsigned long FAILSAFE_TIMEOUT = 0; // 0 = disabled
uint8_t lastCommand = 0;
bool motorsStopped = true;

/* =========================
   SETUP
   ========================= */
void setup() {
  Serial.begin(9600);
  delay(200);

  Serial.println("=================================");
  Serial.println(" IR CONTROLLED RC CAR STARTING ");
  Serial.println("=================================");

  Serial.println("[INIT] Initializing IR receiver...");
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  Serial.println("[INIT] Configuring motor control pins...");
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors();

  Serial.print("[INIT] Motor PWM limit set to: ");
  Serial.print(MOTOR_SPEED);
  Serial.println(" / 255");

  if (FAILSAFE_TIMEOUT == 0) {
    Serial.println("[INIT] FAILSAFE DISABLED (Latch Mode)");
  } else {
    Serial.print("[INIT] FAILSAFE ENABLED, timeout = ");
    Serial.print(FAILSAFE_TIMEOUT);
    Serial.println(" ms");
  }

  Serial.println("[READY] System ready. Waiting for IR commands...");
  Serial.println("---------------------------------");
}

/* =========================
   MAIN LOOP
   ========================= */
void loop() {

  /* ---------- IR DECODE ---------- */
  if (IrReceiver.decode()) {

    uint8_t cmd = IrReceiver.decodedIRData.command;

    Serial.print("[IR] Command received: 0x");
    Serial.println(cmd, HEX);

    lastCommandTime = millis();

    if (cmd == lastCommand) {
      Serial.println("[IR] Same as last command (repeat frame). Ignored.");
    } else {
      Serial.println("[IR] New command detected. Processing...");

      switch (cmd) {

        case CMD_FORWARD:
          Serial.println("[ACTION] MOVE FORWARD");
          moveForward();
          break;

        case CMD_BACKWARD:
          Serial.println("[ACTION] MOVE BACKWARD");
          moveBackward();
          break;

        case CMD_STOP:
          Serial.println("[ACTION] STOP MOTORS");
          stopMotors();
          break;

        default:
          Serial.println("[WARNING] Unknown command. Ignored.");
          break;
      }

      lastCommand = cmd;
    }

    IrReceiver.resume();
  }

  /* ---------- FAILSAFE CHECK ---------- */
  if (FAILSAFE_TIMEOUT > 0) {
    if (millis() - lastCommandTime > FAILSAFE_TIMEOUT) {
      Serial.println("[FAILSAFE] Timeout exceeded. Stopping motors.");
      stopMotors();
      lastCommand = 0;
    }
  }
}

/* =========================
   MOTOR CONTROL FUNCTIONS
   ========================= */

void moveForward() {
  Serial.print("[MOTOR] PWM applied: ");
  Serial.println(MOTOR_SPEED);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  motorsStopped = false;
}

void moveBackward() {
  Serial.print("[MOTOR] PWM applied: ");
  Serial.println(MOTOR_SPEED);

  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  motorsStopped = false;
}

void stopMotors() {
  if (!motorsStopped) {
    Serial.println("[MOTOR] Motors STOPPED");
  }

  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  motorsStopped = true;
}

