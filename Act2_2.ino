//Carl Jayson Eli Bonaobra
//CPE4A
// ===== Activity 2.2: DHT11 Monitoring Using Serial Monitor =====

#include <DHT.h>

#define DHTPIN 4        // GPIO connected to DHT11 data pin
#define DHTTYPE DHT11   // Sensor type

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("DHT11 Monitoring Starting...");
  dht.begin();
}

void loop() {
  delay(2000); // DHT11 needs ~2s between readings

  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();       // Celsius
  float tempF = dht.readTemperature(true);   // Fahrenheit

  if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
    Serial.println("Failed to read from DHT11 sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %  |  Temperature: ");
  Serial.print(tempC);
  Serial.print(" °C / ");
  Serial.print(tempF);
  Serial.println(" °F");
}