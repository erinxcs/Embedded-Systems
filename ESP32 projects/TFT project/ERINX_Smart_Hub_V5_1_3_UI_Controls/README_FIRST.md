# ERINX SMART HUB V5.1.3 — Wi-Fi Password + City Dropdown UX

This is a targeted UI update built from V5.1.2. It retains all currently implemented Gallery, Video, Weather, DHT11, SD, Settings and touchscreen functionality.

## What changed

- In Settings → Wi-Fi Setup → select network, the password field **shows what you type**.
- Tap the outlined **eye / HIDE** control on the right side of the field to mask the password, and tap **SHOW** to reveal it again.
- For passwords longer than 32 displayed characters, the field shows the most recent characters (prefixed by `..`), while the full typed password is preserved.
- The Weather location selector now uses a **drawn chevron-down (⌄)** when closed and **chevron-up (⌃)** when the city list is expanded. The 4 locations and touch areas are unchanged.
- Input is not written to the Serial Monitor. Credentials are saved to on-device Preferences only after connection succeeds, as before.

## Install

1. Extract the ZIP.
2. Open the `ERINX_Smart_Hub_V5_1_3_UI_Controls` folder in Arduino IDE by opening its matching `.ino` file.
3. **Keep `PageTypes.h` beside the `.ino`.** This is required to avoid Arduino auto-prototype errors.
4. Board: **ESP32S3 Dev Module**; Flash: **16MB**; PSRAM: **OPI PSRAM**; use the USB-UART serial port.
5. Keep your working TFT_eSPI `User_Setup.h`, GPIO wiring, and existing microSD files. No extra assets are required.
6. Verify, then Upload. Test Wi-Fi password entry and Weather dropdown.

## Important

- The eye option intentionally defaults to **visible while typing**, as requested. Hide it before showing the screen to anyone else.
- This update has a **local source-integrity check**, but not a confirmed Arduino IDE / ESP32 3.3.12 full compile or device test here. If you get an error, send the **first error line** in the Arduino compiler output, not only the last `exit status 1`.
- The earlier SD_MMC board diagnostic remains relevant: if `SD_MMC` is missing at compile time, verify Tools → Board is **ESP32S3 Dev Module**, not a regular ESP32 board.
