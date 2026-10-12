# 01_Display_Loading_Test

Basic boot/loading demo, animated dots and color/font blocks. No touch, Wi-Fi, or DHT11.

**Hardware:** ESP32D + existing ILI9488 TFT module; see [wiring](../docs/WIRING.md).

**Libraries:** TFT_eSPI only.

**Open:** `01_Display_Loading_Test.ino` inside this folder in Arduino IDE.

**Configuration:** Use [TFT_eSPI User_Setup](../config/User_Setup_ESP32_ILI9488.h).

**Orientation:** Portrait with `tft.setRotation(2)`; if upside-down, change to `0` and recalibrate touch for touch-enabled versions.

**Wi-Fi:** Versions 03, 04, and 05 need you to add your credentials locally. Do not commit passwords.

**Serial Monitor:** 115200 baud.
