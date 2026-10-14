# TFT Project Development Archive

This directory preserves the original ERINX Smart Hub experiments and release-style folders. It is kept for development history and reproducibility.

## Recommended starting point

Use the organized project at [`../ERINX_Smart_Hub/`](../ERINX_Smart_Hub/) for the current source, hardware setup, deduplicated SD-card assets, tools, and documentation.

| Need | Open this |
|---|---|
| Current V5.2 firmware | [`../ERINX_Smart_Hub/firmware/final/ERINX_Smart_Hub_V5_2/`](../ERINX_Smart_Hub/firmware/final/ERINX_Smart_Hub_V5_2/) |
| Main project guide | [`../ERINX_Smart_Hub/README.md`](../ERINX_Smart_Hub/README.md) |
| FPS benchmark | [`ERINX_V5_1_FPS_Benchmark/README.md`](ERINX_V5_1_FPS_Benchmark/README.md) |
| SDMMC diagnostic | [`ERINX_Smart_Hub_V5_1_2_SDMMC_DIAGNOSTIC/SDMMC_Board_Probe/`](ERINX_Smart_Hub_V5_1_2_SDMMC_DIAGNOSTIC/SDMMC_Board_Probe/) |
| Early wiring notes | [`docs/WIRING.md`](docs/WIRING.md) |

## Archive organization

- `01_` through `05_` contain the early ESP32D learning sequence.
- `06_ERINX_Smart_Hub_V4_Tests/` contains ESP32-S3 V4 experiments.
- `ERINX_Smart_Hub_V5_*` folders contain packaged V5 development releases.
- `ERINX_V5_1_FPS_Benchmark/` is a standalone performance diagnostic.
- `01_OPTIONAL_SD_FILES/`, `assets/`, and `config/` are historical shared resources.

Files here may intentionally repeat assets or source from another version so each historical package remains self-contained. Do not delete duplicates solely by filename; use the canonical project when you want the cleanest current layout.
