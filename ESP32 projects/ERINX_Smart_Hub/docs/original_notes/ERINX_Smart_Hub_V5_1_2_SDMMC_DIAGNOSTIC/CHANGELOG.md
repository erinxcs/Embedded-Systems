# V5.1.2 — SDMMC diagnostic update

- Preserves all V5.1.1 functions including Wi-Fi scanning, location selector, photo and video pages.
- Adds early ESP32-S3 target and `SOC_SDMMC_HOST_SUPPORTED` compiler checks.
- Explicitly diagnoses `NO_GLOBAL_INSTANCES` / `NO_GLOBAL_SD_MMC` flags.
- Includes standalone SDMMC Board Probe sketch.
- Orders `<FS.h>` and `<SD_MMC.h>` before media/UI library includes.
- Does not change TFT pins, microSD wiring, photos, videos, or touch calibration.
