/*
 * Solar IQ - ESP32 Sensor Integration
 * Monitors DS18B20, DHT11, and Voltage
 * Uploads to Firebase every 30 seconds
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

// Configuration
#define WIFI_SSID "your_wifi"
#define WIFI_PASSWORD "your_password"
#define FIREBASE_HOST "your-project.firebaseio.com"

// Pin definitions
#define DS18B20_PIN 4
#define DHT_PIN 5
#define VOLTAGE_PIN 34

// Initialize sensors
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
DHT dht(DHT_PIN, DHT11);

void setup() {
  Serial.begin(115200);
  ds18b20.begin();
  dht.begin();
  connectWiFi();
  initFirebase();
}

void loop() {
  float panelTemp = readPanelTemperature();
  float ambientTemp = dht.readTemperature();
  float humidity = dht.readHumidity();
  float voltage = readVoltage();
  
  uploadToFirebase(panelTemp, ambientTemp, humidity, voltage);
  checkAnomalies(panelTemp, ambientTemp, voltage, humidity);
  
  delay(30000); // 30 second interval
}

float readPanelTemperature() {
  ds18b20.requestTemperatures();
  return ds18b20.getTempCByIndex(0);
}

float readVoltage() {
  int raw = analogRead(VOLTAGE_PIN);
  return (raw / 4095.0) * 25.0; // 0-25V range
}

void checkAnomalies(float pTemp, float aTemp, float v, float h) {
  if (pTemp - aTemp > 15) {
    Firebase.setString(firebaseData, "/alerts", "Hotspot detected");
  }
  if (v < 10.0) {
    Firebase.setString(firebaseData, "/alerts", "Low voltage");
  }
}

// Additional helper functions...
