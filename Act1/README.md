# Activity 1: LED Dimming Controlled by Potentiometer

Reads a potentiometer on the 12-bit ADC (0–4095) and maps it to an 8-bit PWM duty cycle (0–255) that sets the LED brightness. The sketch supports both ESP32 Arduino core 3.x (`analogWrite`) and 2.x (`ledcSetup` / `ledcWrite`).

## Parts

- ESP32 DevKit
- Potentiometer
- LED and 220 Ω resistor

## Wiring

| Component | ESP32 pin |
|---|---|
| Potentiometer VCC | 3V3 |
| Potentiometer SIG | GPIO34 |
| Potentiometer GND | GND |
| LED anode (through 220 Ω) | GPIO2 |
| LED cathode | GND |

## Expected output

Serial Monitor at 115200 baud prints readings such as `Pot Value: 2048  |  Brightness: 127`.

## Run in Wokwi

1. Open the sketch in the Arduino IDE, select **ESP32 Dev Module**, and choose **Sketch → Export Compiled Binary**.
2. Open this folder in VS Code with the Wokwi extension and start the simulator. `wokwi.toml` loads the binary from `build/`.
