/*
  01 - TFT DISPLAY / LOADING SCREEN TEST
  ESP32 DevKit + 3.5-inch ILI9488 TFT, NO TOUCH REQUIRED.
  TFT_eSPI must be configured using ../config/User_Setup_ESP32_ILI9488.h.

  This tests the LCD and its backlight being powered; the LED header on the
  TFT module is a BACKLIGHT power/control input, not a programmable pixel LED.
  Never move TFT power leads while USB is connected.
*/
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

const uint16_t BG = 0x010C;
const uint16_t PANEL = 0x09D6;
const uint16_t ACCENT = 0x35DF;
const uint16_t WHITE16 = 0xFFFF;
const uint16_t DIM = 0x7BCE;

void drawText(const char* msg, int x, int y, int font, uint16_t col, uint16_t background);
void bootAnimation();
void displayTest();

void drawText(const char* msg, int x, int y, int font, uint16_t col, uint16_t background) {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(col, background);
  tft.setTextSize(1);
  tft.drawString(msg, x, y, font);
  tft.setTextDatum(TL_DATUM);
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(2); // portrait upright on previously tested module; change to 0 if inverted
  tft.setTextWrap(false);
  Serial.printf("Display: %d x %d\n", tft.width(), tft.height());
  bootAnimation();
  displayTest();
}

void loop() {
  // Continuous, bounded animation. No external hardware required.
  static unsigned long lastFrame = 0;
  static int offset = 0;
  if (millis() - lastFrame >= 45) {
    lastFrame = millis();
    tft.fillRect(35, 345, 250, 64, PANEL);
    for (int i = 0; i < 5; i++) {
      uint16_t color = (i == offset) ? ACCENT : DIM;
      tft.fillCircle(70 + i * 45, 375, (i == offset) ? 10 : 6, color);
    }
    offset = (offset + 1) % 5;
  }
}

void bootAnimation() {
  tft.fillScreen(BG);
  tft.fillRoundRect(14, 112, 292, 266, 16, PANEL);
  drawText("ERINX", 160, 156, 4, WHITE16, PANEL);
  drawText("SMART HUB", 160, 192, 2, ACCENT, PANEL);
  drawText("ESP32 + ILI9488", 160, 240, 2, DIM, PANEL);
  tft.drawRoundRect(32, 279, 256, 25, 9, ACCENT);
  for (int pct = 0; pct <= 100; pct += 2) {
    int filled = map(pct, 0, 100, 0, 246);
    if (filled > 0) tft.fillRect(37, 284, filled, 15, ACCENT);
    char status[24];
    snprintf(status, sizeof(status), "LOADING %d%%", pct);
    tft.fillRect(61, 313, 198, 27, PANEL);
    drawText(status, 160, 325, 2, WHITE16, PANEL);
    delay(36);
  }
  delay(550);
}

void displayTest() {
  tft.fillScreen(BG);
  tft.fillRoundRect(12, 28, 296, 76, 12, PANEL);
  drawText("DISPLAY ONLINE", 160, 64, 4, ACCENT, PANEL);
  tft.fillRoundRect(12, 135, 296, 296, 14, PANEL);
  drawText("COLOR / GRAPHICS", 160, 166, 2, WHITE16, PANEL);
  uint16_t sampleColors[4] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW};
  for (int i = 0; i < 4; ++i) {
    tft.fillRoundRect(39 + i * 63, 211, 51, 75, 7, sampleColors[i]);
  }
  drawText("SMOOTH ANIMATION", 160, 322, 2, WHITE16, PANEL);
  drawText("No touch / Wi-Fi needed", 160, 443, 2, DIM, BG);
}
