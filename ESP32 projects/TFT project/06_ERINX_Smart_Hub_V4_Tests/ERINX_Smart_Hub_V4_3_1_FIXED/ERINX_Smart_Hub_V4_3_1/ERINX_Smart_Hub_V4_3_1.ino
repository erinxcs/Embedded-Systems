/*
  ERINX SMART HUB V4.3.1 - INTERACTIVE HOME SCREEN (COMPILATION FIX)
  Hardware: ESP32-S3 CAM N16R8 + 3.5-inch 480x320 SPI TFT (portrait 320x480)
  Display library: TFT_eSPI (keep your already-working User_Setup.h)

  What works in V4.3:
    - Six touch-operated pages and BACK navigation
    - On-device touch calibration, saved in ESP32 NVS
    - Built-in SDMMC card detection and JPEG/MJPEG filename browser
    - Clock: optional Wi-Fi/NTP, otherwise an honest unsynced state + uptime
    - Live last-touch X/Y coordinates at bottom of home screen

  What is NOT implemented yet:
    - Weather API, DHT11 readings, JPEG rendering, and MJPEG playback.
  Those have placeholder pages; no fake readings are shown.

  IMPORTANT: This uses the same TFT wiring that passed your TFT_eSPI test.
  The built-in SD card is already wired on the ESP32-S3 CAM motherboard.
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD_MMC.h>
#include <FS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>

// Optional: enter your Wi-Fi details to enable real Philippine time.
// Leave WIFI_SSID empty to skip Wi-Fi until the weather module is ready.
const char WIFI_SSID[] = "";
const char WIFI_PASSWORD[] = "";

// ESP32-S3 CAM built-in SDMMC (1-bit mode). No external SD wires.
static const int SD_CLK_PIN = 39;
static const int SD_CMD_PIN = 38;
static const int SD_D0_PIN  = 40;

static const int SCREEN_W = 320;
static const int SCREEN_H = 480;
static const int FOOTER_Y = 450;

// RGB565 palette for 3.5-inch TFT
static const uint16_t C_BG       = 0x0843;
static const uint16_t C_PANEL    = 0x10A5;
static const uint16_t C_PANEL2   = 0x18E7;
static const uint16_t C_BORDER   = 0x29CC;
static const uint16_t C_CYAN     = 0x5E9F;
static const uint16_t C_BLUE     = 0x3C9F;
static const uint16_t C_WHITE    = 0xFFFF;
static const uint16_t C_MUTED    = 0xA534;
static const uint16_t C_YELLOW   = 0xFEA0;
static const uint16_t C_GREEN    = 0x5E8B;
static const uint16_t C_RED      = 0xF3C9;

TFT_eSPI tft = TFT_eSPI();
Preferences preferences;
bool sdReady = false;
bool touchWasDown = false;
uint16_t lastTouchX = 0;
uint16_t lastTouchY = 0;
unsigned long lastRefreshMs = 0;

// The first boot calibrates and saves for portrait rotation 0.
uint16_t touchCalibration[5] = {0, 0, 0, 0, 0};

enum Page : uint8_t {
  PAGE_HOME,
  PAGE_WEATHER,
  PAGE_CLOCK,
  PAGE_CLIMATE,
  PAGE_GALLERY,
  PAGE_VIDEO,
  PAGE_SETTINGS
};
Page currentPage = PAGE_HOME;

const char* PAGE_LABELS[] = {
  "HOME", "WEATHER", "CLOCK", "DHT11", "GALLERY", "VIDEO", "SETTINGS"
};

struct Tile {
  const char* title;
  const char* detail;
  Page page;
  uint16_t color;
};

const Tile TILES[] = {
  {"WEATHER",  "Forecast",      PAGE_WEATHER,  C_CYAN},
  {"CLOCK",    "Time & uptime", PAGE_CLOCK,    C_YELLOW},
  {"DHT11",    "Room climate",  PAGE_CLIMATE,  C_GREEN},
  {"GALLERY",  "SD photos",     PAGE_GALLERY,  C_BLUE},
  {"VIDEO",    "SD clips",      PAGE_VIDEO,    C_RED},
  {"SETTINGS", "Device & touch",PAGE_SETTINGS, C_MUTED}
};

void setText(uint16_t color, uint8_t size = 1) {
  tft.setTextFont(1);  // Built-in GLCD font (LOAD_GLCD in User_Setup.h)
  tft.setTextSize(size);
  tft.setTextColor(color, C_BG);
}

void label(const String& s, int x, int y, uint16_t color = C_WHITE, uint8_t size = 1) {
  setText(color, size);
  tft.drawString(s, x, y, 1);
}

String uptimeText() {
  uint32_t seconds = millis() / 1000UL;
  uint32_t hours = seconds / 3600UL;
  uint32_t minutes = (seconds % 3600UL) / 60UL;
  char tmp[32];
  snprintf(tmp, sizeof(tmp), "%luh %02lum", (unsigned long)hours, (unsigned long)minutes);
  return String(tmp);
}

bool syncedTime(char* timeBuffer, size_t timeLen, char* dateBuffer, size_t dateLen) {
  time_t now = time(nullptr);
  if (now < (time_t)1700000000) return false;
  struct tm info;
  localtime_r(&now, &info);
  strftime(timeBuffer, timeLen, "%I:%M:%S %p", &info);
  strftime(dateBuffer, dateLen, "%a, %b %d %Y", &info);
  return true;
}

void wifiClockInit() {
  if (WIFI_SSID[0] == '\0') {
    Serial.println("Wi-Fi not configured; clock will display uptime.");
    return;
  }
  Serial.println("Connecting Wi-Fi for NTP (max 6 seconds)...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 6000) {
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) {
    // Philippine Standard Time UTC+8, no daylight saving time.
    configTime(8 * 3600, 0, "pool.ntp.org", "time.google.com");
    Serial.println("Wi-Fi connected; NTP requested.");
  } else {
    Serial.println("Wi-Fi not connected; clock will display uptime.");
  }
}

void calibrateAndSaveTouch() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(1);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("TOUCH CALIBRATION", SCREEN_W / 2, 110, 1);
  tft.setTextSize(1);
  tft.drawCentreString("Follow the four corner targets", SCREEN_W / 2, 148, 1);
  delay(900);

  tft.calibrateTouch(touchCalibration, TFT_MAGENTA, TFT_BLACK, 15);
  tft.setTouch(touchCalibration);
  if (preferences.begin("erinxtft43", false)) {
    preferences.putBytes("portrait", touchCalibration, sizeof(touchCalibration));
    preferences.end();
  }
  Serial.printf("Touch calibrated: {%u,%u,%u,%u,%u}\n",
                touchCalibration[0], touchCalibration[1], touchCalibration[2],
                touchCalibration[3], touchCalibration[4]);
}

void loadOrCalibrateTouch() {
  bool loaded = false;
  if (preferences.begin("erinxtft43", true)) {
    if (preferences.getBytesLength("portrait") == sizeof(touchCalibration)) {
      preferences.getBytes("portrait", touchCalibration, sizeof(touchCalibration));
      loaded = true;
    }
    preferences.end();
  }
  if (loaded) {
    tft.setTouch(touchCalibration);
    Serial.println("Saved portrait touch calibration loaded.");
  } else {
    Serial.println("No saved calibration; calibrating now.");
    calibrateAndSaveTouch();
  }
}

void initSD() {
  sdReady = SD_MMC.setPins(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN);
  if (sdReady) sdReady = SD_MMC.begin("/sdcard", true);
  if (sdReady && SD_MMC.cardType() == CARD_NONE) sdReady = false;
  if (sdReady) {
    Serial.printf("SD READY: %llu MB\n", SD_MMC.cardSize() / 1048576ULL);
  } else {
    Serial.println("SD unavailable: insert a FAT32 microSD, then restart.");
  }
}

void drawTop(const char* pageTitle) {
  tft.fillScreen(C_BG);
  // Back control (all pages except home)
  tft.fillRoundRect(12, 14, 68, 36, 9, C_PANEL2);
  label("< BACK", 18, 26, C_CYAN, 1);
  label(pageTitle, 96, 24, C_WHITE, 2);
  tft.drawFastHLine(12, 67, 296, C_BORDER);
}

void drawFooter() {
  tft.fillRect(0, FOOTER_Y, SCREEN_W, SCREEN_H - FOOTER_Y, C_BG);
  tft.drawFastHLine(0, FOOTER_Y, SCREEN_W, C_BORDER);
  label(sdReady ? "SD: READY" : "SD: OFFLINE", 12, 461,
        sdReady ? C_GREEN : C_RED, 1);
  char pos[35];
  snprintf(pos, sizeof(pos), "X:%u  Y:%u", lastTouchX, lastTouchY);
  label(pos, 184, 461, C_MUTED, 1);
}

void drawHomeClock() {
  tft.fillRect(15, 80, 290, 22, C_BG);
  char timeBuffer[32], dateBuffer[32];
  if (syncedTime(timeBuffer, sizeof(timeBuffer), dateBuffer, sizeof(dateBuffer))) {
    label(String(timeBuffer), 16, 83, C_CYAN, 1);
    label("PHT", 272, 83, C_MUTED, 1);
  } else {
    label("Clock not synced - open CLOCK", 16, 83, C_MUTED, 1);
  }
}

void drawTile(int index) {
  const int col = index % 2;
  const int row = index / 2;
  const int x = 12 + col * 154;
  const int y = 111 + row * 109;
  const int w = 142;
  const int h = 99;

  tft.fillRoundRect(x, y, w, h, 13, C_PANEL);
  tft.drawRoundRect(x, y, w, h, 13, C_BORDER);
  tft.fillRoundRect(x + 12, y + 12, 36, 29, 7, TILES[index].color);

  // Simple text-based tile icon, no font package needed.
  const char* glyphs[] = {"W", "12", "C", "SD", ">", "*"};
  tft.setTextFont(1);
  tft.setTextSize(2);
  tft.setTextColor(C_BG, TILES[index].color);
  tft.drawCentreString(glyphs[index], x + 30, y + 19, 1);

  label(TILES[index].title, x + 12, y + 51, C_WHITE, 2);
  label(TILES[index].detail, x + 12, y + 79, C_MUTED, 1);
}

void drawHome() {
  currentPage = PAGE_HOME;
  tft.fillScreen(C_BG);
  label("ERINX", 16, 17, C_WHITE, 3);
  label("SMART HUB", 16, 54, C_CYAN, 2);
  label("V4.3.1", 254, 26, C_MUTED, 1);
  tft.drawFastHLine(12, 75, 296, C_BORDER);
  drawHomeClock();
  for (int i = 0; i < 6; i++) drawTile(i);
  drawFooter();
}

void drawInfoBox(int y, const String& heading, const String& a, const String& b) {
  tft.fillRoundRect(15, y, 290, 125, 12, C_PANEL);
  label(heading, 27, y + 16, C_CYAN, 2);
  label(a, 27, y + 53, C_WHITE, 1);
  label(b, 27, y + 75, C_MUTED, 1);
}

void drawWeather() {
  drawTop("WEATHER");
  drawInfoBox(92, "FORECAST", "Weather API not connected yet", "Next: Open-Meteo + Wi-Fi");
  drawInfoBox(232, "STATUS", "The weather page is ready", "No forecast data is invented");
  drawFooter();
}

void drawClockContent() {
  // Refresh only the time card and uptime; leave the upper info panel intact.
  tft.fillRect(22, 238, 276, 134, C_PANEL);
  char timeBuffer[32], dateBuffer[32];
  if (syncedTime(timeBuffer, sizeof(timeBuffer), dateBuffer, sizeof(dateBuffer))) {
    label(String(timeBuffer), 32, 262, C_CYAN, 2);
    label(String(dateBuffer), 33, 307, C_WHITE, 1);
    label("Philippine Standard Time (UTC+8)", 33, 332, C_MUTED, 1);
  } else {
    label("--:--:--", 42, 263, C_CYAN, 3);
    label("CLOCK NOT SYNCED", 33, 311, C_YELLOW, 1);
    label("Set WIFI_SSID/PASSWORD in .ino", 33, 337, C_MUTED, 1);
  }
  tft.fillRect(15, 402, 290, 26, C_BG);
  label("Uptime: " + uptimeText(), 21, 405, C_GREEN, 2);
}

void drawClock() {
  drawTop("CLOCK");
  drawInfoBox(90, "LOCAL TIME", "Optional NTP synchronization", "Time zone: Philippines, UTC+8");
  tft.fillRoundRect(15, 230, 290, 150, 12, C_PANEL);
  drawClockContent();
  drawFooter();
}

void drawClimate() {
  drawTop("DHT11");
  drawInfoBox(92, "TEMPERATURE", "--.- C", "DHT11 sensor not wired yet");
  drawInfoBox(232, "HUMIDITY", "-- %", "No sensor readings yet");
  drawFooter();
}

bool acceptedMedia(const String& filename, bool video) {
  String s = filename;
  s.toLowerCase();
  if (video) return s.endsWith(".mjpg") || s.endsWith(".mjpeg");
  return s.endsWith(".jpg") || s.endsWith(".jpeg");
}

struct MediaSummary {
  uint16_t count = 0;
  String examples[5];
  uint8_t listed = 0;
};

// Important: the Arduino .ino preprocessor generates function prototypes.
// Avoid using MediaSummary in the function signature because that struct
// is declared later in the sketch than the generated prototypes.
// Using built-in Arduino types here keeps this a standalone .ino file.
void scanDirectory(const char* path, bool video, uint16_t& count,
                   String examples[5], uint8_t& listed) {
  if (!sdReady) return;

  // Arduino ESP32 core 3.x defines the file type as fs::File.
  fs::File dir = SD_MMC.open(path);
  if (!dir || !dir.isDirectory()) return;

  fs::File file = dir.openNextFile();
  uint16_t examined = 0;
  while (file && examined < 150) {
    examined++;
    if (!file.isDirectory()) {
      String name = String(file.name());
      if (acceptedMedia(name, video)) {
        count++;
        if (listed < 5) {
          if (name.length() > 30) name = name.substring(0, 27) + "...";
          examples[listed++] = name;
        }
      }
    }
    file.close();
    file = dir.openNextFile();
  }
  dir.close();
}

void drawMedia(bool video) {
  drawTop(video ? "VIDEO" : "GALLERY");
  const char* folder = video ? "/videos" : "/photos";
  label(video ? "MJPEG FILE BROWSER" : "JPEG FILE BROWSER", 19, 93, C_CYAN, 2);
  label("Checking SD card...", 20, 129, C_MUTED, 1);
  if (!sdReady) {
    drawInfoBox(159, "SD OFFLINE", "Restart with FAT32 microSD", "Slot is on the ESP32-S3 board");
    drawFooter();
    return;
  }

  MediaSummary summary;
  scanDirectory("/", video, summary.count, summary.examples, summary.listed);
  scanDirectory(folder, video, summary.count, summary.examples, summary.listed);
  tft.fillRect(15, 126, 290, 32, C_BG);
  label(String(summary.count) + (video ? " MJPEG file(s) found" : " JPEG file(s) found"),
        20, 132, C_WHITE, 1);
  tft.fillRoundRect(15, 167, 290, 204, 12, C_PANEL);
  if (summary.count == 0) {
    label("No matching media files yet.", 29, 197, C_YELLOW, 1);
    label(String("Add files to ") + folder, 29, 230, C_WHITE, 1);
    label("or the SD card root folder.", 29, 250, C_MUTED, 1);
  } else {
    for (uint8_t i = 0; i < summary.listed; i++) {
      label(String(i + 1) + ". " + summary.examples[i],
            29, 187 + i * 31, C_WHITE, 1);
    }
  }
  label(video ? "Playback support comes later" : "Image rendering comes later",
        20, 389, C_MUTED, 1);
  drawFooter();
}

void drawSettings() {
  drawTop("SETTINGS");
  drawInfoBox(89, "DEVICE", "ESP32-S3 CAM N16R8", "TFT: ILI9488 320 x 480 portrait");

  tft.fillRoundRect(15, 230, 290, 92, 12, C_PANEL);
  label("MICROSD", 27, 247, C_CYAN, 2);
  if (sdReady) {
    label("READY - " + String((unsigned long)(SD_MMC.cardSize() / 1048576ULL)) + " MB",
          27, 282, C_GREEN, 1);
  } else {
    label("OFFLINE", 27, 282, C_RED, 1);
  }

  tft.fillRoundRect(17, 356, 286, 54, 12, C_BLUE);
  tft.setTextFont(1);
  tft.setTextSize(2);
  tft.setTextColor(C_WHITE, C_BLUE);
  tft.drawCentreString("CALIBRATE TOUCH", SCREEN_W / 2, 375, 1);
  label("Tap above if touch points are inaccurate.", 21, 421, C_MUTED, 1);
  drawFooter();
}

void showPage(Page page) {
  currentPage = page;
  switch (page) {
    case PAGE_HOME:     drawHome();          break;
    case PAGE_WEATHER:  drawWeather();       break;
    case PAGE_CLOCK:    drawClock();         break;
    case PAGE_CLIMATE:  drawClimate();       break;
    case PAGE_GALLERY:  drawMedia(false);    break;
    case PAGE_VIDEO:    drawMedia(true);     break;
    case PAGE_SETTINGS: drawSettings();      break;
  }
}

void onTouch(uint16_t x, uint16_t y) {
  lastTouchX = x;
  lastTouchY = y;
  Serial.printf("TOUCH X:%u Y:%u  PAGE:%s\n", x, y, PAGE_LABELS[currentPage]);

  if (currentPage != PAGE_HOME && x >= 12 && x < 90 && y >= 10 && y < 59) {
    showPage(PAGE_HOME);
    return;
  }

  if (currentPage == PAGE_HOME) {
    for (int i = 0; i < 6; i++) {
      int col = i % 2;
      int row = i / 2;
      int tx = 12 + col * 154;
      int ty = 111 + row * 109;
      if (x >= tx && x < tx + 142 && y >= ty && y < ty + 99) {
        showPage(TILES[i].page);
        return;
      }
    }
    drawFooter();
  } else if (currentPage == PAGE_SETTINGS && x >= 17 && x < 303 &&
             y >= 356 && y < 410) {
    calibrateAndSaveTouch();
    showPage(PAGE_SETTINGS);
  } else {
    drawFooter();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("\n=== ERINX SMART HUB V4.3.1 (FIXED) ===");

  tft.init();
  tft.setRotation(0); // 320 x 480 PORTRAIT (matches previous working test)
  tft.fillScreen(C_BG);
  label("ERINX V4.3 - BOOTING", 24, 170, C_CYAN, 2);
  label("Display initialized", 24, 208, C_WHITE, 1);

  loadOrCalibrateTouch();
  initSD();
  wifiClockInit();
  showPage(PAGE_HOME);
  Serial.println("HOME SCREEN READY - tap any tile");
}

void loop() {
  uint16_t x = 0, y = 0;
  bool touched = tft.getTouch(&x, &y);
  if (touched && !touchWasDown) {
    touchWasDown = true;
    onTouch(x, y);
  } else if (!touched) {
    touchWasDown = false;
  }

  if (millis() - lastRefreshMs >= 1000) {
    lastRefreshMs = millis();
    if (currentPage == PAGE_CLOCK) drawClockContent();
    if (currentPage == PAGE_HOME) drawHomeClock();
  }
  delay(20);
}
