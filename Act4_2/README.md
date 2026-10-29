# Activity 4.2: Volume Control Using OLED Display and Potentiometer

Maps a potentiometer to a 0–100% volume level and draws it on a 128×64 SSD1306 OLED as a large percentage with a bar graph. The screen redraws only when the value changes.

## Parts

- ESP32 DevKit
- Potentiometer
- SSD1306 128×64 I2C OLED

## Wiring

| Component | ESP32 pin |
|---|---|
| Potentiometer VCC | 3V3 |
| Potentiometer SIG | GPIO34 |
| OLED VCC | 3V3 |
| OLED SDA | GPIO21 |
| OLED SCL | GPIO22 |
| All GND | GND |

## Libraries

- Adafruit SSD1306
- Adafruit GFX Library

## Run in Wokwi

1. Open the sketch in the Arduino IDE, select **ESP32 Dev Module**, and choose **Sketch → Export Compiled Binary**.
2. Open this folder in VS Code with the Wokwi extension and start the simulator. `wokwi.toml` loads the binary from `build/`.
