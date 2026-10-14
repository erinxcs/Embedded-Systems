# Changelog

This file summarizes the preserved development releases. Hardware verification status is documented separately in `docs/TESTING_CHECKLIST.md`.

## V5.2 — Wireless File Manager

- Added a responsive browser interface served directly by the ESP32-S3.
- Added PIN-gated photo and EVM upload, listing, and deletion on the microSD card.
- Added temporary upload staging and cleanup for interrupted transfers.
- Added desktop and mobile web UI previews.
- Retained the V5.1.3 touchscreen, weather, clock, DHT11, gallery, video, and Wi-Fi features.

## V5.1.3 — UI Controls

- Added visible-by-default Wi-Fi password entry with a SHOW/HIDE control.
- Replaced text-like city selector symbols with drawn chevrons.
- Preserved the V5.1.2 media, weather, and SD features.

## V5.1.2 — SDMMC Diagnostics

- Added ESP32-S3 SD_MMC target and header checks.
- Added a dedicated SDMMC board probe sketch.

## V5.1.1 — Compile Fix

- Added `PageTypes.h` beside the sketch to avoid Arduino auto-prototype parsing errors.

## V5.1 — Network and Location

- Added four-location weather selection.
- Added on-device Wi-Fi scan/connect and Preferences storage.

## V5.0 — Video Player

- Added silent custom EVM video playback.

## V4.3-V4.9

- Migrated the project to ESP32-S3.
- Added UI optimizations, RGB565 icon assets, and a JPEG photo gallery/slideshow.

## Earlier releases

- Built the ESP32D display, touch, Open-Meteo weather, and V2/V3 dashboard prototypes.

Historical release notes are preserved under `docs/original_notes/`.
