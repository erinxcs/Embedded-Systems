# Final firmware releases

## Recommended: V5.2

Open `ERINX_Smart_Hub_V5_2/ERINX_Smart_Hub_V5_2.ino` in Arduino IDE. Keep all four files in that sketch folder together:

- `ERINX_Smart_Hub_V5_2.ino`
- `FileManager.h`
- `FilePortal.h`
- `PageTypes.h`

V5.2 contains the local wireless media manager in addition to the touchscreen dashboard features. See [`../../docs/releases/V5.2.md`](../../docs/releases/V5.2.md) for setup and security notes.

## Previous stable UI: V5.1.3

`ERINX_Smart_Hub_V5_1_3_UI_Controls/` is retained as the last release before the browser-based file manager. It is useful for isolating web-server or SD concurrency problems.

Do not merge files from different release folders; each Arduino sketch should be compiled with its matching companion headers.
