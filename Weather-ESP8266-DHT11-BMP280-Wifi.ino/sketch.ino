#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

/* ---------- WIFI ---------- */
// ADDED semicolons below
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

/* ---------- SERVER ---------- */
// ADDED "http://" prefix (Required for HTTPClient)
const char* serverURL = "http://192.168.1.128/insert_data.php";
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

  // SDA connected to D2, SCL connected to D1
  Wire.begin(D2, D1);

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 error: Check wiring or I2C address (try 0x77)");
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

    // Check if DHT reading failed
    if (isnan(dhtTemp) || isnan(humidity)) {
      Serial.println("DHT read error");
      return; 
    }

    WiFiClient client;
    HTTPClient http;

    // Begin HTTP connection
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
  } else {
    Serial.println("WiFi Disconnected");
  }

  delay(10000); // send every 10 sec
}
