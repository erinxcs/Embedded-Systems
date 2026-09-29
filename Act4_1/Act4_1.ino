//Carl Jayson Eli Bonaobra
//CPE4A
// ===== Activity 4.1: DHT22 Monitoring Using OLED Display =====

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C // common address; try 0x3D if this doesn't work

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while (true); // halt
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("DHT22 Monitor Starting...");
  display.display();
  delay(1500);
}

void loop() {
  delay(2000); // DHT22 minimum sampling interval

  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("DHT22 Sensor Data");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  if (isnan(humidity) || isnan(tempC)) {
    display.setCursor(0, 24);
    display.println("Sensor read failed");
    Serial.println("Failed to read DHT22 sensor");
  } else {
    display.setCursor(0, 20);
    display.print("Temperature: ");
    display.print(tempC, 1);
    display.println(" C");
    display.setCursor(0, 40);
    display.print("Humidity: ");
    display.print(humidity, 1);
    display.println(" %");
    Serial.print("Temperature: ");
    Serial.print(tempC, 1);
    Serial.print(" C, Humidity: ");
    Serial.print(humidity, 1);
    Serial.println(" %");
  }
  display.display();
}
