# Troubleshooting this ESP32 TFT project

## 1. White illuminated display

A white backlit TFT typically means the backlight is on but the LCD controller is not being initialized or written correctly. Verify LCD power rating and wiring, `ILI9488_DRIVER` being the **only enabled driver**, LCD SPI lines (CS=5, DC=27, RST=33, MOSI=23, CLK=18), and 20 MHz SPI clock. Run Arduino IDE: `File > Examples > TFT_eSPI > Test and diagnostics > Read_User_Setup`. Check driver/pins in Serial Monitor at 115200.

## 2. Shapes/colors but no text

Enable `LOAD_GLCD`, `LOAD_FONT2`, `LOAD_FONT4` (and other fonts if needed) in the active `User_Setup.h`. Keep exactly one active set of TFT pin definitions, and make sure the IDE isn't using another installation of TFT_eSPI. Version 01 includes visible text and shapes for diagnosis. Font configuration changes require recompilation and upload.

## 3. Touch doesn't respond or calibration repeats

Double-check T_CLK=18, T_DIN=23, T_DO=19, T_CS=21, plus shared GND. Disconnect LCD SDO(MISO), leave T_IRQ empty. Run version 02, press each calibration target and release. For inverted portrait orientation, set rotation 0 instead of 2 **and then recalibrate**. Calibration is saved under Preferences namespace `smarthub` and reused by V2/V3.

## 4. Time loads, weather doesn't

NTP time and HTTPS weather use different services. `WiFi connected` only confirms the access point association; it does **not** prove the Open-Meteo server is accessible or that JSON parsed. Use Serial Monitor at 115200 and check:

- HTTP status 200: request succeeded; check JSON fields.
- HTTP status 400: request query/parameters invalid.
- HTTP status 429: request is rate-limited.
- Negative HTTP code: connection, TLS, or timeout problem.
- JSON parse error: invalid/incomplete HTTP body or insufficient memory.

The V2 and V3 sketches print detailed HTTPS/JSON errors. Confirm selected location coordinates are correct (e.g. Tanauan center approximately 14.0853, 121.1528). If weather unavailable, retry after connectivity is restored; the sketches use periodic retries.

## 5. UI freezes briefly while tapping REFRESH

Current prototype calls HTTP synchronously, with network timeouts of roughly 12 seconds. Network waits are blocking. This does not prove touch wiring is faulty. Later versions should use a task or staged asynchronous requests to keep input responsive.

## 6. Fonts/text upside down

Use `tft.setRotation(2)` for the currently demonstrated mounting; change to `0` for inverted physical orientation. Remember touch calibration depends on rotation.

## 7. Images and RGB gradients

The PNG in `assets/mockups/` is a reference concept. V3 recreates it with draw primitives and small sprites; it is not rendering the PNG at full resolution. No SD or filesystem assets are needed.

## 8. DHT11 sensor

There is **no sensor reading** in versions 01–05. Internet weather is *outdoor*; DHT11 will measure *local indoor* conditions when added in a future V4. Do not assume the two temperatures must match.
