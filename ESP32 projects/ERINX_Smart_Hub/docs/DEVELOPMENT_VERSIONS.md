# Development Versions and Source File Index

**Primary final build:** V5.1.3 (`firmware/final/`). Historical firmware is retained independently, not mixed into the final build.

## Sketch index

|Type|File|
|---|---|
|development|`firmware/development/ESP32D/03_Weather_NoTouch_OpenMeteo/03_Weather_NoTouch_OpenMeteo.ino`|
|development|`firmware/development/ESP32D/04_Weather_Touch_V2/04_Weather_Touch_V2.ino`|
|development|`firmware/development/ESP32D/05_ERINX_Smart_Hub_V3/05_ERINX_Smart_Hub_V3.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V4_3/ERINX_Smart_Hub_V4_3.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V4_3_1/ERINX_Smart_Hub_V4_3_1.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V4_4/ERINX_Smart_Hub_V4_4.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V4_5/ERINX_Smart_Hub_V4_5.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V4_6_Icons/ERINX_Smart_Hub_V4_6_Icons.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V4_7_BlueWhite/ERINX_Smart_Hub_V4_7_BlueWhite.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V4_8/ERINX_Smart_Hub_V4_8.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V4_9/ERINX_Smart_Hub_V4_9.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V5_0/ERINX_Smart_Hub_V5_0.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V5_1_1_Network_Location/ERINX_Smart_Hub_V5_1_1_Network_Location.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V5_1_2_Network_Location/ERINX_Smart_Hub_V5_1_2_Network_Location.ino`|
|development|`firmware/development/ESP32S3/ERINX_Smart_Hub_V5_1_Network_Location/ERINX_Smart_Hub_V5_1_Network_Location.ino`|
|diagnostics|`firmware/diagnostics/ESP32D/01_Display_Loading_Test/01_Display_Loading_Test.ino`|
|diagnostics|`firmware/diagnostics/ESP32D/02_Touch_Test/02_Touch_Test.ino`|
|diagnostics|`firmware/diagnostics/ESP32S3/01_Display_Test/01_Display_Test.ino`|
|diagnostics|`firmware/diagnostics/ESP32S3/02_Touch_Test/02_Touch_Test.ino`|
|diagnostics|`firmware/diagnostics/ESP32S3/ERINX_V5_1_FPS_Benchmark/ERINX_V5_1_FPS_Benchmark.ino`|
|diagnostics|`firmware/diagnostics/ESP32S3/SDMMC_Board_Probe/SDMMC_Board_Probe.ino`|
|final|`firmware/final/ERINX_Smart_Hub_V5_1_3_UI_Controls/ERINX_Smart_Hub_V5_1_3_UI_Controls.ino`|

## Version chronology (source/archive interpretation)

- 01 Display/Loading and 02 Touch Test — original ESP32D bring-up and calibration.
- 03 No-touch Weather, 04 touch Weather V2 and 05 Smart Hub V3 — ESP32D dashboard evolution.
- V4.3–V4.5 — ESP32-S3 transition and UI refresh/optimization.
- V4.6–V4.8 — icons, blue/white theme, animated eyes and weather visuals.
- V4.9 — microSD JPEG photo gallery and slideshow.
- V5.0 — custom silent EVM video player.
- V5.1 / V5.1.1 — on-device Wi-Fi selection, location choice and PageTypes.h build fix.
- V5.1.2 — ESP32-S3 / SDMMC diagnostic compile-time guards.
- **V5.1.3 — Wi-Fi password visibility toggle and drawn dropdown chevron.**

Original release notes are in `docs/original_notes/` and technical traces are in `docs/SOURCE_FILE_MANIFEST.csv`.
