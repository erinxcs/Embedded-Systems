# ERINX Smart Hub V5.1.1 — Compile Fix

- Fixed Arduino IDE `showPage(Page page)` / `Page has not been declared` compilation issue by moving `enum Page` to `PageTypes.h`.
- Kept existing V5.1 on-screen Wi-Fi scan, password keyboard, location dropdown, weather, photo gallery, video player, clock, DHT11, touch calibration, and SD functionality.
- Same TFT wiring and `User_Setup.h`; no SD file changes needed.
