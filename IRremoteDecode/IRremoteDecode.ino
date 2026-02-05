#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ---------- SCREEN SETTINGS ---------- */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* ---------- WIFI ---------- */
const char* ssid = "Waheguru";        // <--- CHANGE THIS
const char* password = "Gurunanak007"; // <--- CHANGE THIS

/* ---------- SERVER ---------- */
// Ensure you include http://
const char* serverURL = "http://192.168.1.128/insert_data.php"; // <--- CHANGE IP IF NEEDED
const char* apiKey = "secret_esp8266_key";

/* ---------- PINS ---------- */
#define DHTPIN D4
#define DHTTYPE DHT11

Adafruit_BMP280 bmp;
DHT dht(DHTPIN, DHTTYPE);

/* ---------- TIMERS ---------- */
unsigned long lastUploadTime = 0;
unsigned long lastDisplaySwitch = 0;
const long uploadInterval = 10000;   // Send to server every 10 seconds
const long displayInterval = 3000;   // Switch screen info every 3 seconds

int displayState = 0; // 0=Temp, 1=Hum, 2=Press

// Global variables to store sensor readings
float dhtTemp = 0;
float humidity = 0;
float bmpTemp = 0;
float pressure = 0;

/* ---------- SETUP ---------- */
void setup() {
  Serial.begin(9600);
  delay(1000);

  // Initialize I2C (D2=SDA, D1=SCL)
  Wire.begin(D2, D1);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    // We continue anyway, but screen might not work
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Booting...");
  display.display();

  // Initialize Sensors
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 error");
    display.println("BMP Error");
    display.display();
    while (1);
  }
  dht.begin();

  // Initialize WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connecting WiFi...");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  display.println("Connected!");
  display.display();
  delay(1000);
}

/* ---------- LOOP ---------- */
void loop() {
  unsigned long currentMillis = millis();

  // 1. READ SENSORS (Always keep variables updated)
  float newDhtTemp = dht.readTemperature();
  float newHum = dht.readHumidity();
  float newBmpTemp = bmp.readTemperature();
  float newPress = bmp.readPressure() / 100.0;

  // Only update globals if read was successful to avoid sending NaN
  if (!isnan(newDhtTemp) && !isnan(newHum)) {
    dhtTemp = newDhtTemp;
    humidity = newHum;
  }
  if (!isnan(newBmpTemp)) {
    bmpTemp = newBmpTemp;
    pressure = newPress;
  }

  // 2. HANDLE DISPLAY (Switch every 3 seconds)
  if (currentMillis - lastDisplaySwitch >= displayInterval) {
    lastDisplaySwitch = currentMillis;
    displayState++;
    if (displayState > 2) displayState = 0; // Cycle back to 0
    updateDisplay();
  }

  // 3. UPLOAD TO SERVER (Every 10 seconds)
  if (currentMillis - lastUploadTime >= uploadInterval) {
    lastUploadTime = currentMillis;
    if (WiFi.status() == WL_CONNECTED) {
      sendDataToServer();
    } else {
      Serial.println("WiFi Disconnected");
    }
  }
}

/* ---------- HELPER FUNCTIONS ---------- */

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (displayState == 0) {
    // Show Temperature
    display.println("TEMPERATURE");
    display.drawLine(0, 10, 128, 10, WHITE); // decorative line
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.print(dhtTemp, 1);
    display.print(" C");
  } 
  else if (displayState == 1) {
    // Show Humidity
    display.println("HUMIDITY");
    display.drawLine(0, 10, 128, 10, WHITE);
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.print(humidity, 1);
    display.print(" %");
  } 
  else if (displayState == 2) {
    // Show Pressure
    display.println("PRESSURE");
    display.drawLine(0, 10, 128, 10, WHITE); 
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.print(pressure, 1);
    display.setTextSize(1);
    display.print(" hPa");
  }

  display.display();
}

void sendDataToServer() {
   WiFiClient client;
   HTTPClient http;

   http.begin(client, serverURL);
   http.addHeader("Content-Type", "application/x-www-form-urlencoded");

   // Matches your PHP POST variables exactly
   String postData =
     "api_key=" + String(apiKey) +
     "&dht_temp=" + String(dhtTemp) +
     "&humidity=" + String(humidity) +
     "&bmp_temp=" + String(bmpTemp) +
     "&pressure=" + String(pressure);

   int httpResponseCode = http.POST(postData);

   Serial.print("Sending Data... Response: ");
   Serial.println(httpResponseCode);

   http.end();
}