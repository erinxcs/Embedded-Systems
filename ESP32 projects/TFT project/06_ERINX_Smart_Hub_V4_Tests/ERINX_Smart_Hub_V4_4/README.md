# ERINX Smart Hub V4.4 — Live Weather + Philippine Clock + DHT11

This is an incremental update to the **working V4.3.1** (same TFT_eSPI `User_Setup.h`, pin wiring, SDMMC, touch calibration, menus and media file browser).

## What's new

- Live **Open-Meteo** current weather from `api.open-meteo.com`: temperature, relative humidity, feels-like, condition, wind, and API report time (Asia/Manila).
- Refresh current weather about every **10 minutes** when Wi-Fi is available; retry about every minute if fetch fails. If offline, the screen either shows its last successful values explicitly marked offline or an empty state.
- Internet/NTP **Philippine Standard Time (UTC+8)** when connected, otherwise existing unsynced-time/uptime fallback.
- **DHT11 indoor sensor** readings every 3 seconds on GPIO14, with no fabricated data if sensor is absent.
- Keeps **6 touch pages**, saved calibration, TFT/eSPI configuration, built-in SD card, and photo/video *filename browsing*.

**Not yet implemented:** Actual JPEG display and MJPEG video playback, alarm audio, image gallery UI.

## Required Arduino libraries

Install using Arduino IDE > Sketch > Include Library > Manage Libraries:
- **TFT_eSPI** by Bodmer (already working in V4.3.1).
- **ArduinoJson** by Benoit Blanchon **version 7**.
- **DHT sensor library** by Adafruit, and **Adafruit Unified Sensor** dependency.

`WiFi.h`, `WiFiClientSecure.h`, `HTTPClient.h`, `SD_MMC.h`, `FS.h`, and `Preferences.h` are included in the ESP32 Arduino core. The code is designed for ESP32 Arduino Core **3.x**, matching the user's 3.3.12 installation.

## Setup

1. **Keep the working** `TFT_eSPI/User_Setup.h` from V4.3.1. No TFT pin changes.
2. Copy/extract the project folder and open `ERINX_Smart_Hub_V4_4.ino` in Arduino IDE.
3. In the top of the `.ino`, enter **Wi-Fi SSID and password** in `WIFI_SSID` / `WIFI_PASSWORD` (only store these locally; don't post screenshots showing them).
4. Default location is approximate city-centre **Tanauan, Batangas**, `14.0860`, `121.1500`. Edit `WEATHER_LATITUDE`, `WEATHER_LONGITUDE`, and `WEATHER_LOCATION` to change it.
5. Optional: wire DHT11 using the following table **with USB unplugged**. You can skip the DHT11 temporarily; it will show a no-reading message.
6. Verify and upload to **ESP32S3 Dev Module**, 16 MB Flash, OPI PSRAM, and the usual USB-UART port/COM setting.
7. At first run the touch calibration is saved (unchanged NVS namespace from V4.3.1); restart and open the Weather / Clock / DHT11 pages.
8. Serial Monitor baud **115200** prints Wi-Fi, weather request and sensor diagnostics.

## DHT11 wiring

For common **3-pin modules** with `+ / OUT / -` or `VCC / DATA / GND`:

| DHT11 pin | ESP32-S3 CAM |
|---|---|
| VCC / + | **3V3** |
| DATA / OUT | **GPIO14** |
| GND / - | **GND** |

**Bare 4-pin DHT11 sensors** often need a 4.7k–10k pull-up resistor from DATA to 3V3. Confirm the manufacturer pin order; it is NOT the same as 3-pin breakout modules. Do not connect sensor data or a bare DHT11 to 5V ESP32 logic.

GPIO14 is exposed on the user's ESP32-S3 board and does not conflict with the currently used TFT, touch, 1-bit SDMMC, or the documented Freenove-style camera signal assignments. If another device is already on GPIO14, change `DHT_PIN` and rewire.

## Current TFT pin reference (unchanged)

| TFT signal | ESP32-S3 |
|---|---|
| CS | GPIO42 |
| DC/RS | GPIO41 |
| RST | GPIO48 |
| SDI/MOSI | GPIO21 |
| SCK | GPIO47 |
| TOUCH_CS | GPIO2 |
| T_CLK | GPIO47 |
| T_DIN | GPIO21 |
| T_DO | GPIO1 |

TFT VCC/LED supply remains **exactly as previously verified working** and must match the module voltage rating. Do not connect the TFT SD slot, since built-in SDMMC uses CMD=38, CLK=39, D0=40 internally.

## Troubleshooting

- **Weather waits**: Check SSID/password, Wi-Fi signal, Internet connectivity and Serial output (`HTTP error` / `JSON error`). The board requires **2.4 GHz Wi-Fi**. The clock might sync a few seconds after connecting.
- **Weather request succeeds but page blank**: check libraries and restart; weather values only update after a real API response.
- **DHT11 no reading**: check actual +/OUT/- labels, 3V3/GND/GPIO14, pull-up resistor for bare sensor, and common ground. DHT11 values update around every 3 s.
- **Wi-Fi credentials**: Do not share your real password when sending diagnostic code.
- **Weather model values**: These are weather forecast model/current estimates for the selected coordinate, not measurements by the DHT11.
- **JPEG/MJPEG pages**: File listing only (unchanged). Rendering/player comes in V4.5.

## Security/limitations

This prototype uses `WiFiClientSecure::setInsecure()` for a **public read-only weather API request** to keep the first hardware integration simple. It disables HTTPS certificate verification for this request. For long-term or sensitive deployments, switch to root CA verification and update certificates as necessary.

**Not tested on physical hardware in this environment**; compile and test on the user's ESP32-S3. Keep V4.3.1 as a rollback if needed.

## API

[Open-Meteo forecast API docs](https://open-meteo.com/en/docs) — no API key for qualifying non-commercial usage.
