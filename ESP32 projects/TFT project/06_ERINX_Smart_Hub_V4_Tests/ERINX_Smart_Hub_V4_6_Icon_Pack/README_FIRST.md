# ERINX Smart Hub V4.6 — Portrait Icon Upgrade

You already have a working ESP32-S3 CAM N16R8, ILI9488 3.5-inch SPI portrait TFT, touch, and FAT32 SD card. This update **only upgrades the six menu tile icons**. All other V4.5 functions are retained.

## What's inside the ZIP

- `01_COPY_TO_SD_CARD/icons/` — six 40x40 .rgb565 icon files: the ACTUAL data the ESP32 reads.
- `02_PREVIEWS/` — 240px PNG previews and 320x480 dashboard mockup. **Do not copy these onto SD unless you want to keep them as previews.**
- `03_EDITABLE_SVG/` — original editable vector versions of the icons (not read on device).
- `04_ARDUINO_CODE/ERINX_Smart_Hub_V4_6_Icons/ERINX_Smart_Hub_V4_6_Icons.ino` — FULL integrated V4.5 sketch with SD icon loading added.

## Install to microSD — READ THIS FIRST

1. **Disconnect USB power from the ESP32-S3.** Remove the microSD card.
2. Insert the card into a PC using a card reader. Keep your existing FAT32 filesystem and other photos/videos; **do not format it again**.
3. Open the ZIP and copy **the `icons` folder INSIDE `01_COPY_TO_SD_CARD`** to the ROOT of the microSD card.
4. The correct SD layout will be:

```
SD CARD ROOT (e.g. E:/)
  icons/
    weather.rgb565
    clock.rgb565
    dht11.rgb565
    gallery.rgb565
    video.rgb565
    settings.rgb565
  photos/      (optional existing directory)
  videos/      (optional existing directory)
```

Do not copy `01_COPY_TO_SD_CARD` as a top-level folder, or the firmware will not find `/icons`.
5. Safely eject the card, insert it back into ESP32-S3, then reconnect USB.

## Update Arduino IDE

1. Extract `04_ARDUINO_CODE/ERINX_Smart_Hub_V4_6_Icons/` to your project folder on Windows.
2. Open `ERINX_Smart_Hub_V4_6_Icons.ino` with Arduino IDE. **Do not paste this over an existing V4.5 sketch unless you have backed it up.**
3. Copy your working `WIFI_SSID` and `WIFI_PASSWORD` into this **new** sketch if you need live weather and NTP. They start blank for privacy.
4. Keep your **working** TFT_eSPI `User_Setup.h` **unchanged**, including HSPI selection and font config. No wiring changes.
5. Board: `ESP32S3 Dev Module`, flash 16MB, `OPI PSRAM`, 115200 upload, USB-UART. Verify, then Upload.
6. Open Serial Monitor at **115200**, reset the device once. Expect messages similar to:

```
SD READY: 15193 MB
ICONS: 6/6 found in /icons (RGB565 40x40)
HOME SCREEN READY - tap any tile
```

Card capacity will differ. The screen should show colorful icons for Weather, Clock, DHT11, Gallery, Video, Settings.

## Troubleshooting

- `ICONS: 0/6`: check the folder location. Paths must start with `/icons/`, not `/01_COPY_TO_SD_CARD/icons/`.
- `SD unavailable`: check inserted card and FAT32; the built-in SDMMC has already passed your read/write test.
- Fallback letter icons instead of new icons: missing/misnamed files, wrong folder, or files with wrong sizes. Each `.rgb565` file must be exactly **3200 bytes**.
- Different color appearance: confirm you used your previous working `User_Setup.h`; do not edit driver parameters for the icon pack.
- If the previous V4.5 sketch is still running, the new icons will **not appear automatically**. You must upload the updated V4.6 sketch because V4.5 doesn't read image icons.
- Keep your previous V4.5 ZIP as backup.

## Technical notes

Icons are stored as uncompressed 40x40, little-endian RGB565 raw pixels, **no header, no transparency**; their background is already painted to match the existing tile. Each is 3200 bytes; all six are 19,200 bytes. The ESP32 reads **one icon at a time** into a shared 3.2kB buffer, then calls `tft.pushImage` to display it. This needs **no additional image library**, and the internal SDMMC uses GPIO38/39/40, unrelated to the TFT SPI wiring. JPG/PNG formats are not used by this version.

Gallery/photo/video playback is unchanged (still filename listings). We will work on Photo Frame mode in a later version **when requested**.

**Testing caveat:** Code has been checked locally for structure and syntax with stub libraries, but has not been uploaded to the user's real ESP32-S3 hardware. Report compiler/serial errors if any.
