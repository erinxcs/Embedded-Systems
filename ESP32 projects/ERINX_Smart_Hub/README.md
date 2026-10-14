# ERINX Smart Hub — Embedded Systems Mini Project

**Prepared by:** Carl Jayson Eli B. Bonaobra  
**Section:** CPE4A  
**Project:** Embedded Systems Mini Project  
**Main hardware:** ESP32-S3 CAM N16R8, 3.5-inch SPI TFT touchscreen (ILI9488/XPT2046-type), DHT11, built-in microSD slot.  
**Final source version:** V5.1.3 (archived final version; not freshly hardware-tested during reorganization).

## Start here

1. Open `firmware/final/ERINX_Smart_Hub_V5_1_3_UI_Controls/ERINX_Smart_Hub_V5_1_3_UI_Controls.ino` in Arduino IDE.
2. **Keep `PageTypes.h` in the SAME folder** as the `.ino` or Arduino compilation can fail.
3. Choose **ESP32S3 Dev Module**, 16 MB flash and OPI PSRAM (match your working board configuration).
4. Use your verified `TFT_eSPI/User_Setup.h` pin configuration. `hardware/configs/ERINX_V4_User_Setup.h` contains the preserved S3 baseline reference; do not overwrite an already working setup without checking.
5. Install **TFT_eSPI, TJpg_Decoder, ArduinoJson v7, Adafruit DHT sensor library, and Adafruit Unified Sensor**. ESP32 platform includes WiFi, FS, SD_MMC, Preferences, HTTPClient.
6. If you need example media, copy the **contents** of `sd_card/COPY_TO_SD_ROOT/` directly to the root of a compatible FAT32 microSD card in the ESP32-S3 camera board's **built-in** slot.
7. Verify → Upload → calibrate touch on first run → Settings → Wi-Fi Setup → choose 2.4 GHz Wi-Fi → Weather.

**Important:** Never copy the entire `sd_card` directory onto the card; copy the `icons`, `photos`, and `videos` folders contained in `COPY_TO_SD_ROOT` into the card root. Existing media can be retained. The `.evm` video format is custom and **silent**; ordinary `.mp4` files are not directly playable by the firmware.

## Organized repository map

| Folder | Contents |
|---|---|
| `firmware/final/` | **V5.1.3** final sketch and matching `PageTypes.h` |
| `firmware/development/ESP32D/` | Early weather and touchscreen software generations |
| `firmware/development/ESP32S3/` | V4/V5 progressive feature versions |
| `firmware/diagnostics/` | Display, touch, SDMMC and FPS test sketches |
| `sd_card/COPY_TO_SD_ROOT/` | Deduplicated icon, JPG photo, silent EVM sample files |
| `tools/photo_converter/` | PC JPG preparation utility |
| `tools/video_converter/` | PC video to custom EVM conversion utility (FFmpeg/Pillow dependencies) |
| `hardware/configs/` | TFT_eSPI settings for early ESP32D and later ESP32-S3 |
| `assets/` | UI mockups, icon previews and editable vectors |
| `docs/` | Documentation, setup instructions, source inventory and original release notes |

The contents were **moved, not refactored**. All original `.ino` source bytes are preserved. The final sketch is kept separate from historical and diagnostic sketches to avoid accidentally uploading an old version. Identical repeated SD media and copied notes are deduplicated; `docs/SOURCE_FILE_MANIFEST.csv` records each original path and its canonical destination. `PageTypes.h` is intentionally retained with every corresponding V5.1.x sketch to preserve independent Arduino compilation.

## Features in the final sketch

- Six-menu touch home screen: Weather, Clock, DHT11, Gallery, Video, Settings.
- Outdoor Open-Meteo forecast for Malvar, Tanauan, Lipa, Sto. Tomas; city selection stored in Preferences.
- Local DHT11 readings via GPIO14; indoor readings are separate from API weather.
- Philippine time using network NTP; uptime fallback if unsynchronized.
- Internal SDMMC 1-bit microSD support, RGB565 icons, JPG photo browser/slideshow and custom silent `.evm` video playback.
- Nearby Wi-Fi scan, on-screen password entry with SHOW/HIDE, saved Wi-Fi on device.

## Limitations and publication notes

- The video player is silent; the ESP32-S3 CAM camera module is **not** used for taking photos in this version.
- Open-Meteo requires internet, and the code uses `WiFiClientSecure::setInsecure()` for HTTPS. This is acceptable as a prototype tradeoff but should be improved before production use.
- The current Wi-Fi password entry defaults to **visible**, and saved Wi-Fi credentials are in on-device NVS without an app-added encryption layer; hide sensitive passwords when demonstrating.
- Board SPI/touch/power wiring depends on the exact TFT module and known-working connections. Power off before rewiring.
- The archive review is a static structural/asset inventory, **not** an ESP32 board-toolchain compile or a physical hardware verification. Record real measurements/screenshots separately for the project paper.

See `docs/HARDWARE_AND_SETUP.md`, `docs/DEVELOPMENT_VERSIONS.md`, and `docs/TESTING_CHECKLIST.md` for details.
