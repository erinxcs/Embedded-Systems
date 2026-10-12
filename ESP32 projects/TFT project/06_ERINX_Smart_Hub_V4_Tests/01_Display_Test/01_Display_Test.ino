// ERINX SMART HUB V4.1 - DISPLAY TEST
// ESP32-S3 N16R8, TFT_eSPI with ERINX_V4_User_Setup.h definitions.
#include <TFT_eSPI.h>

TFT_eSPI tft;

void colorTest(uint16_t color, uint16_t textColor, const char *label) {
  tft.fillScreen(color);
  tft.setTextColor(textColor, color);
  tft.drawCentreString(label, tft.width() / 2, tft.height() / 2 - 14, 4);
  Serial.printf("Displayed: %s\n", label);
  delay(750);
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("\n=== ERINX V4.1: TFT DISPLAY TEST ===");

  // Release touch device while TFT is being initialized.
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  tft.init();
  tft.setRotation(1); // landscape: expected 480 x 320
  Serial.printf("TFT dimensions: %d x %d\n", tft.width(), tft.height());

  colorTest(TFT_RED, TFT_WHITE, "RED");
  colorTest(TFT_GREEN, TFT_BLACK, "GREEN");
  colorTest(TFT_BLUE, TFT_WHITE, "BLUE");
  colorTest(TFT_WHITE, TFT_BLACK, "WHITE");
  colorTest(TFT_BLACK, TFT_WHITE, "BLACK");

  tft.fillScreen(TFT_BLACK);
  tft.drawRoundRect(9, 9, tft.width() - 18, tft.height() - 18, 14, TFT_CYAN);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("ERINX SMART HUB", tft.width() / 2, 48, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("V4.1 - TFT DISPLAY TEST", tft.width() / 2, 94, 2);

  tft.fillRoundRect(45, 142, tft.width() - 90, 94, 12, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawCentreString("DISPLAY ONLINE", tft.width() / 2, 157, 4);
  tft.drawCentreString("ESP32-S3 N16R8", tft.width() / 2, 205, 2);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("COLOR TEST FINISHED", tft.width() / 2, 251, 2);
  Serial.println("TFT test completed. If visible, proceed to touch test.");
}

void loop() {
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    tft.fillRect(115, 282, 250, 22, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawCentreString("Uptime: " + String(millis() / 1000) + " s",
                         tft.width() / 2, 282, 2);
  }
}
