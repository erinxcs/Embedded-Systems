# 02_Touch_Test

Touch calibration, coordinate test and recalibration button. Saves calibration for V2/V3.

**Hardware:** ESP32D + existing ILI9488 TFT module; see [wiring](../docs/WIRING.md).

**Libraries:** TFT_eSPI and ESP32 core Preferences.

**Open:** `02_Touch_Test.ino` inside this folder in Arduino IDE.

**Configuration:** Use [TFT_eSPI User_Setup](../config/User_Setup_ESP32_ILI9488.h).

**Orientation:** Portrait with `tft.setRotation(2)`; if upside-down, change to `0` and recalibrate touch for touch-enabled versions.

**Wi-Fi:** Versions 03, 04, and 05 need you to add your credentials locally. Do not commit passwords.

**Serial Monitor:** 115200 baud.
