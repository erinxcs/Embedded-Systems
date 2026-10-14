# ERINX V5.1 FPS Benchmark Results

This document is the repeatable results log for `ERINX_V5_1_FPS_Benchmark.ino`. Replace `NOT TESTED` only with values printed by the sketch or directly observed during the same run.

## Test configuration

| Item | Value |
|---|---|
| Test date | NOT TESTED |
| Board | ESP32-S3 CAM N16R8 |
| Arduino ESP32 core | NOT TESTED |
| TFT controller | ILI9488, 320x480 SPI |
| TFT_eSPI version | NOT TESTED |
| TJpg_Decoder version | NOT TESTED |
| Flash / PSRAM | 16 MB / OPI PSRAM |
| microSD card | NOT TESTED |
| Power source | NOT TESTED |

## Measurements

Run each resolution twice. The sketch excludes the warm-up run and prints the values used below.

| SPI | Resolution | Run | Frames | Elapsed (s) | FPS | SD ms/frame | JPEG + screen ms | LCD push ms | Result / notes |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 10 MHz | 160x240 | 1 | — | — | — | — | — | — | NOT TESTED |
| 10 MHz | 160x240 | 2 | — | — | — | — | — | — | NOT TESTED |
| 10 MHz | 240x360 | 1 | — | — | — | — | — | — | NOT TESTED |
| 10 MHz | 240x360 | 2 | — | — | — | — | — | — | NOT TESTED |
| 10 MHz | 320x480 | 1 | — | — | — | — | — | — | NOT TESTED |
| 10 MHz | 320x480 | 2 | — | — | — | — | — | — | NOT TESTED |
| 20 MHz | 160x240 | 1 | — | — | — | — | — | — | NOT TESTED |
| 20 MHz | 160x240 | 2 | — | — | — | — | — | — | NOT TESTED |
| 20 MHz | 240x360 | 1 | — | — | — | — | — | — | NOT TESTED |
| 20 MHz | 240x360 | 2 | — | — | — | — | — | — | NOT TESTED |
| 20 MHz | 320x480 | 1 | — | — | — | — | — | — | NOT TESTED |
| 20 MHz | 320x480 | 2 | — | — | — | — | — | — | NOT TESTED |
| 27 MHz | 160x240 | 1 | — | — | — | — | — | — | NOT TESTED |
| 27 MHz | 160x240 | 2 | — | — | — | — | — | — | NOT TESTED |
| 27 MHz | 240x360 | 1 | — | — | — | — | — | — | NOT TESTED |
| 27 MHz | 240x360 | 2 | — | — | — | — | — | — | NOT TESTED |
| 27 MHz | 320x480 | 1 | — | — | — | — | — | — | NOT TESTED |
| 27 MHz | 320x480 | 2 | — | — | — | — | — | — | NOT TESTED |

## Stability checks

For every SPI frequency, record whether the following were observed:

- display corruption, color errors, or flicker;
- JPEG decode errors or incomplete frames;
- ESP32 resets, watchdog warnings, or PSRAM errors;
- SD read failures;
- abnormal TFT or regulator temperature.

Do not test 40 MHz until 27 MHz is stable. Restore the last stable TFT_eSPI configuration after testing.

## Summary

| Metric | Best verified result |
|---|---|
| Highest stable SPI frequency | NOT TESTED |
| Best 160x240 FPS | NOT TESTED |
| Best 240x360 FPS | NOT TESTED |
| Best 320x480 FPS | NOT TESTED |
| Recommended playback profile | NOT TESTED |

Attach the relevant Serial Monitor output or link it from this file when publishing results.
