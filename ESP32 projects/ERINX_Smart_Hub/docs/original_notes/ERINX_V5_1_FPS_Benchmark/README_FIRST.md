# ERINX SMART HUB V5.1 — ACTUAL VIDEO FPS BENCHMARK

This is a **separate diagnostic sketch**. It does not replace your working V5.0 Smart Hub program permanently. It does NOT touch or change SD photo/icon/video content (except adding an optional CSV log).

## Requirements

- ESP32-S3 CAM N16R8, 3.5-inch 320x480 ILI9488 SPI TFT; use EXACT same working pin wiring and `TFT_eSPI/User_Setup.h` as V5.0.
- Board: `ESP32S3 Dev Module`; Flash Size: `16MB`; Flash Mode: `QIO`; **Tools > PSRAM > OPI PSRAM**, THEN REUPLOAD.
- `TFT_eSPI` by Bodmer and `TJpg_Decoder` by Bodmer installed.
- FAT32 card inserted in the **ESP32-S3 built-in microSD** slot.

## Steps

1. Power off ESP32-S3, insert microSD card in PC.
2. Extract the `COPY_TO_SD_CARD/bench` folder to root of SD card; do not erase existing `/icons`, `/photos`, `/videos`.
3. Safely eject, put card back in S3, power on.
4. Open `ERINX_V5_1_FPS_Benchmark/ERINX_V5_1_FPS_Benchmark.ino` in Arduino IDE.
5. With **OPI PSRAM enabled**, Verify, Upload, and open Serial Monitor @ 115200.
6. Test auto-runs all 3 clips TWICE (warmup excluded). The display shows FPS results when finished. Paste Serial results back in chat.

## For SPI speed comparison

Edit the **one active** line in `Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`:

```
#define SPI_FREQUENCY 10000000
```

Test **10 MHz** first. Then (if fully stable) **20 MHz**, then **27000000** (27 MHz). **40 MHz is experimental**; only consider it if the previous results show stable picture, no resets, no color glitches. Each change needs Verify+Upload again. Restore last stable setting afterward. SPI clock used is a compiled TFT_eSPI setting, not a runtime UI control.

**Do not edit** `SPI_TOUCH_FREQUENCY`, wiring, `USE_HSPI_PORT`, or the SDMMC pins.

## Results meaning

- `uncapped_FPS`: frames fully read and drawn per second, with no artificial frame delay.
- `SD_ms_per_frame`: card read average including frame length.
- `JPEG_plus_screen_ms`: JPEG decode + pixel pushes, combined.
- `LCD_push_ms`: measured TFT pushImage callback time within JPEG decoder (a subset of JPEG+screen).
- Source header is marked at 15 FPS but **ignored** in benchmark: this measures throughput, not playback timing.
- Final V5.0 UI and animations can slow down actual application playback. Results are hardware throughput under this sketch.
- A CSV with results is appended to `/ERINX_FPS_RESULTS.csv` on SD. You may delete it any time.

## Test files

- `TEST_160x240.evm`
- `TEST_240x360.evm`
- `TEST_320x480.evm`

All three are the SAME 34-frame source clip, converted to each resolution, baseline JPEG. Their `*.evm` container is the same as V5.0. This makes comparison meaningful, although image content/compression complexity can affect FPS.

## If PSRAM fails

If you still get `PSRAM NOT DETECTED`, there is no FPS result yet. Double-check `Tools > PSRAM > OPI PSRAM`, then **reupload the benchmark**. If still zero, capture boot errors after reset with Core Debug Level `Verbose`.

## If the screen turns white or black

Restore the last stable `SPI_FREQUENCY` immediately. Wiring stability and TFT driver affect safe max clock speed.

## Notes

No need to alter microSD format again. This test is silent and not MP4 direct playback.
