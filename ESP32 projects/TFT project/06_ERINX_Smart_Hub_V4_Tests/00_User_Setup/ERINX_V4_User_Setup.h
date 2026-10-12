#pragma once
// ERINX SMART HUB V4 - ESP32-S3 N16R8 - 3.5" 480x320 SPI TFT
// Copy these definitions into the ACTIVE TFT_eSPI/User_Setup.h.
// This assumes an ILI9488 screen and XPT2046-compatible touch controller.
#define USER_SETUP_INFO "ERINX_V4_ESP32S3"
#define ILI9488_DRIVER
#define TFT_MISO 1   // Touch T_DO; TFT SDO may remain disconnected
#define TFT_MOSI 21  // TFT SDI and TOUCH T_DIN
#define TFT_SCLK 47  // TFT SCK and TOUCH T_CLK
#define TFT_CS 42
#define TFT_DC 41
#define TFT_RST 48
#define TOUCH_CS 2
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 16000000
#define SPI_TOUCH_FREQUENCY 2500000
#define SUPPORT_TRANSACTIONS
