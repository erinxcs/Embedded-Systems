# ERINX SMART HUB V4 — ESP32-S3 TFT and Touch Tests

## Hardware
- ESP32-S3 CAM N16R8; built-in SD slot is already tested and working.
- 3.5-inch SPI 480x320 TFT (assumed ILI9488) with XPT2046-style touch.
- SD-card reader in the display does NOT need wiring.

## Display wiring
| TFT | ESP32-S3 |
|---|---|
| VCC | 5V only if TFT module permits 3.3-5V input |
| GND | GND |
| CS | GPIO42 |
| RESET/RST | GPIO48 |
| DC/RS | GPIO41 |
| SDI/MOSI | GPIO21 |
| SCK | GPIO47 |
| LED | 3V3 only if compatible with board's backlight circuitry |
| SDO/MISO | Not connected |

## Touch wiring
| TFT touch | ESP32-S3 |
|---|---|
| T_CLK | GPIO47 |
| T_CS | GPIO2 |
| T_DIN | GPIO21 |
| T_DO | GPIO1 |
| T_IRQ | Not connected |

## Preparation
1. Turn power OFF before adjusting jumper wires.
2. Arduino IDE: choose **ESP32S3 Dev Module** (not ESP32 Dev Module); select the serial port.
3. Install **TFT_eSPI by Bodmer** via Library Manager.
4. Back up the existing `TFT_eSPI/User_Setup.h` from your V3 setup.
5. Use `00_User_Setup/ERINX_V4_User_Setup.h` settings as the ACTIVE User_Setup.h. Ensure `User_Setup_Select.h` includes it, not some other setup. Do not leave duplicate active driver/pin definitions.
6. Driver `ILI9488_DRIVER` is assumed. If your original working V3 was configured with a different display driver, use the proven driver instead.
7. Connect board's USB-UART port; Serial Monitor baud 115200.

## Tests
1. Open `01_Display_Test/01_Display_Test.ino`, upload. Expected: red, green, blue, white, black sequence, followed by ERINX SMART HUB confirmation screen and uptime.
2. Only after display works, open `02_Touch_Test/02_Touch_Test.ino`, upload. Touch the corner markers when prompted. Then touching the screen should draw yellow dots and print coordinates on Serial Monitor.
3. Copy the five printed `calData` numbers to preserve touch calibration for final firmware at the same display rotation.

## If it doesn't work
- Backlight dark: check power/backlight voltage ratings, GND, and TFT VCC/LED connections.
- White/lit blank screen: verify active TFT_eSPI setup, correct driver, CS/DC/RST/MOSI/SCLK pin mapping.
- Display works but touch doesn't: check T_DO to GPIO1, T_CS to GPIO2, shared T_DIN and T_CLK, then repeat calibration.
- The display pins `SD_CS`, `SD_MOSI`, `SD_MISO`, `SD_SCK` are intentionally unused because microSD resides on the ESP32-S3 CAM itself.
