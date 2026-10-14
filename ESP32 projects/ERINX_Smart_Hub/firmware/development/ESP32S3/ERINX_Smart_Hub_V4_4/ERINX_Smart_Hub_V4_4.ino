/*
  ERINX SMART HUB V4.4 - LIVE WEATHER + PHILIPPINE TIME + DHT11
  Hardware: ESP32-S3 CAM N16R8 + 3.5-inch 480x320 SPI TFT (portrait 320x480)
  Display library: TFT_eSPI (keep your already-working User_Setup.h)

  What works in V4.4:
    - Six touch-operated pages and BACK navigation
    - On-device touch calibration, saved in ESP32 NVS
    - Built-in SDMMC card detection and JPEG/MJPEG filename browser
    - Clock: optional Wi-Fi/NTP for Philippine Standard Time, otherwise unsynced + uptime
    - Open-Meteo current weather for Tanauan (editable location), refresh 10 minutes
    - DHT11 on GPIO14, independent indoor temp/humidity (graceful offline)
    - Live last-touch X/Y coordinates at bottom of home screen

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
  label("V4.4", 274, 26, C_MUTED, 1);
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
  label(String(WEATHER_LOCATION), 18, 83, C_CYAN, 2);

  tft.fillRoundRect(15, 121, 290, 180, 13, C_PANEL);
  label("OUTDOOR WEATHER - OPEN-METEO", 27, 137, C_MUTED, 1);

  if (weather.valid) {
    label(String(weather.temperature, 1) + " C", 27, 165, C_WHITE, 3);
    label(String(weatherName(weather.weatherCode)), 27, 211, C_CYAN, 2);
    label(String("Humidity: ") + String(weather.humidity, 0) + "%", 27, 255, C_WHITE, 1);
    if (WiFi.status() != WL_CONNECTED) {
      label("OFFLINE - LAST RECEIVED DATA", 27, 279, C_YELLOW, 1);
    }
  } else {
    label("--.- C", 27, 168, C_WHITE, 3);
    label(WIFI_SSID[0] == '\0' ? "Enter Wi-Fi details in .ino" :
         "Waiting for weather data...", 27, 222, C_YELLOW, 1);
    label(WiFi.status() == WL_CONNECTED ? "Wi-Fi: connected" : "Wi-Fi: offline",
          27, 265, C_MUTED, 1);
  }

  tft.fillRoundRect(15, 314, 290, 108, 12, C_PANEL);
  label("ADDITIONAL DETAILS", 27, 326, C_CYAN, 1);
  if (weather.valid) {
    label(String("Feels like: ") + String(weather.feelsLike, 1) + " C", 27, 347, C_WHITE, 1);
    label(String("Wind: ") + String(weather.windKmh, 1) + " km/h", 27, 366, C_WHITE, 1);
    String timeText = weather.updatedAt.length() >= 16 ?
      weather.updatedAt.substring(11, 16) : "--:--";
    label(String("Model time (PHT): ") + timeText, 27, 388, C_MUTED, 1);
  } else {
    label("No weather reading yet", 27, 352, C_MUTED, 1);
    label("Automatic retry when connected", 27, 378, C_MUTED, 1);
  }
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
  label("INDOOR SENSOR - GPIO 14", 20, 89, C_CYAN, 1);
  tft.fillRoundRect(15, 125, 290, 120, 12, C_PANEL);
  label("ROOM TEMPERATURE", 27, 143, C_MUTED, 1);
  label(dhtValid ? String(roomTemperature, 1) + " C" : "--.- C",
        27, 178, C_WHITE, 3);
  tft.fillRoundRect(15, 261, 290, 120, 12, C_PANEL);
  label("ROOM HUMIDITY", 27, 279, C_MUTED, 1);
  label(dhtValid ? String(roomHumidity, 0) + " %" : "-- %",
        27, 313, C_CYAN, 3);
  label(dhtValid ? "Sensor online - automatic refresh" :
        "No DHT11 reading - check 3V3 / DATA / GND",
        20, 406, dhtValid ? C_GREEN : C_YELLOW, 1);
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
  Serial.println("\n=== ERINX SMART HUB V4.4 ===");

  tft.init();
  tft.setRotation(0); // 320 x 480 PORTRAIT (matches previous working test)
  tft.fillScreen(C_BG);
  label("ERINX V4.4 - BOOTING", 24, 170, C_CYAN, 2);
  label("Display initialized", 24, 208, C_WHITE, 1);

  loadOrCalibrateTouch();
  initSD();
  dht.begin();
  Serial.println("DHT11 initialized on GPIO14");
  wifiClockInit();
  if (WiFi.status() == WL_CONNECTED) fetchWeather();
  readDht();
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

  unsigned long now = millis();
  if (now - lastDhtAttemptMs >= DHT_REFRESH_MS) {
    bool previousValid = dhtValid;
    float oldTemp = roomTemperature;
    float oldHumidity = roomHumidity;
    readDht();
    if (currentPage == PAGE_CLIMATE &&
        (dhtValid != previousValid ||
         (dhtValid && (oldTemp != roomTemperature || oldHumidity != roomHumidity)))) {
      drawClimate();
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
  delay(20);
}
