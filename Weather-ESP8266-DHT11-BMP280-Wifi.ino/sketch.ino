#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

/* ---------- WIFI ---------- */
const char* ssid = "FILL"  // ----------------> change
const char* password = "FILL"

/* ---------- SERVER ---------- */
const char* serverURL = "192.168.1.128/insert_data.php"; // --------- change
const char* apiKey = "secret_esp8266_key";

/* ---------- PINS ---------- */
#define DHTPIN D4
#define DHTTYPE DHT11

Adafruit_BMP280 bmp;
DHT dht(DHTPIN, DHTTYPE);

/* ---------- SETUP ---------- */
void setup() {
  Serial.begin(9600);
  delay(1000);

  Wire.begin(D2, D1);

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 error");
    while (1);
  }

  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
}

/* ---------- LOOP ---------- */
void loop() {
  if (WiFi.status() == WL_CONNECTED) {

    float dhtTemp = dht.readTemperature();
    float humidity = dht.readHumidity();
    float bmpTemp = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0;

    if (isnan(dhtTemp) || isnan(humidity)) {
      Serial.println("DHT read error");
      return;
    }

    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverURL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData =
      "api_key=" + String(apiKey) +
      "&dht_temp=" + String(dhtTemp) +
      "&humidity=" + String(humidity) +
      "&bmp_temp=" + String(bmpTemp) +
      "&pressure=" + String(pressure);

    int httpResponseCode = http.POST(postData);

    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);

    http.end();
  }

  delay(10000); // send every 10 sec
}

