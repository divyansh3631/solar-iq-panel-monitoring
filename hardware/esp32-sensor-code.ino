/*
 * Solar IQ - ESP32 Sensor Integration
 * Monitors DS18B20, DHT11, and Voltage Sensor
 * Uploads data to Firebase every 30 seconds
 * 
 * Final Year Capstone Project
 * Thapar Institute of Engineering and Technology
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

// WiFi Configuration
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// Firebase Configuration
#define FIREBASE_HOST "your-project.firebaseio.com"
#define FIREBASE_AUTH "your_firebase_auth_token"

// Pin Definitions
#define DS18B20_PIN 4        // Panel temperature sensor
#define DHT_PIN 5            // Ambient temp & humidity
#define VOLTAGE_PIN 34       // Analog voltage sensor
#define DHT_TYPE DHT11

// Initialize Sensors
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
DHT dht(DHT_PIN, DHT_TYPE);

// Firebase Objects
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// Variables
float panelTemp = 0;
float ambientTemp = 0;
float humidity = 0;
float voltage = 0;
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 30000; // 30 seconds

void setup() {
  Serial.begin(115200);
  Serial.println("Solar IQ - Initializing...");
  
  // Initialize sensors
  ds18b20.begin();
  dht.begin();
  
  // Connect to WiFi
  connectWiFi();
  
  // Configure Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Solar IQ System Ready!");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = currentMillis;
    
    // Read all sensors
    readSensors();
    
    // Display readings
    displayReadings();
    
    // Upload to Firebase
    uploadToFirebase();
    
    // Check for anomalies
    checkAnomalies();
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void readSensors() {
  // Read DS18B20 (Panel Temperature)
  ds18b20.requestTemperatures();
  panelTemp = ds18b20.getTempCByIndex(0);
  
  // Read DHT11 (Ambient Temperature & Humidity)
  ambientTemp = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // Read Voltage Sensor
  int analogValue = analogRead(VOLTAGE_PIN);
  voltage = (analogValue / 4095.0) * 25.0; // 0-25V range
  
  // Handle sensor errors
  if (isnan(ambientTemp) || isnan(humidity)) {
    Serial.println("DHT11 Error!");
    ambientTemp = 0;
    humidity = 0;
  }
  
  if (panelTemp == DEVICE_DISCONNECTED_C) {
    Serial.println("DS18B20 Error!");
    panelTemp = 0;
  }
}

void displayReadings() {
  Serial.println("\n--- Sensor Readings ---");
  Serial.printf("Panel Temp:   %.2f °C\n", panelTemp);
  Serial.printf("Ambient Temp: %.2f °C\n", ambientTemp);
  Serial.printf("Humidity:     %.2f %%\n", humidity);
  Serial.printf("Voltage:      %.2f V\n", voltage);
  Serial.println("----------------------\n");
}

void uploadToFirebase() {
  if (WiFi.status() == WL_CONNECTED) {
    String path = "/panels/panel_01/sensors";
    
    // Create JSON object
    FirebaseJson json;
    json.set("panelTemp", panelTemp);
    json.set("ambientTemp", ambientTemp);
    json.set("humidity", humidity);
    json.set("voltage", voltage);
    json.set("timestamp", String(millis()));
    
    // Upload
    if (Firebase.setJSON(firebaseData, path, json)) {
      Serial.println("✓ Data uploaded to Firebase");
    } else {
      Serial.println("✗ Upload failed: " + firebaseData.errorReason());
    }
  } else {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectWiFi();
  }
}

void checkAnomalies() {
  bool alert = false;
  String alertMsg = "";
  
  // Check for hotspot
  if (panelTemp - ambientTemp > 15) {
    alert = true;
    alertMsg += "Hotspot detected! ";
  }
  
  // Check for low voltage
  if (voltage < 10.0) {
    alert = true;
    alertMsg += "Low voltage output! ";
  }
  
  // Check for high humidity
  if (humidity > 85) {
    alert = true;
    alertMsg += "High humidity risk! ";
  }
  
  if (alert) {
    Firebase.setString(firebaseData, "/panels/panel_01/alert", alertMsg);
    Serial.println("⚠ ALERT: " + alertMsg);
  }
}
