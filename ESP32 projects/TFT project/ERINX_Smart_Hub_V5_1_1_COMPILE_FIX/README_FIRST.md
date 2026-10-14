# ERINX SMART HUB V5.1.1 — WEATHER LOCATION + TOUCH WI-FI SETUP

**Hardware:** ESP32-S3 CAM N16R8, 3.5-inch SPI 480×320 TFT (portrait 320×480), built-in SDMMC, DHT11 on GPIO14. Same wiring and working `TFT_eSPI/User_Setup.h` as V5.0.

## New controls

### Weather city dropdown
Home → WEATHER → tap city bar (v) → Malvar / Tanauan / Lipa / Sto. Tomas. Choosing a city saves it in ESP32 Preferences/NVS, clears the previous city's values and fetches new Open-Meteo data when connected. Default is Tanauan. The coordinates approximate each city/town center; use them as area forecasts, not street-level readings.

### Nearby Wi-Fi connections (no code editing)
Home → SETTINGS → **WI-FI SETUP** → SCAN → choose an SSID → enter password on-screen (for secured networks) → JOIN. The menu displays up to 12 nearby networks, 4 at a time; use PREV/NEXT to page. `Aa` changes between upper/lowercase letters, `#+=` toggles symbols, SPACE inserts a space, DEL removes a character, JOIN connects.

A successful connection is saved to ESP32 local non-volatile storage and automatically reused after reboot. It is **not** uploaded to any server by this project. The password is stored in device flash preferences without adding an encryption layer; only connect to networks you own or have permission to use, and use **FORGET** if passing the device to someone else. FORGET clears saved Wi-Fi details and disconnects. A wrong password leaves the saved working connection untouched.

The ESP32-S3 scans only **2.4 GHz Wi-Fi**. Hidden SSIDs and enterprise/captive-portal networks are not supported by this simple UI. Nearby network scan can temporarily affect an existing Wi-Fi connection. Each new Wi-Fi password must contain at least 8 characters (ordinary WPA2/WPA3 personal passwords).

## V5.1.1 compilation fix

The main sketch requires `PageTypes.h` in the **same directory** as the `.ino` file. This addresses the Arduino-generated function prototype error `showPage(Page page): Page has not been declared`. Open the new `.ino` from its matching folder, not the old V5.1 sketch. The extra file does NOT need uploading to your microSD card.

The line `Multiple libraries were found for "SD.h"` is informational; the compilation failure was about `Page`.

## Install

1. Back up your known-working V5.0 sketch and `User_Setup.h`.
2. In Arduino IDE, open `ERINX_Smart_Hub_V5_1_1_Network_Location/ERINX_Smart_Hub_V5_1_1_Network_Location.ino`.
3. The sketch no longer requires editing Wi-Fi credentials in code.
4. Keep libraries from V5.0: TFT_eSPI (Bodmer), TJpg_Decoder (Bodmer), ArduinoJson v7, DHT sensor library and Adafruit Unified Sensor. The ESP32 platform provides WiFi, Preferences, SD_MMC, FS and HTTPClient.
5. Board = **ESP32S3 Dev Module**, Flash = 16MB, PSRAM = OPI PSRAM. Keep the verified SPI speed (your User_Setup.h), and the same touch wiring.
6. Verify → Upload. Do not copy over your personal SD card photos/videos. If V5.0 icons/sample assets are missing, they are provided in `01_OPTIONAL_SD_FILES/`.
7. On TFT open SETTINGS → WI-FI SETUP, connect to a network, and check the clock + Weather. On the Weather page, test all 4 locations and restart the ESP32 to confirm saved settings.

## Troubleshooting

- **No networks:** Check that router/hotspot has 2.4 GHz enabled; tap SCAN again; if already trying to connect, wait ~15 seconds.
- **Failed to connect:** Re-enter password (case-sensitive); retry. Only successful logins are saved.
- **Time not synced:** Wait several seconds after connecting, NTP requires working Internet.
- **Weather unavailable:** Weather requires Internet access to api.open-meteo.com; Wi-Fi association by itself does not prove Internet access.
- **Video PSRAM unavailable:** Tools → PSRAM → OPI PSRAM, then upload again. This version does not implement the earlier proposed FPS optimization.
- **Screen blank:** keep your previously working TFT_eSPI User_Setup.h and correct breadboard wiring.

## Preservation

Existing six Home menus, animated eyes, Weather symbols, DHT11, clock, Gallery and silent .evm Video player remain in the sketch. All assets remain on the built-in microSD. No new wiring is needed.

## Testing note

Locally checked with C++ stubs and source checks, including resolving Page from the new header; not compiled against your actual ESP32-S3 Arduino core and not hardware-tested. Verify/Upload in Arduino IDE before assuming it works on your device.
