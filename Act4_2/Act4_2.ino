//Carl Jayson Eli Bonaobra
//CPE4A
// ===== Activity 4.2: Volume Control Using OLED Display and Potentiometer =====

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int potPin = 34;

int lastVolume = -1; // track changes to avoid unnecessary redraws

void setup() {
  Serial.begin(115200);
  pinMode(potPin, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Volume Control Ready");
  display.display();
  delay(1000);
}

void loop() {
  int potValue = analogRead(potPin);                 // 0 - 4095
  int volume = map(potValue, 0, 4095, 0, 100);        // scale to 0 - 100%

  if (volume != lastVolume) {
    drawVolumeScreen(volume);
    Serial.print("Volume: ");
    Serial.print(volume);
    Serial.println("%");
    lastVolume = volume;
  }

  delay(50);
}

void drawVolumeScreen(int volume) {
  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("VOLUME");

  // Big percentage number
  display.setTextSize(3);
  display.setCursor(30, 15);
  display.print(volume);
  display.println("%");

  // Bar graph outline
  int barX = 10;
  int barY = 48;
  int barWidth = 108;
  int barHeight = 12;
  display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

  // Filled portion representing volume level
  int fillWidth = map(volume, 0, 100, 0, barWidth - 4);
  display.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, SSD1306_WHITE);

  display.display();
}