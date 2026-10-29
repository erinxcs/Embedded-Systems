# Activity 3.3: DHT11 Monitoring via Bluetooth Serial

Reads a DHT11 every two seconds and sends the result to both the USB Serial Monitor and a Bluetooth serial connection named `ESP32_DHT11_BT`.

## Parts

- ESP32 DevKit (original ESP32)
- DHT11 temperature and humidity sensor

## Wiring

| Component | ESP32 pin |
|---|---|
| DHT VCC | 3V3 |
| DHT data | GPIO4 |
| DHT GND | GND |

## Libraries

- DHT sensor library (Adafruit)

## Usage

Pair with `ESP32_DHT11_BT` and open a Bluetooth serial terminal. Readings arrive as:

`Humidity: 65.0% | Temp: 29.0C / 84.2F`

> **Board note:** `BluetoothSerial` uses Bluetooth Classic, which is available only on the original ESP32. It does not work on the ESP32-S2, S3, C3, or in Wokwi.
