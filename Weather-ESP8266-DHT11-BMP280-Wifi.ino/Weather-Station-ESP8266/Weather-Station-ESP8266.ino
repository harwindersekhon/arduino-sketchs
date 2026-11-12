#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi credentials live in arduino_secrets.h (gitignored).
// First time: copy arduino_secrets.h.example to arduino_secrets.h and fill it in.
#include "arduino_secrets.h"

/* ---------- WIFI ---------- */
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
// Transmit power in dBm (0 - 20.5). Lower = less power. Raise it if the signal is weak.
const float WIFI_TX_DBM = 14.0;

/* ---------- SERVER ---------- */
// "http://" prefix is required for HTTPClient
const char* serverURL = "http://192.168.0.128/insert_data.php";
const char* apiKey = "secret_esp8266_key";

/* ---------- PINS ---------- */
#define DHTPIN D4
#define DHTTYPE DHT11
// I2C: SDA = D2, SCL = D1 (shared by BMP280 and OLED)

/* ---------- SCREEN (SSD1306 128x32) ---------- */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDR 0x3C

/* ---------- TIMING ---------- */
const unsigned long PAGE_INTERVAL_MS   = 5000UL;    // swap screen page every 5 s
const unsigned long SENSOR_INTERVAL_MS = 60000UL;   // refresh readings every 60 s
const unsigned long UPLOAD_INTERVAL_MS = 300000UL;  // send to server every 5 min

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BMP280 bmp;
DHT dht(DHTPIN, DHTTYPE);

/* ---------- STATE ---------- */
float dhtTemp = NAN;
float humidity = NAN;
float bmpTemp = NAN;
float pressure = NAN;
bool haveDht = false;     // at least one valid DHT reading
bool haveBmp = false;     // at least one valid BMP reading

int lastHttpCode = 0;     // 0 = nothing uploaded yet
bool uploadedOnce = false;
unsigned long lastUpload = 0;
unsigned long lastSensorRead = 0;

bool displayOk = false;
enum Page : uint8_t { PAGE_WEATHER, PAGE_NETWORK, PAGE_COUNT };
uint8_t page = PAGE_NETWORK;  // show WiFi status first at boot

void showMessage(const char* l1, const char* l2 = nullptr, const char* l3 = nullptr);

/* ---------- SETUP ---------- */
void setup() {
  Serial.begin(115200);
  Serial.println();

  Wire.begin(D2, D1);

  displayOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (displayOk) {
    display.dim(true);            // lowest contrast: lit pixels are the OLED's main cost
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);   // keep the 4-row layout fixed
  } else {
    Serial.println(F("SSD1306 init failed"));
  }
  showMessage("Booting...");

  if (!bmp.begin(0x76)) {
    Serial.println(F("BMP280 error: Check wiring or I2C address (try 0x77)"));
    showMessage("BMP280 error", "Check wiring / 0x77", "Press RESET");
    Serial.flush();
    ESP.deepSleep(0);  // halt at uA instead of spinning; OLED keeps the message
  }
  // Forced mode: the sensor sleeps between measurements
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,
                  Adafruit_BMP280::SAMPLING_X1,   // temperature
                  Adafruit_BMP280::SAMPLING_X1,   // pressure
                  Adafruit_BMP280::FILTER_OFF,
                  Adafruit_BMP280::STANDBY_MS_1);

  dht.begin();

  setupWifi();
  showMessage("Connecting WiFi...");
}

/* ---------- LOOP ---------- */
void loop() {
  unsigned long start = millis();
  bool connected = (WiFi.status() == WL_CONNECTED);

  // Upload as soon as possible after connecting, then every UPLOAD_INTERVAL_MS
  bool uploadDue = connected && (!uploadedOnce || start - lastUpload >= UPLOAD_INTERVAL_MS);

  // Read sensors on schedule, right before an upload, or every tick until the first valid reading
  if (uploadDue || !haveDht || !haveBmp || start - lastSensorRead >= SENSOR_INTERVAL_MS) {
    readSensors();
    lastSensorRead = start;
  }

  if (uploadDue && haveDht && haveBmp) {
    sendDataToServer();
    lastUpload = start;
    uploadedOnce = true;
  }

  drawPage(page);
  page = (page + 1) % PAGE_COUNT;

  // Idle until the next page swap. With WiFi light sleep enabled,
  // delay() lets the CPU and radio sleep between AP beacons.
  unsigned long spent = millis() - start;
  delay(spent < PAGE_INTERVAL_MS ? PAGE_INTERVAL_MS - spent : 0);
}

/* ---------- WIFI ---------- */
void setupWifi() {
  WiFi.persistent(false);                  // don't rewrite flash on every boot
  WiFi.mode(WIFI_STA);                     // station only, soft-AP off
  // Light sleep between DTIM beacons. If WiFi keeps dropping with your router,
  // change this to WIFI_MODEM_SLEEP.
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP, 3);
  WiFi.setOutputPower(WIFI_TX_DBM);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);              // non-blocking; loop() keeps running
  Serial.println(F("Connecting WiFi..."));
}

/* ---------- SENSORS ---------- */
void readSensors() {
  bmp.takeForcedMeasurement();
  float t = bmp.readTemperature();
  float p = bmp.readPressure();
  if (!isnan(t) && !isnan(p)) {
    bmpTemp = t;
    pressure = p / 100.0;
    haveBmp = true;
  } else {
    Serial.println(F("BMP280 read error"));
  }

  float dt = dht.readTemperature();
  float h = dht.readHumidity();
  // Keep the last good values if the DHT read fails
  if (!isnan(dt) && !isnan(h)) {
    dhtTemp = dt;
    humidity = h;
    haveDht = true;
  } else {
    Serial.println(F("DHT read error"));
  }
}

/* ---------- SERVER ---------- */
void sendDataToServer() {
  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, serverURL)) {
    lastHttpCode = -1;
    Serial.println(F("HTTP begin failed"));
    return;
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData =
    "api_key=" + String(apiKey) +
    "&dht_temp=" + String(dhtTemp) +
    "&humidity=" + String(humidity) +
    "&bmp_temp=" + String(bmpTemp) +
    "&pressure=" + String(pressure);

  lastHttpCode = http.POST(postData);

  Serial.print(F("HTTP Response: "));
  Serial.println(lastHttpCode);

  http.end();
}

/* ---------- DISPLAY ---------- */
void drawPage(uint8_t p) {
  if (!displayOk) return;
  display.clearDisplay();
  display.setCursor(0, 0);
  if (p == PAGE_WEATHER) {
    drawWeatherPage();
  } else {
    drawNetworkPage();
  }
  display.display();
}

void drawWeatherPage() {
  display.println(F("WEATHER"));

  display.print(F("Temp: "));
  printValue(dhtTemp, haveDht);
  display.println(F(" C"));

  display.print(F("Hum : "));
  printValue(humidity, haveDht);
  display.println(F(" %"));

  display.print(F("Pres: "));
  printValue(pressure, haveBmp);
  display.println(F(" hPa"));
}

void drawNetworkPage() {
  if (WiFi.status() == WL_CONNECTED) {
    display.println(F("WiFi: CONNECTED"));
    display.print(F("SSID: "));
    display.println(WiFi.SSID().substring(0, 15));  // 21 chars per row
    display.print(F("IP: "));
    display.println(WiFi.localIP().toString());
    display.print(F("RSSI "));
    display.print(WiFi.RSSI());
    display.print(F("dBm Up:"));
  } else {
    display.println(F("WiFi: DISCONNECTED"));
    display.println(F("Connecting..."));
    display.println(F("IP: --"));
    display.print(F("Last up: "));
  }
  printHttpCode();
}

void printValue(float v, bool valid) {
  if (valid) {
    display.print(v, 1);
  } else {
    display.print(F("--"));
  }
}

void printHttpCode() {
  if (lastHttpCode == 0) {
    display.print(F("--"));
  } else {
    display.print(lastHttpCode);
  }
}

// Up to 3 lines, used during boot and for fatal errors
void showMessage(const char* l1, const char* l2, const char* l3) {
  if (!displayOk) return;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(l1);
  if (l2) display.println(l2);
  if (l3) display.println(l3);
  display.display();
}
