# Activity 3.1: LED Control via Bluetooth Serial

Advertises the ESP32 as `ESP32_LED_BT` and toggles an LED from commands sent by a paired phone or computer. Each command is acknowledged over Bluetooth and the USB Serial Monitor.

| Command | Result |
|---|---|
| `1` | LED on |
| `0` | LED off |

## Parts

- ESP32 DevKit (original ESP32)
- LED and 220 Ω resistor, or the on-board LED on GPIO2

## Wiring

| Component | ESP32 pin |
|---|---|
| LED anode (through 220 Ω) | GPIO2 |
| LED cathode | GND |

## Usage

1. Upload the sketch and pair with `ESP32_LED_BT`.
2. Connect with a Bluetooth serial terminal app, such as Serial Bluetooth Terminal on Android.
3. Send `1` or `0`.

> **Board note:** `BluetoothSerial` uses Bluetooth Classic, which is available only on the original ESP32. It does not work on the ESP32-S2, S3, C3, or in Wokwi.
