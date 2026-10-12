# ERINX Smart Hub V4.5 — Portrait UI Optimization

This is the **V4.4 feature set with UI/performance improvements**, not a new feature release. TFT stays **320×480 portrait**, without changing any hardware pin mapping.

## Changes vs. V4.4

- Cleaner ERINX OS home layout with dark navy/cyan, 2×3 menu tiles, a Philippine time strip and a status footer.
- More readable and consistent card text. Transparent text no longer paints a mismatched background color behind letters.
- Only the changed clock text is redrawn (home once per minute/status change, clock page once per second).
- DHT11 only updates the live values and status area rather than clearing the whole climate page every 3 seconds.
- X/Y touch footer updates its own region instead of refreshing the full footer.
- Non-blocking Wi-Fi connection at startup (UI need not wait up to six seconds to appear).
- Short tap debounce and the same stored portrait touch calibration (ESP32 NVS namespace `erinxtft43`).
- Keeps the existing SD card listing, weather fetch, Wi-Fi and DHT11 logic. No JPEG video rendering yet.

## Install and run

1. **Keep the SAME working `TFT_eSPI/User_Setup.h` and jumper wiring** from V4.4. Do not edit library setup.
2. Extract this ZIP, open `ERINX_Smart_Hub_V4_5/ERINX_Smart_Hub_V4_5.ino` in Arduino IDE.
3. In the sketch, set `WIFI_SSID` and `WIFI_PASSWORD` locally to use Open-Meteo and NTP. Leave SSID empty to run offline. Do not share Wi-Fi passwords in screenshots.
4. Required: TFT_eSPI (existing working setup), ArduinoJson v7, Adafruit DHT sensor library, Adafruit Unified Sensor. Built-in ESP32 libraries provide WiFi, SD_MMC, HTTPClient and Preferences.
5. Board: ESP32S3 Dev Module; 16MB Flash; OPI PSRAM; Serial Monitor 115200 baud; USB-UART port.
6. Test: home menu buttons, back navigation, weather page, clock page (1 second time update), climate page (3-second sensor polling), gallery/video file listing, Settings > Calibrate Touch.

## Unchanged wiring

| Connection | GPIO |
|---|---|
| TFT MOSI / Touch DIN | 21 |
| TFT SCK / Touch CLK | 47 |
| TFT CS | 42 |
| TFT DC | 41 |
| TFT RST | 48 |
| Touch CS | 2 |
| Touch DO / MISO | 1 |
| DHT11 DATA | 14 |
| SDMMC internal CMD / CLK / D0 | 38 / 39 / 40 |

Keep existing *verified* VCC/GND/LED voltage connections. Do not connect the TFT's separate SD card slot.

## Notes / limitations

- This build has **not** been verified on the physical ESP32-S3 hardware or compiled in Arduino IDE from this environment. Keep your working V4.4 backup for easy rollback.
- Weather HTTPS uses `client.setInsecure()` as in V4.4, so certificates are not verified. Replace with CA verification for a security-sensitive/production release.
- HTTP weather fetching can still briefly pause touch input during a network request. Scheduled only about every 10 minutes; a future version can move fetching to a background task if needed.
- Gallery and video pages are **filename browsers only**; they do not render media yet.
- DHT11 readings appear only when wired; outdoor weather still requires connected Wi-Fi and an API response.
- No new feature added — next changes can be chosen after physical testing.
