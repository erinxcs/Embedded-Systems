/*
  ERINX Smart Hub TFT_eSPI configuration

  INSTALLATION:
  Back up TFT_eSPI/User_Setup.h, then copy the contents of THIS file into it.
  Confirm User_Setup_Select.h includes User_Setup.h (default arrangement).
  DO NOT paste this at the end of an unchanged existing setup file:
  that would leave conflicting display drivers or duplicate pin definitions.
*/
#define USER_SETUP_INFO "ERINX_ESP32D_ILI9488_XPT2046"

// Exactly one display controller. SPI (not parallel).
#define ILI9488_DRIVER

// LCD (ESP32 DevKit / original ESP32)
#define TFT_MISO 19  // Used by TOUCH T_DO only; LCD SDO is disconnected.
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC   27
#define TFT_RST  33

// Touch XPT2046; it shares CLK and MOSI with LCD.
#define TOUCH_CS 21

// Font selection: bitmap fonts required for UI text
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       20000000
#define SPI_READ_FREQUENCY  16000000
#define SPI_TOUCH_FREQUENCY  2500000
