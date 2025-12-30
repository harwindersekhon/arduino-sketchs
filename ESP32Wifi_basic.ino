#include <ESP8266WiFi.h>

// ====== USER CONFIG ======
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
// =========================

#define LED_PIN D1   // GPIO5, external LED (ACTIVE HIGH)

unsigned long previousMillis = 0;
const long blinkInterval = 500; // ms
bool ledState = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);

  // LED ON initially (WiFi NOT connected)
  digitalWrite(LED_PIN, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Blink LED when connected
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }
  } else {
    // Keep LED solid ON when NOT connected
    digitalWrite(LED_PIN, HIGH);
  }
}

