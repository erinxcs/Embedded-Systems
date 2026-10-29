# Activity 4.1: DHT22 Monitoring Using OLED Display

Shows DHT22 temperature and humidity on a 128×64 SSD1306 OLED, refreshed every two seconds. Readings are also printed to the Serial Monitor at 115200 baud.

## Parts

- ESP32 DevKit
- DHT22 temperature and humidity sensor
- SSD1306 128×64 I2C OLED

## Wiring

| Component | ESP32 pin |
|---|---|
| DHT22 VCC | 3V3 |
| DHT22 data | GPIO4 |
| OLED VCC | 3V3 |
| OLED SDA | GPIO21 |
| OLED SCL | GPIO22 |
| All GND | GND |

## Libraries

- Adafruit SSD1306
- Adafruit GFX Library
- DHT sensor library (Adafruit)

## Troubleshooting

If the display stays blank and the Serial Monitor shows `SSD1306 allocation failed`, change `SCREEN_ADDRESS` from `0x3C` to `0x3D`.

## Run in Wokwi

1. Open the sketch in the Arduino IDE, select **ESP32 Dev Module**, and choose **Sketch → Export Compiled Binary**.
2. Open this folder in VS Code with the Wokwi extension and start the simulator. `wokwi.toml` loads the binary from `build/`.
