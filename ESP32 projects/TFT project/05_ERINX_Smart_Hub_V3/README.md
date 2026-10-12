# 05_ERINX_Smart_Hub_V3

Updated ERINX blue portrait hub with time card, weather card, animated weather icon and touch navigation.

**Hardware:** ESP32D + existing ILI9488 TFT module; see [wiring](../docs/WIRING.md).

**Libraries:** TFT_eSPI, ArduinoJson v7, ESP32 core WiFi/HTTP/Preferences.

**Open:** `05_ERINX_Smart_Hub_V3.ino` inside this folder in Arduino IDE.

**Configuration:** Use [TFT_eSPI User_Setup](../config/User_Setup_ESP32_ILI9488.h).

**Orientation:** Portrait with `tft.setRotation(2)`; if upside-down, change to `0` and recalibrate touch for touch-enabled versions.

**Wi-Fi:** Versions 03, 04, and 05 need you to add your credentials locally. Do not commit passwords.

**Serial Monitor:** 115200 baud.
