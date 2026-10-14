# ERINX Smart Hub — ESP32 + 3.5-inch TFT archive

An organized, versioned Arduino project for an **original ESP32 DevKit (ESP32D)** and **3.5-inch ILI9488 SPI 480x320 TFT with XPT2046 touchscreen**. The UI is drawn in **portrait 320x480**.

Each numbered folder is an **independent Arduino sketch**. Open the `.ino` inside its identically named folder, install the libraries, then compile and upload one version at a time.

## Versions, in order

| Folder | What it demonstrates | Wi-Fi | Touch |
|---|---|:---:|:---:|
| `01_Display_Loading_Test/` | Startup loading progress, font/color test, moving dot animation. TFT alone. | No | No |
| `02_Touch_Test/` | Calibration, live X/Y taps, test canvas, recalibration button. Saves calibration for V2 and V3. | No | Yes |
| `03_Weather_NoTouch_OpenMeteo/` | Open-Meteo Tanauan weather, Philippines NTP clock, animated weather icon. | Yes | No |
| `04_Weather_Touch_V2/` | Original functional but basic touch weather dashboard, location selection, refresh/settings. | Yes | Yes |
| `05_ERINX_Smart_Hub_V3/` | Redesigned blue ERINX dashboard with weather card, clock sprite, menu and icons. | Yes | Yes |

## Hardware / wiring

See [`docs/WIRING.md`](docs/WIRING.md). One ESP32D and one TFT module are enough for every included version. Do not connect any additional sensor for V1-V3.

**TFT power/backlight:** use the exact VCC and LED wiring already verified to work for your physical board. The silk-screen labels alone don't prove voltage tolerance. Do not unplug/replug wires when powered.

## Arduino IDE setup

1. Install ESP32 by Espressif Systems in **Boards Manager**.
2. Install **TFT_eSPI by Bodmer** in Library Manager for all five sketches.
3. Install **ArduinoJson by Benoit Blanchon, v7** for versions 03, 04 and 05.
4. Back up your active `TFT_eSPI/User_Setup.h`. Replace it with the contents of [`config/User_Setup_ESP32_ILI9488.h`](config/User_Setup_ESP32_ILI9488.h). Confirm `User_Setup_Select.h` loads `User_Setup.h` (standard library setting). Do **not** paste the new config after an existing setup; don't enable multiple drivers/pin definitions.
5. Choose the appropriate **ESP32 Dev Module** (or exact ESP32 DevKit model) and COM port; upload a sketch.
6. Use **115200 baud** in Serial Monitor to observe API/connectivity and touch coordinates.

### Wi-Fi projects (03–05)

Enter Wi-Fi credentials **only on your own computer** in the sketch:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

The ZIP contains **no Wi-Fi passwords**. If you plan to publish a version on GitHub, don't commit the locally edited file with credentials. Use local ignored secrets or restore placeholders before each commit.

Weather is fetched from the **Open-Meteo forecast API**, no API key required for the basic prototype. The connection uses `WiFiClientSecure::setInsecure()` for educational prototyping; use certificate validation in a production device. Weather data and Philippine time are **independent network requests**; working NTP does not guarantee a successful HTTPS forecast.

## LCD orientation & touch calibration

Sketches use `tft.setRotation(2)` for the portrait position shown in earlier testing. If physically upside down, change to `0` and **recalibrate** touch. Version 02 provides a `RECALIBRATE TOUCH` button. Calibration is stored in ESP32 NVS (`Preferences`, namespace `smarthub`, keys `calOK`, `c0`…`c4`), and is reused by V2 and V3. Calibration can be re-run in Settings of V2/V3.

## Development notes

- V2 and V3 have been retained as separate generations: both provide location/weather/touch, but V3 upgrades layout, sprites, and UI icons.
- Weather readings from API represent **outdoor/forecast conditions**, not an indoor sensor reading.
- Current release is **V3**. Planned **V4** starts the optional DHT11 indoor temperature/humidity module; see [`docs/ROADMAP.md`](docs/ROADMAP.md).
- All code is Arduino C++. The reference mockup in `assets/mockups/` is for visual guidance and is **not** required on the TFT.
- These sketches are packaged and checked for structure; they have **not been compiled with the ESP32 board toolchain or tested on your physical hardware** in this archive build. If something fails, copy the exact Arduino compiler/Serial output for correction.

## Safety

- Power off before rewiring. Keep loose pins/wires from contacting one another.
- LCD `SDO(MISO)` stays **unconnected** because ILI9488 SDO may not release a shared SPI MISO line; touch `T_DO` alone uses ESP32 GPIO19.
- The TFT `LED` pin is backlight power/control **not a standalone addressable LED**. Version 01 animates the *displayed loading UI* rather than electrically flashing the backlight.
- If you have a bare DHT11 later, use 3.3 V-safe logic and suitable pull-up as required by your specific module.
