#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <Dabble.h>
#include <Servo.h>

/* ========= HC-10 ========= */
#define BT_RX 2
#define BT_TX 3

/* ========= L298N ========= */
#define ENA 5
#define IN1 8
#define IN2 9

#define ENB 6
#define IN3 10
#define IN4 11

/* ========= SERVO ========= */
#define SERVO_PIN 4

/* ========= TUNING ========= */
#define MOTOR_SPEED 150

#define STEER_CENTER 90
#define STEER_MIN    50     // full LEFT limit (physical)
#define STEER_MAX    140    // full RIGHT limit (physical)

#define STEER_STEP   1      // SMALLEST practical step (1°)
#define LOOP_DELAY   20     // ms

Servo steering;
int currentSteer = STEER_CENTER;

/* ================= SETUP ================= */
void setup() {
  Serial.begin(9600);
  Dabble.begin(9600, BT_RX, BT_TX);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  steering.attach(SERVO_PIN);
  steering.write(currentSteer);

  stopMotors();
}

/* ================= LOOP ================= */
void loop() {
  Dabble.processInput();

  /* -------- MICRO STEERING (FLIPPED) -------- */
  if (GamePad.isLeftPressed()) {
    // LEFT button -> steer RIGHT
    currentSteer += STEER_STEP;
  }

  if (GamePad.isRightPressed()) {
    // RIGHT button -> steer LEFT
    currentSteer -= STEER_STEP;
  }

  // Reset steering
  if (GamePad.isCrossPressed()) {
    currentSteer = STEER_CENTER;
  }

  // Clamp to safe limits
  currentSteer = constrain(currentSteer, STEER_MIN, STEER_MAX);
  steering.write(currentSteer);

  /* -------- THROTTLE -------- */
  if (GamePad.isUpPressed()) {
    moveForward();
  }
  else if (GamePad.isDownPressed()) {
    moveBackward();
  }
  else {
    stopMotors();
  }

  delay(LOOP_DELAY);
}

/* ================= MOTOR FUNCTIONS ================= */

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

