/*
 * Solar IQ - ESP32 Sensor Integration
 * Final Year Capstone Project - Thapar Institute
 * 
 * Monitors: DS18B20 (Panel Temp), DHT11 (Ambient), Voltage Sensor
 * Uploads to Firebase every 30 seconds
 * Detects anomalies: Hotspots, Low Voltage, High Humidity
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

// ============ CONFIGURATION ============
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
#define FIREBASE_HOST "your-project.firebaseio.com"
#define FIREBASE_AUTH "your_firebase_auth_token"

// Pin Definitions
#define DS18B20_PIN 4    // Panel temperature
#define DHT_PIN 5        // Ambient temp & humidity
#define VOLTAGE_PIN 34   // Analog voltage sensor
#define DHT_TYPE DHT11

// ============ SENSOR INITIALIZATION ============
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
DHT dht(DHT_PIN, DHT_TYPE);

FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// ============ VARIABLES ============
float panelTemp = 0;
float ambientTemp = 0;
float humidity = 0;
float voltage = 0;
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 30000; // 30 seconds

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n========================================");
  Serial.println("  Solar IQ - Initializing System");
  Serial.println("  Thapar Institute Capstone Project");
  Serial.println("========================================\n");
  
  // Initialize sensors
  Serial.print("Initializing DS18B20... ");
  ds18b20.begin();
  Serial.println("✓");
  
  Serial.print("Initializing DHT11... ");
  dht.begin();
  Serial.println("✓");
  
  // Connect to WiFi
  connectWiFi();
  
  // Configure Firebase
  Serial.print("Connecting to Firebase... ");
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("✓");
  
  Serial.println("\n✓ Solar IQ System Ready!\n");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = currentMillis;
    
    Serial.println("========== Reading Cycle ==========");
    
    // Read all sensors
    readSensors();
    
    // Display readings
    displayReadings();
    
    // Upload to Firebase
    uploadToFirebase();
    
    // Check for anomalies
    checkAnomalies();
    
    Serial.println("===================================\n");
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" ✓");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println(" ✗ FAILED");
  }
}

void readSensors() {
  // Read DS18B20 (Panel Temperature)
  ds18b20.requestTemperatures();
  panelTemp = ds18b20.getTempCByIndex(0);
  
  // Read DHT11 (Ambient Temperature & Humidity)
  ambientTemp = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // Read Voltage Sensor (0-25V range)
  int analogValue = analogRead(VOLTAGE_PIN);
  voltage = (analogValue / 4095.0) * 25.0;
  
  // Handle sensor errors
  if (isnan(ambientTemp) || isnan(humidity)) {
    Serial.println("⚠ DHT11 Reading Error!");
    ambientTemp = 0;
    humidity = 0;
  }
  
  if (panelTemp == DEVICE_DISCONNECTED_C) {
    Serial.println("⚠ DS18B20 Disconnected!");
    panelTemp = 0;
  }
}

void displayReadings() {
  Serial.println("\n📊 Sensor Readings:");
  Serial.println("─────────────────────────────");
  Serial.printf("  Panel Temperature:   %.2f °C\n", panelTemp);
  Serial.printf("  Ambient Temperature: %.2f °C\n", ambientTemp);
  Serial.printf("  Humidity:            %.2f %%\n", humidity);
  Serial.printf("  Panel Voltage:       %.2f V\n", voltage);
  Serial.printf("  Temp Difference:     %.2f °C\n", panelTemp - ambientTemp);
  Serial.println("─────────────────────────────");
}

void uploadToFirebase() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠ WiFi Disconnected. Reconnecting...");
    connectWiFi();
    return;
  }
  
  String path = "/panels/panel_01/sensors";
  
  // Create JSON object
  FirebaseJson json;
  json.set("panelTemp", panelTemp);
  json.set("ambientTemp", ambientTemp);
  json.set("humidity", humidity);
  json.set("voltage", voltage);
  json.set("tempDiff", panelTemp - ambientTemp);
  json.set("timestamp", String(millis()));
  json.set("date", String(__DATE__));
  json.set("time", String(__TIME__));
  
  // Upload to Firebase
  Serial.print("☁️  Uploading to Firebase... ");
  if (Firebase.setJSON(firebaseData, path, json)) {
    Serial.println("✓");
  } else {
    Serial.println("✗");
    Serial.println("Error: " + firebaseData.errorReason());
  }
  
  // Also log to history
  String historyPath = "/panels/panel_01/history/" + String(millis());
  Firebase.setJSON(firebaseData, historyPath, json);
}

void checkAnomalies() {
  bool alert = false;
  String alertMsg = "";
  String alertType = "none";
  
  // Check for hotspot (panel much hotter than ambient)
  if ((panelTemp - ambientTemp) > 15) {
    alert = true;
    alertType = "hotspot";
    alertMsg += "⚠ HOTSPOT DETECTED! ";
    Serial.println("\n🔥 ALERT: Hotspot detected!");
    Serial.printf("   Panel is %.1f°C hotter than ambient\n", panelTemp - ambientTemp);
  }
  
  // Check for low voltage
  if (voltage < 10.0 && voltage > 0) {
    alert = true;
    alertType = "low_voltage";
    alertMsg += "⚠ LOW VOLTAGE OUTPUT! ";
    Serial.println("\n⚡ ALERT: Low voltage output!");
    Serial.printf("   Voltage: %.2f V (Expected: >10V)\n", voltage);
  }
  
  // Check for high humidity (condensation risk)
  if (humidity > 85) {
    alert = true;
    alertType = "high_humidity";
    alertMsg += "⚠ HIGH HUMIDITY RISK! ";
    Serial.println("\n💧 ALERT: High humidity detected!");
    Serial.printf("   Humidity: %.1f%% (Risk of condensation)\n", humidity);
  }
  
  // Upload alert to Firebase
  if (alert) {
    FirebaseJson alertJson;
    alertJson.set("message", alertMsg);
    alertJson.set("type", alertType);
    alertJson.set("panelTemp", panelTemp);
    alertJson.set("ambientTemp", ambientTemp);
    alertJson.set("humidity", humidity);
    alertJson.set("voltage", voltage);
    alertJson.set("timestamp", String(millis()));
    
    Firebase.setJSON(firebaseData, "/panels/panel_01/alert", alertJson);
    
    Serial.println("📱 Alert logged to Firebase");
  } else {
    Serial.println("✓ All parameters normal");
  }
}
