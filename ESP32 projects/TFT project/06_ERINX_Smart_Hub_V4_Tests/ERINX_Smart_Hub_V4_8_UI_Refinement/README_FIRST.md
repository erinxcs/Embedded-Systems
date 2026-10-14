# ERINX SMART HUB V4.8 — Portrait UI Refinement

**Target hardware:** ESP32-S3 CAM N16R8, 3.5-inch 480×320 SPI TFT displayed in portrait (320×480), resistive touch, built-in microSD and optional DHT11 on GPIO14.

Built on the V4.7 sketch that was working on your device. No pin changes, no changes to the working `TFT_eSPI/User_Setup.h`, and no new third-party libraries.

## What's changed

- Original title `ERINX SMART HUB` retained; existing home/menu cards and six pages preserved.
- **Wi-Fi status:** three CURVED Wi-Fi signal arcs, with a small **!** when not connected to Wi-Fi; home and page headers update when Wi-Fi connection changes. This is the Wi-Fi association status, **not a separate internet connectivity test**.
- **Robot eyes:** two independently shaped white eyes inside a dark frame (not the old single oval), looking around, blinking, sleepy/cross-eyed. Small animated face top-right on Home. Large animated face in the empty lower part of Clock.
- **Weather:** centered scene without the old blue rectangle or multiple drifting clouds. Four code-driven animations: sunny, cloudy, rainy, thunderstorm. Uses Open-Meteo WMO `weather_code`. While there is no weather reading, a neutral cloud is shown, **not a fabricated forecast**.
- Low-frequency partial redrawing, no continuous full-screen repaints on these animations.
- The SD icon files are unchanged from V4.7; no need to recopy if already working.

## How to install

1. Back up your working V4.7 sketch. Make sure your ESP32-S3 is powered OFF before removing/inserting the SD card.
2. If your V4.7 icons already show on the screen, **keep your microSD as it is**. Otherwise copy `01_COPY_TO_SD_CARD/icons` to the **root** of your FAT32 microSD so you have `/icons/weather.rgb565`, etc.
3. Open `02_ARDUINO_CODE/ERINX_Smart_Hub_V4_8/ERINX_Smart_Hub_V4_8.ino` in Arduino IDE.
4. If you use live weather/time, edit `WIFI_SSID` and `WIFI_PASSWORD` near the top of the new sketch. These are blank in this package; credentials from your older sketch do not automatically carry over.
5. Do not change the working TFT_eSPI library's `User_Setup.h` or any TFT/touch wiring.
6. Arduino IDE: **ESP32S3 Dev Module**, **16MB flash**, **OPI PSRAM**, USB-UART port. Verify then Upload.
7. Serial Monitor: **115200 baud**. Verify `SD READY` and optionally weather Wi-Fi status.
8. Test Home robot eyes, Wi-Fi connected/disconnected icon, Clock eyes/time/date, Weather changing animation, BACK button, DHT11, Gallery/Video file browser, Settings.

## Libraries

- TFT_eSPI (Bodmer), keep working `User_Setup.h`
- ArduinoJson 7
- DHT sensor library (Adafruit) and Adafruit Unified Sensor
- ESP32 board package includes WiFi, HTTPClient, SD_MMC, FS, Preferences

## Important limitations

- Weather is from Open-Meteo; there is no network forecast without Wi-Fi. The sketch uses TLS with `setInsecure()` solely for the existing prototype weather endpoint; a production project should use certificate verification.
- Gallery and Video still browse filenames; JPEG rendering and MJPEG playback are for later versions, **not implemented in V4.8**.
- This code passed a local C++ syntax check against test stubs but has **not** been compiled against the exact ESP32-S3 Arduino toolchain or tested on your physical TFT. Keep your last working V4.7 ZIP as a fallback.

## Files

```
01_COPY_TO_SD_CARD/icons/        RGB565 40x40 existing icons (optional recopy)
02_ARDUINO_CODE/ERINX_Smart_Hub_V4_8/ERINX_Smart_Hub_V4_8.ino
README_FIRST.md
CHANGELOG.md
```
