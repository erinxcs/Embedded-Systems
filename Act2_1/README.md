# Activity 2.1: LED Control Using Serial Communication

Turns an LED on or off from commands typed into the Serial Monitor.

| Command | Result |
|---|---|
| `1` | LED on |
| `0` | LED off |

Other characters, including newlines, are ignored.

## Parts

- ESP32 DevKit
- LED and 220 Ω resistor

## Wiring

| Component | ESP32 pin |
|---|---|
| LED anode (through 220 Ω) | GPIO2 |
| LED cathode | GND |

## Usage

Open the Serial Monitor at 115200 baud and send `1` or `0`.

## Run in Wokwi

1. Open the sketch in the Arduino IDE, select **ESP32 Dev Module**, and choose **Sketch → Export Compiled Binary**.
2. Open this folder in VS Code with the Wokwi extension and start the simulator. `wokwi.toml` loads the binary from `build/`.
