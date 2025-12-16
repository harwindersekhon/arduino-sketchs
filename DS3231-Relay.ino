#include <Wire.h>
#include <RTClib.h>

#define RELAY_PIN 8

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

RTC_DS3231 rtc;

void setup() {
  Wire.begin();
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  if (!rtc.begin()) {
    Serial.println("RTC NOT FOUND");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("System Started");
}

void loop() {
  DateTime now = rtc.now();

  int h = now.hour();
  int m = now.minute();
  int s = now.second();

  // Determine relay window: 11:00:00 to 11:04:59
  bool relayShouldBeOn = (h == 11 && m < 5);

  digitalWrite(RELAY_PIN, relayShouldBeOn ? RELAY_ON : RELAY_OFF);

  // Calculate seconds since midnight
  long nowSec = h * 3600L + m * 60L + s;
  long onStart = 11 * 3600L;          // 11:00:00
  long offTime = 11 * 3600L + 5 * 60L; // 11:05:00

  long delta;

  if (relayShouldBeOn) {
    delta = offTime - nowSec;
    Serial.print("Time: ");
    printTime(h, m, s);
    Serial.print(" | Relay: ON  | Turning OFF in: ");
  } else {
    if (nowSec < onStart) {
      delta = onStart - nowSec;
    } else {
      delta = (24L * 3600L - nowSec) + onStart;
    }
    Serial.print("Time: ");
    printTime(h, m, s);
    Serial.print(" | Relay: OFF | Turning ON in: ");
  }

  printDuration(delta);
  Serial.println();

  delay(1000);
}

void printTime(int h, int m, int s) {
  print2(h);
  Serial.print(":");
  print2(m);
  Serial.print(":");
  print2(s);
}

void printDuration(long sec) {
  long hh = sec / 3600;
  long mm = (sec % 3600) / 60;
  long ss = sec % 60;

  print2(hh);
  Serial.print(":");
  print2(mm);
  Serial.print(":");
  print2(ss);
}

void print2(int v) {
  if (v < 10) Serial.print("0");
  Serial.print(v);
}

