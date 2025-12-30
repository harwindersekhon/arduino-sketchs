#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

// ========= USER CONFIG =========
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
// ===============================

// ---- Pins ----
#define LED_PIN D1        // GPIO5, external LED (ACTIVE HIGH)
#define DHT_PIN D2        // GPIO4
#define DHT_TYPE DHT11

// ---- Objects ----
ESP8266WebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);

// ---- LED Blink Control ----
unsigned long previousMillis = 0;
const long blinkInterval = 500;
bool ledState = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);

  // LED ON initially (Wi-Fi NOT connected)
  digitalWrite(LED_PIN, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  dht.begin();

  // Web server route
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
  handleWifiLed();
}

// ================= FUNCTIONS =================

void handleWifiLed() {
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }
  } else {
    digitalWrite(LED_PIN, HIGH); // Solid ON if not connected
  }
}

void handleRoot() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Celsius

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>ESP8266 DHT11</title>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;background:#f4f4f4;}";
  html += "h1{color:#333;}";
  html += ".box{background:#fff;padding:20px;margin:40px auto;width:300px;border-radius:8px;}";
  html += "</style></head><body>";

  html += "<div class='box'>";
  html += "<h1>DHT11 Sensor</h1>";

  if (isnan(humidity) || isnan(temperature)) {
    html += "<p>Sensor read failed</p>";
  } else {
    html += "<p><strong>Temperature:</strong> ";
    html += String(temperature);
    html += " &deg;C</p>";

    html += "<p><strong>Humidity:</strong> ";
    html += String(humidity);
    html += " %</p>";
  }

  html += "<p><strong>Wi-Fi:</strong> Connected</p>";
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}
