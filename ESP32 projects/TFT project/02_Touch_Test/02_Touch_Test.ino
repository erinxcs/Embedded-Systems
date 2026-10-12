/*
  02 - TOUCH TEST + SAVED CALIBRATION
  ESP32 DevKit + 3.5-inch ILI9488 + onboard XPT2046 touch.
  GPIO: T_CLK=18 T_CS=21 T_DIN=23 T_DO=19; LCD SDO DISCONNECTED.
  The calibration keys (namespace "smarthub") are shared with V2/V3 so a
  successful calibration can be reused by later sketches.
*/
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Preferences.h>

TFT_eSPI tft = TFT_eSPI();
Preferences prefs;
uint16_t calibration[5] = {0};

const uint16_t BG = 0x010C;
const uint16_t CARD = 0x09D6;
const uint16_t ACCENT = 0x35DF;

void calibrateScreen();
void drawUI();
void printTouch(uint16_t x, uint16_t y);

void calibrateScreen() {
  tft.fillScreen(BG);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, BG);
  tft.drawString("TOUCH CALIBRATION", 20, 50, 2);
  tft.drawString("Press and release targets", 16, 90, 2);
  tft.calibrateTouch(calibration, ACCENT, BG, 15);
  tft.setTouch(calibration);
  for (int i = 0; i < 5; i++) {
    String key = String("c") + i;
    prefs.putUShort(key.c_str(), calibration[i]);
  }
  prefs.putBool("calOK", true);
  delay(200);
}

void drawUI() {
  tft.fillScreen(BG);
  tft.fillRoundRect(12, 15, 296, 77, 12, CARD);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, CARD);
  tft.drawString("TOUCH TEST", 160, 44, 4);
  tft.drawString("Tap anywhere below", 160, 76, 2);
  tft.drawRoundRect(14, 109, 292, 232, 10, ACCENT);
  tft.fillRoundRect(15, 395, 290, 63, 12, CARD);
  tft.setTextColor(TFT_WHITE, CARD);
  tft.drawString("RECALIBRATE TOUCH", 160, 427, 2);
  tft.setTextColor(TFT_WHITE, BG);
  tft.drawString("X: --    Y: --", 160, 368, 2);
  tft.setTextDatum(TL_DATUM);
}

void printTouch(uint16_t x, uint16_t y) {
  Serial.printf("TAP X:%u Y:%u\n", x, y);
  tft.fillRect(14, 345, 292, 44, BG);
  char buffer[40];
  snprintf(buffer, sizeof(buffer), "X: %u   Y: %u", x, y);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, BG);
  tft.drawString(buffer, 160, 367, 2);
  tft.setTextDatum(TL_DATUM);
  if (x >= 19 && x <= 299 && y >= 114 && y <= 337) {
    tft.fillCircle(x, y, 5, TFT_CYAN);
  }
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(2); // change to 0 if upside-down; recalibrate if changed
  prefs.begin("smarthub", false);
  if (prefs.getBool("calOK", false)) {
    for (int i = 0; i < 5; i++) {
      String key = String("c") + i;
      calibration[i] = prefs.getUShort(key.c_str(), 0);
    }
    tft.setTouch(calibration);
  } else {
    calibrateScreen();
  }
  drawUI();
  Serial.println("Tap screen. 115200 baud. Calibration saved in ESP32 NVS.");
}

void loop() {
  uint16_t x = 0, y = 0;
  static bool lastDown = false;
  static unsigned long lastPress = 0;
  bool down = tft.getTouch(&x, &y, 600);
  if (down && !lastDown && millis() - lastPress > 180) {
    lastPress = millis();
    if (x >= 15 && x <= 305 && y >= 395 && y <= 458) {
      calibrateScreen();
      drawUI();
    } else {
      printTouch(x, y);
    }
  }
  lastDown = down;
  delay(15);
}
