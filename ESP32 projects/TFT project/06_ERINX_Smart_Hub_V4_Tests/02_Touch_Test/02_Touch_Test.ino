// ERINX SMART HUB V4.2 - TOUCHSCREEN CALIBRATION + TEST
// TFT_eSPI touch support expects an XPT2046-compatible resistive panel.
#include <TFT_eSPI.h>

TFT_eSPI tft;
uint16_t calData[5];

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ERINX V4.2: TOUCH TEST ===");

  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  tft.init();
  tft.setRotation(1); // Must match calibration orientation

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("TOUCH CALIBRATION", tft.width() / 2, 80, 4);
  tft.drawCentreString("Touch the corner markers", tft.width() / 2, 135, 2);
  delay(1200);

  // Follow each magenta corner marker on the display.
  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
  tft.setTouch(calData);

  Serial.printf("uint16_t calData[5] = {%u, %u, %u, %u, %u};\n",
    calData[0], calData[1], calData[2], calData[3], calData[4]);
  Serial.println("Touch calibration complete. Touch/draw on the display.");
  Serial.println("Save calData[5] values for ERINX V4 main firmware.");

  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_CYAN);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("ERINX V4.2 TOUCH TEST", tft.width() / 2, 10, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Touch anywhere to draw", tft.width() / 2, 34, 2);
}

void loop() {
  uint16_t x = 0, y = 0;
  if (tft.getTouch(&x, &y)) {
    if (x >= tft.width()) x = tft.width() - 1;
    if (y >= tft.height()) y = tft.height() - 1;
    tft.fillCircle(x, y, 4, TFT_YELLOW);
    tft.fillRect(6, 57, tft.width() - 12, 24, TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Touch X: " + String(x) + "  Y: " + String(y), 10, 59, 2);
    Serial.printf("TOUCH X=%u Y=%u\n", x, y);
    delay(45);
  }
}
