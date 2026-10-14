/*
  ERINX SMART HUB V4.8 - WEATHER / EYES / WIFI UI REFINEMENT (PORTRAIT)
  Hardware: ESP32-S3 CAM N16R8 + 3.5-inch 480x320 SPI TFT (portrait 320x480)
  Display library: TFT_eSPI (keep your already-working User_Setup.h)

  Preserved from V4.4:
    - Six touch-operated pages and BACK navigation
    - On-device touch calibration, saved in ESP32 NVS
    - Built-in SDMMC card detection and JPEG/MJPEG filename browser
    - Clock: optional Wi-Fi/NTP for Philippine Standard Time, otherwise unsynced + uptime
    - Open-Meteo current weather for Tanauan (editable location), refresh 10 minutes
    - DHT11 on GPIO14, independent indoor temp/humidity (graceful offline)
    - Live last-touch X/Y coordinates at bottom of home screen

  V4.5 UI optimizations (preserved):
    - Cleaner portrait-only home layout (320x480)
    - Transparent text on filled cards (no dark text background patches)
    - Clock + sensor partial updates rather than full screen redraws
    - Cached home clock content and clock page content; redraw only on change
    - Touch footer partial update; single-tap debounce
    - No new hardware requirements or new features

  V4.7 additions:
    - Original 40x40 RGB565 image icons loaded directly from built-in SDMMC
    - No new decoder library. Works using fs::File + TFT_eSPI.pushImage()
    - Automatic text-glyph fallback if icon files missing

  V4.8 additions:
    - Expressive 2-eye mascot in Home header and bottom of Clock page
    - Curved Wi-Fi icon and warning mark when Wi-Fi is disconnected
    - Centered weather animation for clear, cloud, rain, thunderstorm
    - No new asset files, GPIO wiring, or sensor requirements

  What is NOT implemented yet:
    - JPEG image rendering and MJPEG playback.
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
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <math.h>

// Enter your Wi-Fi credentials to enable live weather and NTP clock.
// Leave WIFI_SSID empty to run the dashboard offline.
const char WIFI_SSID[] = "";
const char WIFI_PASSWORD[] = "";

// Open-Meteo location: approximate city centre of Tanauan, Batangas, PH.
// Adjust these numbers for your preferred location.
const char WEATHER_LOCATION[] = "Tanauan, Batangas";
const double WEATHER_LATITUDE  = 14.0860;
const double WEATHER_LONGITUDE = 121.1500;

// DHT11 (not used by camera, SDMMC, TFT, or touch in this setup).
// For a 3-pin module: VCC -> 3V3; DATA -> GPIO14; GND -> GND.
#define DHT_PIN 14
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

static const unsigned long WEATHER_REFRESH_MS = 600000UL; // 10 minutes
static const unsigned long WEATHER_RETRY_MS   = 60000UL;  // retry after failure
static const unsigned long DHT_REFRESH_MS     = 3000UL;
static const unsigned long WIFI_RETRY_MS      = 15000UL;

struct WeatherState {
  bool valid = false;
  float temperature = NAN;
  float humidity = NAN;
  float feelsLike = NAN;
  float windKmh = NAN;
  int weatherCode = -1;
  bool isDay = true;
  String updatedAt = "";
};

WeatherState weather;
bool dhtValid = false;
float roomTemperature = NAN;
float roomHumidity = NAN;
unsigned long lastWeatherAttemptMs = 0;
unsigned long lastWeatherSuccessMs = 0;
unsigned long lastDhtAttemptMs = 0;
unsigned long lastWifiRetryMs = 0;
bool weatherAttempted = false;
bool ntpConfigured = false;

// ESP32-S3 CAM built-in SDMMC (1-bit mode). No external SD wires.
static const int SD_CLK_PIN = 39;
static const int SD_CMD_PIN = 38;
static const int SD_D0_PIN  = 40;

static const int SCREEN_W = 320;
static const int SCREEN_H = 480;
static const int FOOTER_Y = 450;
static const int TILE_TOP = 149;
static const int TILE_STEP_Y = 97;
static const int TILE_H = 89;
static const int TILE_W = 142;

// RGB565 palette for 3.5-inch TFT (blue/white redesign)
static const uint16_t C_BG       = 0x08A6;  // deep navy
static const uint16_t C_PANEL    = 0x114A;  // main card
static const uint16_t C_PANEL2   = 0x19CF;  // highlight card
static const uint16_t C_BORDER   = 0x33B4;  // cyan border
static const uint16_t C_CYAN     = 0x7EFF;  // bright cyan
static const uint16_t C_BLUE     = 0x4D9F;  // soft blue
static const uint16_t C_WHITE    = 0xFFFF;
static const uint16_t C_MUTED    = 0xB69B;  // blue-gray text
static const uint16_t C_YELLOW   = 0xFF6A;  // tiny accent for statuses only
static const uint16_t C_GREEN    = 0x87F1;  // aqua-green for OK
static const uint16_t C_RED      = 0xF3EA;

TFT_eSPI tft = TFT_eSPI();
Preferences preferences;
bool sdReady = false;
bool touchWasDown = false;
uint16_t lastTouchX = 0;
uint16_t lastTouchY = 0;
unsigned long lastRefreshMs = 0;
unsigned long lastTapMs = 0;
String cachedHomeClock = "";
String cachedClockText = "";
String cachedUptimeText = "";
unsigned long lastWeatherFrameMs = 0;
int8_t lastHomeEyePose = -1;
int8_t lastClockEyePose = -1;
int8_t lastHeaderWifiState = -1;

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

// The icons are RAW little-endian RGB565, exactly 40x40x2 = 3200 bytes.
// Put them at /icons/*.rgb565 at the ROOT of the ESP32-S3 built-in SD card.
// PNG preview files from the ZIP are NOT read by the firmware.
static const int ICON_W = 40;
static const int ICON_H = 40;
static uint16_t iconBuffer[ICON_W * ICON_H]; // 3.2KB RAM, reused for every tile
const char* ICON_FILES[6] = {
  "/icons/weather.rgb565",
  "/icons/clock.rgb565",
  "/icons/dht11.rgb565",
  "/icons/gallery.rgb565",
  "/icons/video.rgb565",
  "/icons/settings.rgb565"
};

// Read one small image straight from microSD. No PNG/JPEG decoder needed.
// If the SD is absent, this returns false and the old glyph icon is shown.
bool drawSDIcon(int index, int x, int y) {
  if (!sdReady || index < 0 || index >= 6) return false;
  fs::File f = SD_MMC.open(ICON_FILES[index], FILE_READ);
  if (!f) return false;
  if (f.size() != sizeof(iconBuffer)) {
    f.close();
    return false;
  }
  size_t actual = f.read((uint8_t*)iconBuffer, sizeof(iconBuffer));
  f.close();
  if (actual != sizeof(iconBuffer)) return false;

  // Encoded on disk as little-endian uint16_t RGB565, exactly what
  // ESP32-S3 reads into the in-memory uint16_t buffer.
  tft.pushImage(x, y, ICON_W, ICON_H, iconBuffer);
  return true;
}

void reportIconStatus() {
  if (!sdReady) {
    Serial.println("ICONS: SD OFFLINE - using fallback glyphs");
    return;
  }
  int found = 0;
  for (int i = 0; i < 6; ++i) {
    fs::File file = SD_MMC.open(ICON_FILES[i], FILE_READ);
    if (file && file.size() == sizeof(iconBuffer)) ++found;
    if (file) file.close();
  }
  Serial.printf("ICONS: %d/6 found in /icons (RGB565 40x40)\n", found);
}

// Foreground-only text: the background remains the card color, not C_BG.
// For dynamic labels always erase their *small value rectangle* before redraw.
void setText(uint16_t color, uint8_t size = 1) {
  tft.setTextFont(1);  // GLCD font (LOAD_GLCD must be enabled in User_Setup.h)
  tft.setTextSize(size);
  tft.setTextColor(color);
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

// Non-blocking Wi-Fi start: paint the first dashboard without waiting 6 seconds.
void wifiClockInit() {
  if (WIFI_SSID[0] == '\0') {
    Serial.println("Wi-Fi not configured; clock will display uptime.");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiRetryMs = millis();
  Serial.println("Wi-Fi connecting in background...");
}

void updateWiFiClock() {
  if (WIFI_SSID[0] == '\0') return;
  if (WiFi.status() == WL_CONNECTED) {
    if (!ntpConfigured) {
      configTime(8 * 3600, 0, "pool.ntp.org", "time.google.com");
      ntpConfigured = true;
      Serial.println("Wi-Fi connected; Philippine-time NTP requested.");
    }
  } else {
    ntpConfigured = false;
  }
}

// Compact descriptions of Open-Meteo WMO weather codes.
const char* weatherName(int code) {
  if (code == 0) return "Clear sky";
  if (code == 1) return "Mainly clear";
  if (code == 2) return "Partly cloudy";
  if (code == 3) return "Overcast";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Rain showers";
  if (code >= 85 && code <= 86) return "Snow showers";
  if (code >= 95) return "Thunderstorm";
  return "Other conditions";
}

void readDht() {
  lastDhtAttemptMs = millis();
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    dhtValid = false;
    Serial.println("DHT11: no valid reading (check wiring or sensor)");
    return;
  }
  roomTemperature = t;
  roomHumidity = h;
  dhtValid = true;
  Serial.printf("DHT11: %.1f C, %.0f %% RH\n", t, h);
}

bool fetchWeather() {
  weatherAttempted = true;
  lastWeatherAttemptMs = millis();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Weather: Wi-Fi offline, no API request");
    return false;
  }

  // Public GET endpoint: no API key or private data is included in the URL.
  // setInsecure() is used ONLY for this hobby weather prototype. For a
  // production build, verify the server with a trusted CA certificate.
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") +
      String(WEATHER_LATITUDE, 4) + "&longitude=" +
      String(WEATHER_LONGITUDE, 4) +
      "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,is_day,wind_speed_10m" +
      "&timezone=Asia%2FManila&forecast_days=1";

  if (!http.begin(client, url)) {
    Serial.println("Weather: HTTPS initialization failed");
    return false;
  }
  http.setConnectTimeout(4000);
  http.setTimeout(5000);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Weather HTTP error: %d\n", code);
    http.end();
    return false;
  }

  // 'current' returns a small JSON object. Parse it before releasing HTTP.
  String body = http.getString();
  http.end();
  JsonDocument doc; // ArduinoJson 7
  DeserializationError jsonError = deserializeJson(doc, body);
  if (jsonError) {
    Serial.printf("Weather JSON failed: %s\n", jsonError.c_str());
    return false;
  }
  JsonObject current = doc["current"].as<JsonObject>();
  if (current.isNull() || current["temperature_2m"].isNull() ||
      current["relative_humidity_2m"].isNull() ||
      current["weather_code"].isNull()) {
    Serial.println("Weather JSON missing required current fields");
    return false;
  }

  weather.temperature = current["temperature_2m"].as<float>();
  weather.humidity = current["relative_humidity_2m"].as<float>();
  weather.feelsLike = current["apparent_temperature"].as<float>();
  weather.windKmh = current["wind_speed_10m"].as<float>();
  weather.weatherCode = current["weather_code"].as<int>();
  weather.isDay = current["is_day"].as<int>() == 1;
  weather.updatedAt = current["time"].as<String>();
  weather.valid = true;
  lastWeatherSuccessMs = millis();
  Serial.printf("Weather %s: %.1f C, %.0f%%, WMO %d\n",
                WEATHER_LOCATION, weather.temperature, weather.humidity,
                weather.weatherCode);
  return true;
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


// Wi-Fi arcs are deliberately drawn with short line segments, so no graphics
// library other than the existing TFT_eSPI is required.
// "connected" is Wi-Fi link status; an internet reachability test is separate.
void drawWifiIcon(int x, int y, bool connected) {
  uint16_t color = connected ? C_WHITE : C_MUTED;
  tft.fillCircle(x, y + 9, 2, color);
  // 3 curved bars, expanding outwards from the bottom dot.
  for (int r = 6; r <= 16; r += 5) {
    for (int deg = 210; deg < 330; deg += 10) {
      float a = deg * 0.01745329252f;
      float b = (deg + 10) * 0.01745329252f;
      tft.drawLine(x + lroundf(r * cosf(a)), y + 9 + lroundf(r * sinf(a)),
                   x + lroundf(r * cosf(b)), y + 9 + lroundf(r * sinf(b)), color);
    }
  }
  // Exclamation mark is shown only when disconnected.
  if (!connected) {
    tft.fillRoundRect(x + 12, y - 9, 3, 10, 1, C_WHITE);
    tft.fillCircle(x + 13, y + 5, 2, C_WHITE);
  }
}

void drawSdSmallIcon(int x, int y, bool ready) {
  uint16_t c = ready ? C_CYAN : C_MUTED;
  tft.drawRoundRect(x, y, 14, 18, 2, c);
  tft.drawLine(x + 3, y + 5, x + 11, y + 5, c);
  tft.drawLine(x + 3, y + 8, x + 11, y + 8, c);
  tft.drawLine(x + 3, y + 11, x + 11, y + 11, c);
  if (!ready) tft.drawLine(x + 2, y + 16, x + 12, y + 2, C_MUTED);
}

// POSES: forward, left, right, up, down, blink, sleep, cross-eyed.
uint8_t eyePoseAt(unsigned long ms) {
  if (ms % 6300UL < 170UL) return 5; // fast blink (~0.17 second)
  switch ((ms / 900UL) % 8UL) {
    case 1: return 1; // left
    case 2: return 2; // right
    case 3: return 3; // up
    case 4: return 0; // front
    case 5: return 4; // down
    case 6: return 7; // cross-eyed
    case 7: return 6; // sleepy
    default: return 0;
  }
}

// Rectangular black face containing two separate expressive white eyes.
// Large variant fills the spare area at the bottom of the Clock page.
void drawRobotEyes(int x, int y, bool large, bool force = false) {
  if (currentPage != PAGE_HOME && currentPage != PAGE_CLOCK) return;
  uint8_t pose = eyePoseAt(millis());
  int8_t &lastPose = large ? lastClockEyePose : lastHomeEyePose;
  if (!force && lastPose == (int8_t)pose) return;  // no redundant redraw
  lastPose = pose;

  const int w = large ? 264 : 65;
  const int h = large ? 90 : 32;
  const int radius = large ? 16 : 9;
  const int eyeW = large ? 44 : 18;
  const int eyeH = large ? 58 : 21;
  const int gap = large ? 25 : 7;
  const int leftEyeX = x + (w - (eyeW * 2 + gap)) / 2;
  const int topEyeY = y + (h - eyeH) / 2;

  tft.fillRoundRect(x, y, w, h, radius, C_PANEL);
  tft.drawRoundRect(x, y, w, h, radius, C_BORDER);

  const int pupilR = large ? 7 : 3;
  const int shiftX = pose == 1 ? -pupilR : (pose == 2 ? pupilR : 0);
  const int shiftY = pose == 3 ? -pupilR : (pose == 4 ? pupilR : 0);
  for (int i = 0; i < 2; ++i) {
    int ex = leftEyeX + i * (eyeW + gap);
    int cx = ex + eyeW / 2;
    int cy = topEyeY + eyeH / 2;
    if (pose == 5 || pose == 6) {  // blink / drowsy
      int eyeline = pose == 5 ? cy : cy + (large ? 8 : 3);
      tft.drawFastHLine(ex, eyeline, eyeW, C_WHITE);
      tft.drawFastHLine(ex + 1, eyeline + 1, eyeW - 2, C_CYAN);
      continue;
    }
    tft.fillRoundRect(ex, topEyeY, eyeW, eyeH, eyeW / 2, C_WHITE);
    int cross = pose == 7 ? (i == 0 ? pupilR : -pupilR) : shiftX;
    tft.fillCircle(cx + cross, cy + shiftY, pupilR, C_BG);
    if (large) tft.fillCircle(cx + cross - 2, cy + shiftY - 2, 2, C_WHITE);
  }
}

// Open-Meteo WMO categories. The animation only represents the latest
// received weather state. For offline/no data we show a neutral cloud.
uint8_t weatherVisualState() {
  if (!weather.valid) return 1;
  const int code = weather.weatherCode;
  if (code >= 95) return 3;  // thunderstorm
  if (code == 0 || code == 1) return 0; // sunny
  if ((code >= 51 && code <= 86)) return 2; // rain/drizzle/snow showers
  return 1; // partly cloudy, overcast, fog, unknown
}

void drawSimpleCloud(int cx, int cy, uint16_t color = C_WHITE) {
  tft.fillCircle(cx - 22, cy + 2, 15, color);
  tft.fillCircle(cx - 4, cy - 7, 20, color);
  tft.fillCircle(cx + 18, cy, 15, color);
  tft.fillRoundRect(cx - 34, cy + 1, 70, 18, 8, color);
}

// Centered in the existing weather card, no small separate animation box.
// Frame count is intentionally low to reduce TFT SPI traffic / flicker.
void drawWeatherScene(bool force = false) {
  if (currentPage != PAGE_WEATHER) return;
  unsigned long ms = millis();
  if (!force && ms - lastWeatherFrameMs < 350UL) return;
  lastWeatherFrameMs = ms;
  const int bx = 36, by = 159, bw = 248, bh = 94;
  tft.fillRect(bx, by, bw, bh, C_PANEL);

  uint8_t mode = weatherVisualState();
  int motion = (int)((ms / 350UL) % 15UL) - 7;
  int cx = 160 + motion;
  int cy = 196;

  if (mode == 0) { // Sun: gentle pulsating rays
    int sunR = 22 + ((ms / 700UL) % 2UL);
    tft.fillCircle(160, 197, sunR, C_WHITE);
    for (int i = 0; i < 8; ++i) {
      float a = (i * 45) * 0.01745329252f;
      tft.drawLine(160 + lroundf(29 * cosf(a)), 197 + lroundf(29 * sinf(a)),
                   160 + lroundf(38 * cosf(a)), 197 + lroundf(38 * sinf(a)), C_CYAN);
    }
  } else if (mode == 1) { // Cloudy / partly cloudy
    drawSimpleCloud(cx, cy, C_WHITE);
  } else if (mode == 2) { // Rain
    drawSimpleCloud(cx, cy - 14, C_WHITE);
    for (int i = 0; i < 5; ++i) {
      int ry = 218 + (int)((ms / 350UL + i * 3) % 12UL);
      int rx = 111 + i * 22;
      tft.drawLine(rx, ry, rx - 4, ry + 8, C_CYAN);
    }
  } else { // Thunderstorm
    drawSimpleCloud(cx, cy - 14, C_WHITE);
    if (((ms / 350UL) % 4UL) < 2UL) {
      tft.drawLine(163, 213, 150, 226, C_CYAN);
      tft.drawLine(150, 226, 167, 226, C_CYAN);
      tft.drawLine(167, 226, 155, 241, C_WHITE);
    }
    for (int i = 0; i < 3; ++i) {
      int ry = 223 + (int)((ms / 350UL + i * 2) % 7UL);
      int rx = 116 + i * 49;
      tft.drawLine(rx, ry, rx - 3, ry + 6, C_CYAN);
    }
  }
}

void drawTop(const char* pageTitle) {
  tft.fillScreen(C_BG); // Only on intentional page navigation
  tft.fillRoundRect(12, 14, 74, 38, 9, C_PANEL2);
  label("< BACK", 20, 27, C_CYAN, 1);
  label(pageTitle, 98, 21, C_WHITE, 2);
  tft.fillRect(245, 12, 63, 43, C_BG);
  drawWifiIcon(265, 33, WiFi.status() == WL_CONNECTED);
  drawSdSmallIcon(289, 23, sdReady);
  lastHeaderWifiState = WiFi.status() == WL_CONNECTED ? 1 : 0;
  tft.drawFastHLine(12, 64, 296, C_BORDER);
}

void updateTopWifiStatus() {
  if (currentPage == PAGE_HOME) return;
  int8_t status = WiFi.status() == WL_CONNECTED ? 1 : 0;
  if (status == lastHeaderWifiState) return;
  lastHeaderWifiState = status;
  tft.fillRect(245, 12, 43, 43, C_BG);
  drawWifiIcon(265, 33, status == 1);
}

void drawFooter() {
  tft.fillRect(0, FOOTER_Y, SCREEN_W, SCREEN_H - FOOTER_Y, C_BG);
  tft.drawFastHLine(0, FOOTER_Y, SCREEN_W, C_BORDER);
  label("TAP", 12, 461, C_GREEN, 1);
  label(sdReady ? "SD READY" : "NO SD", 48, 461,
        sdReady ? C_CYAN : C_RED, 1);
  char pos[35];
  snprintf(pos, sizeof(pos), "X:%u  Y:%u", lastTouchX, lastTouchY);
  label(pos, 210, 461, C_MUTED, 1);
}

// Touch readings never trigger a whole screen/footer repaint.
void updateTouchFooter() {
  tft.fillRect(180, 454, 140, 25, C_BG);
  char pos[35];
  snprintf(pos, sizeof(pos), "X:%u  Y:%u", lastTouchX, lastTouchY);
  label(pos, 184, 461, C_MUTED, 1);
}

// The home clock/status is a compact, partial-update region.
void drawHomeClock() {
  char timeBuffer[32], dateBuffer[32];
  bool synced = syncedTime(timeBuffer, sizeof(timeBuffer), dateBuffer, sizeof(dateBuffer));
  String clockText = synced ? String(timeBuffer) : String("TIME NOT SYNCED");
  if (synced && clockText.length() >= 8) clockText = clockText.substring(0, 5) + clockText.substring(8);
  const String key = clockText + String("/") + String(WiFi.status() == WL_CONNECTED) + String("/") + String(sdReady);
  if (cachedHomeClock == key) return;
  cachedHomeClock = key;

  tft.fillRect(22, 96, 180, 28, C_PANEL2);
  tft.fillRect(230, 85, 60, 40, C_PANEL2);
  if (synced) {
    label(clockText, 24, 95, C_WHITE, 3);
  } else {
    label("OFFLINE", 24, 100, C_YELLOW, 2);
  }
  drawWifiIcon(247, 97, WiFi.status() == WL_CONNECTED);
  drawSdSmallIcon(277, 89, sdReady);
  label(WiFi.status() == WL_CONNECTED ? "ON" : "OFF", 236, 116, C_WHITE, 1);
  label(sdReady ? "OK" : "NO", 269, 116, C_WHITE, 1);
}

void drawTile(int index) {
  const int col = index % 2;
  const int row = index / 2;
  const int x = 12 + col * 154;
  const int y = TILE_TOP + row * TILE_STEP_Y;
  tft.fillRoundRect(x, y, TILE_W, TILE_H, 12, C_PANEL);
  tft.drawRoundRect(x, y, TILE_W, TILE_H, 12, C_BORDER);
  if (!drawSDIcon(index, x + 10, y + 8)) {
    tft.fillRoundRect(x + 10, y + 8, 40, 40, 8, C_PANEL2);
    const char* glyphs[] = {"W", "C", "D", "G", "V", "S"};
    tft.setTextFont(1);
    tft.setTextSize(2);
    tft.setTextColor(C_WHITE, C_PANEL2);
    tft.drawCentreString(glyphs[index], x + 30, y + 20, 1);
  }
  label(TILES[index].title, x + 11, y + 51, C_WHITE, 2);
  label(TILES[index].detail, x + 11, y + 73, C_MUTED, 1);
}

void drawHome() {
  currentPage = PAGE_HOME;
  tft.fillScreen(C_BG);
  label("ERINX SMART HUB", 15, 18, C_WHITE, 2);
  label("SMART DISPLAY  /  V4.8", 16, 44, C_MUTED, 1);
  lastHomeEyePose = -1;
  drawRobotEyes(240, 12, false, true);
  tft.drawFastHLine(12, 65, 296, C_BORDER);

  tft.fillRoundRect(12, 74, 296, 62, 12, C_PANEL2);
  label("PHILIPPINE TIME", 23, 83, C_CYAN, 1);
  cachedHomeClock = "";
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
  label(String(WEATHER_LOCATION), 18, 82, C_CYAN, 2);

  tft.fillRoundRect(15, 112, 290, 150, 13, C_PANEL);
  label("CURRENT CONDITIONS", 27, 126, C_WHITE, 2);
  drawWeatherScene(true);

  tft.fillRoundRect(15, 276, 290, 144, 12, C_PANEL);
  label("LIVE DATA", 27, 290, C_CYAN, 1);
  if (weather.valid) {
    label(String(weather.temperature, 1) + " C", 27, 310, C_WHITE, 3);
    label(String(weatherName(weather.weatherCode)), 27, 350, C_WHITE, 2);
    label(String("Humidity: ") + String(weather.humidity, 0) + "%", 27, 378, C_MUTED, 1);
    String timeText = weather.updatedAt.length() >= 16 ? weather.updatedAt.substring(11, 16) : "--:--";
    label(String("Updated: ") + timeText + " PHT", 27, 397, C_MUTED, 1);
  } else {
    label("--.- C", 27, 311, C_WHITE, 3);
    label(WIFI_SSID[0] == 0 ? "Add Wi-Fi in .ino" : "Waiting for weather data...", 27, 352, C_YELLOW, 1);
    label(WiFi.status() == WL_CONNECTED ? "Wi-Fi connected" : "Wi-Fi offline", 27, 379, C_MUTED, 1);
  }
  drawFooter();
}

void drawClockContent() {
  char timeBuffer[32], dateBuffer[32];
  bool synced = syncedTime(timeBuffer, sizeof(timeBuffer), dateBuffer, sizeof(dateBuffer));
  String textKey = synced ? String(timeBuffer) + String("/") + dateBuffer : String("UNSYNCED");
  if (cachedClockText != textKey) {
    cachedClockText = textKey;
    tft.fillRect(26, 108, 268, 68, C_PANEL2);
    tft.fillRect(26, 224, 268, 86, C_PANEL);
    if (synced) {
      String t = String(timeBuffer);
      String mainTime = t.substring(0, 8);
      String ampm = t.substring(9);
      label(mainTime, 34, 116, C_WHITE, 4);
      label(ampm, 244, 132, C_WHITE, 2);
      time_t now = time(nullptr);
      struct tm info;
      localtime_r(&now, &info);
      char dayBuf[18], dateBuf[28];
      strftime(dayBuf, sizeof(dayBuf), "%A", &info);
      strftime(dateBuf, sizeof(dateBuf), "%B %d, %Y", &info);
      label(dayBuf, 34, 236, C_WHITE, 2);
      label(dateBuf, 34, 264, C_CYAN, 1);
      label("Philippine Standard Time", 34, 286, C_MUTED, 1);
    } else {
      label("--:--:--", 34, 116, C_WHITE, 4);
      label("AM", 244, 132, C_WHITE, 2);
      label("CLOCK NOT SYNCED", 34, 236, C_YELLOW, 2);
      label("Connect Wi-Fi in .ino", 34, 264, C_MUTED, 1);
      label("Philippine Standard Time", 34, 286, C_MUTED, 1);
    }
  }
  String up = uptimeText();
  if (up != cachedUptimeText) {
    cachedUptimeText = up;
    tft.fillRect(0, 451, SCREEN_W, 25, C_BG);
    label("TAP", 12, 461, C_GREEN, 1);
    label(String("UPTIME ") + up, 56, 461, C_MUTED, 1);
    char pos[35];
    snprintf(pos, sizeof(pos), "X:%u  Y:%u", lastTouchX, lastTouchY);
    label(pos, 210, 461, C_MUTED, 1);
  }
}

void drawClock() {
  drawTop("CLOCK");
  tft.fillRoundRect(15, 88, 290, 100, 12, C_PANEL2);
  tft.fillRoundRect(15, 210, 290, 116, 12, C_PANEL);
  cachedClockText = "";
  cachedUptimeText = "";
  drawClockContent();
  lastClockEyePose = -1;
  drawRobotEyes(28, 344, true, true);  // two expressive eyes in spare space
  drawFooter();
}

void drawClimateContent() {
  // Update only the two value fields and the sensor-status line.
  tft.fillRect(27, 178, 268, 49, C_PANEL);
  label(dhtValid ? String(roomTemperature, 1) + " C" : "--.- C",
        27, 178, C_WHITE, 3);
  tft.fillRect(27, 313, 268, 49, C_PANEL);
  label(dhtValid ? String(roomHumidity, 0) + " %" : "-- %",
        27, 313, C_CYAN, 3);
  tft.fillRect(17, 400, 300, 30, C_BG);
  label(dhtValid ? "Sensor online - automatic refresh" :
        "No DHT11 reading - check wiring",
        20, 406, dhtValid ? C_GREEN : C_YELLOW, 1);
}

void drawClimate() {
  drawTop("DHT11");
  label("INDOOR SENSOR - GPIO 14", 20, 89, C_CYAN, 1);
  tft.fillRoundRect(15, 125, 290, 120, 12, C_PANEL);
  label("ROOM TEMPERATURE", 27, 143, C_MUTED, 1);
  tft.fillRoundRect(15, 261, 290, 120, 12, C_PANEL);
  label("ROOM HUMIDITY", 27, 279, C_MUTED, 1);
  drawClimateContent();
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
  tft.fillRoundRect(15, 91, 290, 122, 12, C_PANEL);
  label("DEVICE", 27, 105, C_CYAN, 2);
  label("ESP32-S3 CAM N16R8", 27, 138, C_WHITE, 1);
  label(WiFi.status() == WL_CONNECTED ? "Wi-Fi: CONNECTED" : "Wi-Fi: OFFLINE",
        27, 161, WiFi.status() == WL_CONNECTED ? C_GREEN : C_YELLOW, 1);
  label(dhtValid ? "DHT11: READY" : "DHT11: NO READING", 27, 186,
        dhtValid ? C_GREEN : C_MUTED, 1);

  tft.fillRoundRect(15, 226, 290, 96, 12, C_PANEL);
  label("MICROSD", 27, 241, C_CYAN, 2);
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
  label("Tap to redo touchscreen calibration", 21, 421, C_MUTED, 1);
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
      int ty = TILE_TOP + row * TILE_STEP_Y;
      if (x >= tx && x < tx + TILE_W && y >= ty && y < ty + TILE_H) {
        showPage(TILES[i].page);
        return;
      }
    }
    updateTouchFooter();
  } else if (currentPage == PAGE_SETTINGS && x >= 17 && x < 303 &&
             y >= 356 && y < 410) {
    calibrateAndSaveTouch();
    showPage(PAGE_SETTINGS);
  } else {
    updateTouchFooter();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("\n=== ERINX SMART HUB V4.8 EYES + WEATHER UI ===");

  tft.init();
  tft.setRotation(0); // 320 x 480 PORTRAIT (matches previous working test)
  tft.fillScreen(C_BG);
  label("ERINX V4.8 - BOOTING", 24, 170, C_CYAN, 2);
  label("Display initialized", 24, 208, C_WHITE, 1);

  loadOrCalibrateTouch();
  initSD();
  reportIconStatus();
  dht.begin();
  Serial.println("DHT11 initialized on GPIO14");
  wifiClockInit();
  updateWiFiClock();
  if (WiFi.status() == WL_CONNECTED) fetchWeather();
  readDht();
  showPage(PAGE_HOME);
  Serial.println("HOME SCREEN READY - tap any tile");
}

void loop() {
  updateWiFiClock();
  unsigned long now = millis();
  uint16_t x = 0, y = 0;
  bool touched = tft.getTouch(&x, &y);
  if (touched && !touchWasDown) {
    touchWasDown = true;
    if (now - lastTapMs >= 130UL) {
      lastTapMs = now;
      onTouch(x, y);
    }
  } else if (!touched) {
    touchWasDown = false;
  }

  if (now - lastDhtAttemptMs >= DHT_REFRESH_MS) {
    bool previousValid = dhtValid;
    float oldTemp = roomTemperature;
    float oldHumidity = roomHumidity;
    readDht();
    if (currentPage == PAGE_CLIMATE &&
        (dhtValid != previousValid ||
         (dhtValid && (oldTemp != roomTemperature || oldHumidity != roomHumidity)))) {
      drawClimateContent();
    }
  }

  if (WIFI_SSID[0] != '\0' && WiFi.status() != WL_CONNECTED &&
      now - lastWifiRetryMs >= WIFI_RETRY_MS) {
    lastWifiRetryMs = now;
    WiFi.reconnect();
  }

  bool refreshDue = !weatherAttempted ||
      (now - lastWeatherAttemptMs >=
       (weather.valid ? WEATHER_REFRESH_MS : WEATHER_RETRY_MS));
  if (refreshDue && WiFi.status() == WL_CONNECTED) {
    bool gotNewWeather = fetchWeather();
    if (gotNewWeather && currentPage == PAGE_WEATHER) drawWeather();
  }

  if (millis() - lastRefreshMs >= 1000) {
    lastRefreshMs = millis();
    if (currentPage == PAGE_CLOCK) drawClockContent();
    if (currentPage == PAGE_HOME) drawHomeClock();
  }
  // UI animations use separate non-blocking state updates.
  if (currentPage == PAGE_HOME) drawRobotEyes(240, 12, false);
  if (currentPage == PAGE_CLOCK) drawRobotEyes(28, 344, true);
  if (currentPage == PAGE_WEATHER) drawWeatherScene();
  updateTopWifiStatus();
  delay(12);
}
