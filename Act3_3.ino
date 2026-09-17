//Carl Jayson Eli Bonaobra
//CPE4A
// ===== Activity 3.3: DHT11 Monitoring via Bluetooth Serial =====

#include "BluetoothSerial.h"
#include <DHT.h>

BluetoothSerial SerialBT;

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  SerialBT.begin("ESP32_DHT11_BT");
  Serial.println("Bluetooth DHT11 Monitor Ready. Pair with 'ESP32_DHT11_BT'.");
}

void loop() {
  delay(2000); // DHT11 minimum sampling interval

  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();
  float tempF = dht.readTemperature(true);

  if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
    Serial.println("Failed to read from DHT11 sensor!");
    SerialBT.println("Sensor read error!");
    return;
  }

  char buffer[80];
  snprintf(buffer, sizeof(buffer), "Humidity: %.1f%% | Temp: %.1fC / %.1fF", humidity, tempC, tempF);

  Serial.println(buffer);
  SerialBT.println(buffer);
}