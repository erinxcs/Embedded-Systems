# Activity 2.2: DHT11 Monitoring Using Serial Monitor

Reads humidity and temperature from a DHT11 every two seconds and prints them in Celsius and Fahrenheit. A failed reading prints an error and is skipped.

## Parts

- ESP32 DevKit
- DHT11 temperature and humidity sensor

## Wiring

| Component | ESP32 pin |
|---|---|
| DHT VCC | 3V3 |
| DHT data | GPIO4 |
| DHT GND | GND |

## Libraries

- DHT sensor library (Adafruit)

## Expected output

`Humidity: 65.00 %  |  Temperature: 29.00 °C / 84.20 °F`

> **Simulation note:** Wokwi has no DHT11 part, so `diagram.json` uses a DHT22. The sketch is written for a physical DHT11; change `DHTTYPE` to `DHT22` for accurate simulated readings. The LED in the diagram is not used by this sketch.

## Run in Wokwi

1. Open the sketch in the Arduino IDE, select **ESP32 Dev Module**, and choose **Sketch → Export Compiled Binary**.
2. Open this folder in VS Code with the Wokwi extension and start the simulator. `wokwi.toml` loads the binary from `build/`.
