# ESP32D + ILI9488 3.5-inch SPI TFT wiring

Power down the USB connection before changing wires. The board in the user's earlier photos is labeled **3.5\" TFT SPI 480X320 V1.0**, with fourteen labeled header pins.

| TFT header label | ESP32D GPIO / connection | Which versions |
|---|---|---|
| VCC | **Existing verified working supply** (do not guess voltage from label) | 01–05 |
| GND | GND | 01–05 |
| CS | GPIO 5 | 01–05 |
| RESET / RST | GPIO 33 | 01–05 |
| DC/RS | GPIO 27 | 01–05 |
| SDI (MOSI) | GPIO 23 | 01–05 |
| SCK | GPIO 18 | 01–05 |
| LED | **Existing verified working backlight connection** | 01–05 |
| SDO (MISO) | Not connected | 01–05 |
| T_CLK | GPIO 18 | 02, 04, 05 |
| T_CS | GPIO 21 | 02, 04, 05 |
| T_DIN | GPIO 23 | 02, 04, 05 |
| T_DO | GPIO 19 | 02, 04, 05 |
| T_IRQ | Not connected | 01–05 |

### Why do the touch pins share GPIOs?

SPI devices share clock and MOSI lines. `T_CLK` shares GPIO18 with `SCK`; `T_DIN` shares GPIO23 with LCD `SDI`. Touch controller has its own chip-select GPIO21 while LCD uses GPIO5.

The TFT_eSPI configuration has `TFT_MISO 19` but **LCD SDO is physically disconnected**. The input at ESP32 GPIO19 is from touch `T_DO` only.

### Touch calibration and screen direction

`rotation(2)` is portrait; `rotation(0)` flips it 180 degrees. Recalibrate whenever the screen orientation changes. Start from version `02_Touch_Test` to establish/store touch calibration.

### No DHT11 yet

No extra hardware beyond TFT + ESP32D is used in the included versions. For V4, a DHT11 may be connected to a spare pin (e.g., GPIO32, subject to your exact board); do not connect it yet.
