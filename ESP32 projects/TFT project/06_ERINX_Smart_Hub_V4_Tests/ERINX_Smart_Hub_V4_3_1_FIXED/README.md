# ERINX Smart Hub V4.3.1 — Interactive Home Screen (Compile Fix)

## What is implemented

- Working full-screen 320×480 portrait home screen, designed for ESP32-S3 N16R8 and a 3.5-inch SPI ILI9488 TFT
- 6 functional touch-menu navigation pages: WEATHER, CLOCK, DHT11, GALLERY, VIDEO, SETTINGS
- Touch calibration on first boot; stores settings in ESP32 Preferences (NVS). Recalibrate via Settings
- Built-in SD card initialized using SD_MMC 1-bit mode; detects size and browses JPG/JPEG and MJPG/MJPEG filenames (no playback yet)
- Clock supports optional Wi-Fi NTP for actual Philippine time; shows `--:--:--` and uptime without Wi-Fi credentials
- Serial Monitor prints touch coordinates and current page

### Not yet implemented

- Open-Meteo weather data
- DHT11 environmental readings (not yet wired)
- Actual JPEG drawing and MJPEG video playback/audio

These screens clearly show placeholder messages, not fake sensor readings.

## Steps to run

1. Keep your **already-working** `TFT_eSPI/User_Setup.h` unchanged. It should select ILI9488_DRIVER, USE_HSPI_PORT, TFT_MOSI=21, TFT_SCLK=47, TFT_MISO=1, TFT_CS=42, TFT_DC=41, TFT_RST=48, TOUCH_CS=2, and LOAD_GLCD.
2. In Arduino IDE, select board **ESP32S3 Dev Module**, Flash **16MB**, PSRAM **OPI PSRAM**. Using USB-UART? USB CDC On Boot = Disabled. Select the right COM port.
3. Open `ERINX_Smart_Hub_V4_3_1/ERINX_Smart_Hub_V4_3_1.ino`.
4. Optional: enter `WIFI_SSID` and `WIFI_PASSWORD` near top of file if you want an NTP-synchronized clock. If left blank, the program works offline.
5. Insert your working FAT32 microSD. Upload. Open Serial Monitor at 115200 baud.
6. On first run, press the four calibration points. The calibration is saved for future boot-ups.
7. Tap one of the six tiles. Use `< BACK` for Home. The Settings page has `CALIBRATE TOUCH` if needed.

## Wiring (unchanged from V4.2 that worked)

| TFT terminal | ESP32-S3 CAM |
|---|---|
| VCC | 5V only if the display module explicitly supports 5V power |
| GND | GND |
| CS | GPIO42 |
| RESET/RST | GPIO48 |
| DC/RS | GPIO41 |
| SDI/MOSI | GPIO21 |
| SCK | GPIO47 |
| LED | 3V3 only if the module's LED input is compatible |
| T_CLK | GPIO47 |
| T_CS | GPIO2 |
| T_DIN | GPIO21 |
| T_DO | GPIO1 |
| T_IRQ | no connection |

The **ESP32-S3's onboard SD slot** uses internally connected GPIO39 (CLK), GPIO38 (CMD), GPIO40 (D0). Do not wire the TFT's separate SD slot.

## Files on SD card

The GALLERY page can list `.jpg` / `.jpeg` files in the root and `/photos` folder.
The VIDEO page can list `.mjpg` / `.mjpeg` files in the root and `/videos` folder. **File listing is not playback.** The code scans up to 150 directory entries per folder, showing the first five names.

Example:

```
SD card /
├── photos/
│   ├── photo1.jpg
│   └── photo2.jpeg
└── videos/
    └── demo.mjpg
```

## Troubleshooting

- **White or gray display:** power down, re-seat the same wiring that already worked in V4.2 and check your TFT_eSPI User_Setup.h configuration. Do not change the pin mapping at the same time as code.
- **No visible text:** ensure `#define LOAD_GLCD` is enabled in the correct active TFT_eSPI setup.
- **Touch offset:** Settings > CALIBRATE TOUCH. It stores calibration for portrait mode 0.
- **No SD:** FAT32, power off before reseating; re-run your previously passing stand-alone SD read/write test.
- **Clock --:--:--:** Wi-Fi credentials are blank or time hasn't synced; uptime still works.
- **DHT11/weather/gallery/video incomplete:** by design for V4.3, staged integration follows.

No additional libraries are required beyond **TFT_eSPI** and the standard ESP32 Arduino core libraries.

## V4.3.1 compile fix

This version fixes the Arduino IDE errors:
- `MediaSummary has not been declared`: the `scanDirectory` function signature now uses standard Arduino types only, avoiding the Arduino auto-prototype limitation.
- `File was not declared`: explicitly uses `fs::File` for compatibility with ESP32 Arduino Core 3.x.

**No wiring changes and no changes to the working TFT_eSPI User_Setup.h are needed.**

If using Arduino IDE, open the `.ino` **from inside its matching sketch folder** so the IDE can open/save the project normally.

Note: this corrected source was structurally checked, but hardware compilation should be confirmed by pressing Verify in your installed Arduino IDE; an Arduino ESP32 toolchain is not available in this environment.
