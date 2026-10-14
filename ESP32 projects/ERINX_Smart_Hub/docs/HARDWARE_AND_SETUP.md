# Hardware and Firmware Setup — V5.1.3

## Pin reference (ESP32-S3 CAM N16R8)

| Connection | ESP32-S3 GPIO / supply | Notes |
|---|---|---|
| TFT CS | 42 | Display chip-select |
| TFT DC | 41 | Display data/command |
| TFT RST | 48 | Display reset |
| TFT MOSI / touch DIN | 21 | Shared SPI MOSI |
| TFT SCK / touch CLK | 47 | Shared SPI clock |
| Touch DO (MISO) | 1 | TFT SDO left unconnected in V4 reference |
| Touch CS | 2 | Touch chip-select |
| DHT11 DATA | 14 | Sensor powered at board-compatible 3.3 V |
| SDMMC CLK | 39 | Built-in board microSD |
| SDMMC CMD | 38 | Built-in board microSD |
| SDMMC D0 | 40 | Built-in board microSD, 1-bit mode |

**Power:** TFT module VCC and LED/backlight pins depend on the module's regulator/backlight circuitry. Reuse the previously verified wiring; do **not** assume 3.3 V or 5 V solely from a silkscreen label. Power OFF when changing wiring. All logic circuits must share appropriate ground. The ESP32-S3 CAM camera is physically present but **not used by V5.1.3**.

## Libraries / IDE

- Select **ESP32S3 Dev Module** with 16MB Flash and OPI PSRAM, appropriate USB-UART port.
- Install Bodmer `TFT_eSPI`, Bodmer `TJpg_Decoder`, Benoit Blanchon `ArduinoJson` v7, Adafruit `DHT sensor library` and `Adafruit Unified Sensor`.
- The board package supplies SD_MMC, Preferences, WiFi, HTTPClient, FS and WiFiClientSecure.
- Verify `TFT_eSPI/User_Setup.h` loads ILI9488/TFT/touch assignments that already work on the device. See `hardware/configs/ERINX_V4_User_Setup.h`.
- Final Arduino sketch folder must contain the `.ino` **and** `PageTypes.h`.
- Monitor Serial at **115200 baud**.

## microSD card root example

```text
/
  icons/
    weather.rgb565  clock.rgb565  dht11.rgb565
    gallery.rgb565  video.rgb565  settings.rgb565
  photos/
    ERINX_SAMPLE_01.jpg  ERINX_SAMPLE_02.jpg  ERINX_SAMPLE_03.jpg
  videos/
    ERINX_SAMPLE_LOOP.evm
```

Icons are raw 40x40 RGB565 (3,200 bytes each). Photos should preferably be portrait baseline JPG; the sketch can decode scaled JPEG. Videos must be produced as *silent custom* `.evm` files using `tools/video_converter/convert_videos.py`, following that script's usage and dependencies (FFmpeg + Python Pillow). Open `.ino` with Arduino IDE; `.py` scripts run on a PC rather than on ESP32.

## Weather location table (as stored in V5.1.3 source)

| Location | Latitude | Longitude |
|---|---:|---:|
| Malvar | 14.05425 | 121.15479 |
| Tanauan | 14.06741 | 121.15311 |
| Lipa | 13.96816 | 121.15909 |
| Sto. Tomas | 14.11073 | 121.14236 |

Note: the current Open-Meteo URL builder formats each number to 4 decimal places (rather than all stored digits); forecasts are area-based, not exact-point measurements.
