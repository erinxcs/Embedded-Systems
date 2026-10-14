/*
  ERINX SMART HUB V5.1.2 - WIFI + LOCATION (BOARD / SDMMC DIAGNOSTIC)
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

  V4.9 additions:
    - JPEG Photo Gallery (up to 40 images in /photos)
    - Full-screen portrait photo viewing, previous/next, auto-slide frame
    - Tap overlay controls; controls auto-hide after 3 seconds
    - Baseline JPEG decoding from ESP32-S3 SDMMC via TJpg_Decoder
  V5.1.1 fix:
    - Page enum defined in PageTypes.h to fix Arduino auto-prototype compilation.
  V5.1.2 diagnosis:
    - Explicit ESP32-S3 target and SDMMC availability check at compile time.
    - SDMMC header imported before dependent UI/media headers.
  V5.1 additions:
    - Weather city dropdown: Malvar / Tanauan / Lipa / Sto. Tomas; saved in Preferences.
    - Nearby Wi-Fi scan, on-screen password keyboard, connect and remember in NVS.
    - Existing Gallery, SD icons, DHT11, Clock and Video Player kept from V5.0.
  V5.0 additions:
    - Video list in /videos, choose a .evm clip
    - Custom MJPEG-frame container (ERXV) from PC FFmpeg/Pillow converter
    - Silent portrait playback, play/pause, prev/next, seamless restart on end
    - Auto-hide playback controls, saved home/clock/weather/touch/photo features
  Photos should ideally be 320x480 baseline JPEG; large files are downscaled by 1/2/4/8.

  IMPORTANT: This uses the same TFT wiring that passed your TFT_eSPI test.
  The built-in SD card is already wired on the ESP32-S3 CAM motherboard.
*/

#include <Arduino.h>
#include <soc/soc_caps.h>

// This project uses the ESP32-S3 built-in SDMMC controller.
// If you see either message in the IDE, fix Tools > Board first.
#ifndef CONFIG_IDF_TARGET_ESP32S3
#error "ERINX V5.1.2 requires Tools > Board > ESP32S3 Dev Module (NOT ESP32 Dev Module/C3/C6)."
#endif
#if !defined(SOC_SDMMC_HOST_SUPPORTED) || !SOC_SDMMC_HOST_SUPPORTED
#error "Selected ESP32 target has no SDMMC host. Select ESP32S3 Dev Module."
#endif
#if defined(NO_GLOBAL_INSTANCES) || defined(NO_GLOBAL_SD_MMC)
#error "SD_MMC global disabled by build macro. Remove NO_GLOBAL_INSTANCES / NO_GLOBAL_SD_MMC."
#endif

// Import the SDMMC implementation before image decoder/UI libraries.
#include <FS.h>
#include <SD_MMC.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <math.h>
#include <TJpg_Decoder.h>
#include <esp_heap_caps.h>
#include <string.h>
#include "PageTypes.h"

// V5.1: On-screen Wi-Fi picker and saved connections. No password in code.
// Values persist in ESP32 NVS across restarts until manually forgotten.
struct WeatherPlace { const char* name; double lat; double lon; };
const WeatherPlace WEATHER_PLACES[4] = {
  {"Malvar",      14.0473, 121.1576},
  {"Tanauan",     14.0860, 121.1500},
  {"Lipa",        13.9414, 121.1642},
  {"Sto. Tomas",  14.1092, 121.1419}
};
uint8_t selectedWeatherPlace = 1; // Tanauan default
bool weatherDropdownOpen = false;
const char* currentCity() { return WEATHER_PLACES[selectedWeatherPlace].name; }

static const uint8_t MAX_WIFI_NETWORKS = 12;
String wifiSSIDs[MAX_WIFI_NETWORKS];
int wifiRSSI[MAX_WIFI_NETWORKS];
bool wifiSecured[MAX_WIFI_NETWORKS];
uint8_t wifiCount = 0, wifiListPage = 0;
bool wifiScanning = false, wifiConnecting = false;
unsigned long wifiScanStarted = 0, wifiConnectStarted = 0;
String savedSSID, savedPassword, pendingSSID, pendingPassword;
String wifiMessage = "Ready";
bool wifiSaveWhenConnected = false;
String passwordInput;
bool passwordCaps = false, passwordSymbols = false;

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

Page currentPage = PAGE_HOME;

const char* PAGE_LABELS[] = {
  "HOME", "WEATHER", "CLOCK", "DHT11", "GALLERY", "PHOTO", "VIDEO", "PLAYER", "SETTINGS", "WI-FI", "PASSWORD"
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

// -------- V4.9 PHOTO GALLERY --------
// Fixed-capacity list; avoid scanning the card during every touch event.
static const uint8_t MAX_PHOTOS = 40;
static const uint8_t PHOTOS_PER_PAGE = 4;
static const unsigned long SLIDE_INTERVAL_MS = 7000UL; // 7 s
static const unsigned long PHOTO_CONTROLS_MS = 3000UL;
String photoFiles[MAX_PHOTOS];
uint8_t photoCount = 0;
uint8_t galleryPage = 0;
uint8_t photoIndex = 0;
bool slideShow = false;
bool photoControlsVisible = false;
unsigned long photoControlsShownAt = 0;
unsigned long photoShownAt = 0;

// -------- V5.0 SILENT VIDEO PLAYER --------
// Custom .evm container: 16-byte little-endian header:
// magic "ERXV" | width u16 | height u16 | fps u16 | reserved u16 | frameCount u32
// Followed by frameCount entries: JPEG size u32 + baseline JPEG bytes.
// No audio. The PC converter creates this format from ordinary video files.
static const uint8_t MAX_VIDEOS = 24;
static const uint8_t VIDEOS_PER_PAGE = 4;
static const uint32_t VIDEO_MAX_JPEG = 128UL * 1024UL; // allocated in PSRAM
static const unsigned long VIDEO_UI_TIMEOUT_MS = 3000UL;
String videoFiles[MAX_VIDEOS];
uint8_t videoCount = 0;
uint8_t videoPage = 0;
uint8_t videoIndex = 0;
fs::File activeVideo;
uint8_t* videoFrameBuffer = nullptr;
uint16_t videoWidth = 0, videoHeight = 0, videoFps = 7;
uint32_t videoFrameCount = 0, videoFrameNumber = 0;
uint32_t videoFramesDecoded = 0;
uint32_t videoStartMs = 0;
uint32_t videoNextDue = 0;
uint16_t videoFramePeriod = 143;
bool videoPlaying = false;
bool videoLoop = true;
bool videoUIVisible = true;
uint32_t videoUIShownMs = 0;
uint32_t videoLastFrameDurationMs = 0;
String videoError = "";

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
  WiFi.mode(WIFI_STA);
  preferences.begin("erinxx-net", true);
  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("pass", "");
  preferences.end();
  if (savedSSID.length() > 0) {
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    lastWifiRetryMs = millis();
    wifiMessage = "Connecting saved network";
    Serial.println("Wi-Fi: reconnecting saved network");
  } else {
    wifiMessage = "Open Settings > Wi-Fi Setup";
    Serial.println("Wi-Fi: no saved network. Use the on-screen setup.");
  }
}


void updateWiFiClock() {
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
      String(WEATHER_PLACES[selectedWeatherPlace].lat, 4) + "&longitude=" +
      String(WEATHER_PLACES[selectedWeatherPlace].lon, 4) +
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
                currentCity(), weather.temperature, weather.humidity,
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
  if (currentPage != PAGE_WEATHER || weatherDropdownOpen) return;
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
  label("SMART DISPLAY  /  V5.1", 16, 44, C_MUTED, 1);
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


void drawWeather(); // explicit forward declaration for ordinary C++ lint

void drawWeatherDropdown() {
  // Interactive city dropdown overlay, with ASCII 'v' for GLCD compatibility.
  tft.fillRoundRect(15, 71, 290, 40, 10, C_PANEL2);
  tft.drawRoundRect(15, 71, 290, 40, 10, C_BORDER);
  label(String(currentCity()) + ", Batangas", 27, 83, C_WHITE, 2);
  label(weatherDropdownOpen ? "^" : "v", 281, 83, C_CYAN, 2);
  if (!weatherDropdownOpen) return;
  tft.fillRoundRect(15, 113, 290, 208, 9, C_BG);
  tft.drawRoundRect(15, 113, 290, 208, 9, C_BORDER);
  for (int i = 0; i < 4; i++) {
    int y = 121 + i * 49;
    bool active = i == selectedWeatherPlace;
    tft.fillRoundRect(21, y, 278, 43, 7, active ? C_BLUE : C_PANEL);
    label(WEATHER_PLACES[i].name, 32, y + 13, C_WHITE, 2);
    if (active) label("OK", 263, y + 15, C_CYAN, 1);
  }
}

void selectWeatherPlace(uint8_t index) {
  if (index >= 4) return;
  weatherDropdownOpen = false;
  if (index != selectedWeatherPlace) {
    selectedWeatherPlace = index;
    preferences.begin("erinxx-ui", false);
    preferences.putUChar("city", selectedWeatherPlace);
    preferences.end();
    weather = WeatherState(); // no stale readings from previous city
    weatherAttempted = false; // request selected city on next loop
    lastWeatherAttemptMs = 0;
    Serial.printf("Weather location changed: %s\n", currentCity());
  }
  drawWeather();
}

void drawWeather() {
  drawTop("WEATHER");
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
    label(savedSSID.length() == 0 && WiFi.status() != WL_CONNECTED ?
          "Set up Wi-Fi in SETTINGS" : "Waiting for weather data...", 27, 352, C_YELLOW, 1);
    label(WiFi.status() == WL_CONNECTED ? "Wi-Fi connected" : "Wi-Fi offline", 27, 379, C_MUTED, 1);
  }
  drawFooter();
  drawWeatherDropdown();
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


// JPEG callback: TJpgDec sends small RGB565 pixel blocks directly to TFT.
// The decoder's byte-swap setting is configured once in setup().
bool photoOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (x >= SCREEN_W || y >= SCREEN_H) return false;
  tft.pushImage(x, y, w, h, bitmap);  // TFT_eSPI automatically clips to screen
  return true;
}

// Scan /photos only, avoiding the SD icon assets and any other directories.
// Supported: .jpg and .jpeg (baseline RGB JPEG, not progressive).
void refreshPhotoList() {
  photoCount = 0;
  if (!sdReady) return;
  fs::File dir = SD_MMC.open("/photos");
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    Serial.println("PHOTOS: /photos folder missing");
    return;
  }
  fs::File f = dir.openNextFile();
  while (f && photoCount < MAX_PHOTOS) {
    if (!f.isDirectory()) {
      String name = String(f.name());
      if (acceptedMedia(name, false)) {
        const char* path = f.path();
        String fullPath = path ? String(path) : "";
        if (!fullPath.startsWith("/photos/")) {
          const int slash = name.lastIndexOf('/');
          fullPath = String("/photos/") + (slash >= 0 ? name.substring(slash + 1) : name);
        }
        photoFiles[photoCount++] = fullPath;
      }
    }
    f.close();
    f = dir.openNextFile();
  }
  dir.close();
  // Stable filename sorting helps manual next/previous navigation.
  for (uint8_t i = 0; i < photoCount; i++) {
    for (uint8_t j = i + 1; j < photoCount; j++) {
      if (photoFiles[j].compareTo(photoFiles[i]) < 0) {
        String temp = photoFiles[i];
        photoFiles[i] = photoFiles[j];
        photoFiles[j] = temp;
      }
    }
  }
  Serial.printf("PHOTOS: %u JPEG files in /photos (maximum %u)\n", photoCount, MAX_PHOTOS);
}

String photoName(uint8_t i) {
  if (i >= photoCount) return "";
  String full = photoFiles[i];
  const int slash = full.lastIndexOf('/');
  return slash >= 0 ? full.substring(slash + 1) : full;
}

void drawGallery() {
  currentPage = PAGE_GALLERY;
  drawTop("GALLERY");
  label("DIGITAL PHOTO FRAME", 20, 82, C_CYAN, 2);
  if (!sdReady) {
    drawInfoBox(150, "SD OFFLINE", "Insert FAT32 microSD", "Restart the ESP32-S3");
    drawFooter();
    return;
  }
  if (!photoCount) {
    tft.fillRoundRect(14, 135, 292, 220, 12, C_PANEL);
    label("NO JPEG PHOTOS FOUND", 26, 166, C_WHITE, 2);
    label("Create /photos on your SD card", 26, 207, C_CYAN, 1);
    label("Copy JPG images inside /photos", 26, 232, C_MUTED, 1);
    label("Use baseline JPG, 320 x 480", 26, 258, C_MUTED, 1);
    label("Then restart the ESP32-S3", 26, 286, C_MUTED, 1);
    drawFooter();
    return;
  }
  const uint8_t pages = (photoCount + PHOTOS_PER_PAGE - 1) / PHOTOS_PER_PAGE;
  if (galleryPage >= pages) galleryPage = pages - 1;
  char header[42];
  snprintf(header, sizeof(header), "%u PHOTOS      PAGE %u/%u", photoCount, galleryPage + 1, pages);
  label(header, 20, 112, C_MUTED, 1);
  for (uint8_t row = 0; row < PHOTOS_PER_PAGE; row++) {
    uint8_t index = galleryPage * PHOTOS_PER_PAGE + row;
    if (index >= photoCount) break;
    int y = 136 + row * 62;
    tft.fillRoundRect(14, y, 292, 55, 10, C_PANEL);
    tft.drawRoundRect(14, y, 292, 55, 10, C_BORDER);
    label(String(index + 1), 25, y + 16, C_CYAN, 2);
    String name = photoName(index);
    if (name.length() > 27) name = name.substring(0, 24) + "...";
    label(name, 64, y + 20, C_WHITE, 1);
    label(">", 283, y + 19, C_CYAN, 1);
  }
  tft.fillRoundRect(12, 399, 87, 40, 9, C_PANEL2);
  tft.fillRoundRect(108, 399, 104, 40, 9, C_BLUE);
  tft.fillRoundRect(221, 399, 87, 40, 9, C_PANEL2);
  label("PREV", 29, 413, C_WHITE, 1);
  label("FRAME", 141, 413, C_WHITE, 1);
  label("NEXT", 239, 413, C_WHITE, 1);
  drawFooter();
}

// PHOTO viewer fills the screen with no forced header/footer.
// Images are centered with letterboxing (best result: convert to 320x480).
// Viewer controls can disappear; touching the image reveals them again.
void renderCurrentPhoto(bool showControls) {
  currentPage = PAGE_PHOTO;
  photoControlsVisible = showControls;
  tft.fillScreen(TFT_BLACK);
  if (!photoCount || photoIndex >= photoCount) {
    label("NO PHOTO SELECTED", 26, 200, C_WHITE, 2);
    return;
  }
  uint16_t w = 0, h = 0;
  const String path = photoFiles[photoIndex];
  JRESULT info = TJpgDec.getFsJpgSize(&w, &h, path.c_str(), SD_MMC);
  if (info == JDR_OK && w && h) {
    uint8_t scale = 1;
    while (scale < 8 && (w > SCREEN_W * scale || h > SCREEN_H * scale)) scale *= 2;
    if (w <= SCREEN_W * scale && h <= SCREEN_H * scale) {
      TJpgDec.setJpgScale(scale);
      int dw = (w + scale - 1) / scale;
      int dh = (h + scale - 1) / scale;
      int dx = (SCREEN_W - dw) / 2;
      int dy = (SCREEN_H - dh) / 2;
      JRESULT status = TJpgDec.drawFsJpg(dx, dy, path.c_str(), SD_MMC);
      if (status != JDR_OK) {
        Serial.printf("JPEG DECODING FAILED: %u, %s\n", (unsigned)status, path.c_str());
        tft.fillScreen(C_BG);
        label("JPEG DECODE FAILED", 24, 198, C_WHITE, 2);
        label("Use baseline JPG format", 24, 236, C_CYAN, 1);
      } else {
        Serial.printf("PHOTO: %s [%ux%u] scale 1/%u\n", path.c_str(), w, h, scale);
      }
    } else {
      label("IMAGE TOO LARGE", 24, 195, C_WHITE, 2);
      label("Resize to 320x480 on PC", 24, 234, C_CYAN, 1);
    }
  } else {
    Serial.printf("JPEG HEADER FAILED: %u, %s\n", (unsigned)info, path.c_str());
    label("UNSUPPORTED JPEG", 24, 195, C_WHITE, 2);
    label("Use baseline RGB JPEG", 24, 234, C_CYAN, 1);
  }
  if (photoControlsVisible) {
    tft.fillRoundRect(6, 6, 73, 34, 9, C_PANEL);
    label("< BACK", 16, 18, C_WHITE, 1);
    tft.fillRect(0, 421, SCREEN_W, 59, C_PANEL);
    tft.drawFastHLine(0, 421, 320, C_BORDER);
    label("< PREV", 14, 439, C_WHITE, 1);
    label(slideShow ? "PAUSE" : "PLAY", 137, 439, C_CYAN, 1);
    label("NEXT >", 246, 439, C_WHITE, 1);
    char counter[26];
    snprintf(counter, sizeof(counter), "%u / %u", photoIndex + 1, photoCount);
    label(counter, 140, 424, C_MUTED, 1);
    photoControlsShownAt = millis();
  }
}

void openPhoto(uint8_t index, bool autoPlay) {
  if (!photoCount) return;
  photoIndex = index % photoCount;
  slideShow = autoPlay;
  photoShownAt = millis();
  renderCurrentPhoto(true);
}

void goPhoto(int direction) {
  if (!photoCount) return;
  photoIndex = (photoIndex + photoCount + direction) % photoCount;
  photoShownAt = millis();
  renderCurrentPhoto(photoControlsVisible);
}

void touchGallery(int x, int y) {
  if (y >= 136 && y < 384) {
    const int row = (y - 136) / 62;
    const int within = (y - 136) % 62;
    const int index = galleryPage * PHOTOS_PER_PAGE + row;
    if (within < 55 && index < photoCount) openPhoto(index, false);
    return;
  }
  if (y >= 399 && y < 439) {
    const uint8_t pages = (photoCount + PHOTOS_PER_PAGE - 1) / PHOTOS_PER_PAGE;
    if (x < 100 && galleryPage > 0) { galleryPage--; drawGallery(); }
    else if (x > 220 && galleryPage + 1 < pages) { galleryPage++; drawGallery(); }
    else if (x >= 108 && x < 212 && photoCount) { openPhoto(0, true); }
  }
}

void touchPhoto(int x, int y) {
  if (x < 85 && y < 55) {
    slideShow = false;
    drawGallery();
    return;
  }
  if (!photoControlsVisible) {
    photoControlsVisible = true;
    renderCurrentPhoto(true);
    return;
  }
  if (y >= 420) {
    if (x < 105) { goPhoto(-1); return; }
    if (x > 215) { goPhoto(1); return; }
    slideShow = !slideShow;
    photoShownAt = millis();
    renderCurrentPhoto(true);
    return;
  }
  // Tap elsewhere: hide controls immediately, redraw JPEG to restore image.
  renderCurrentPhoto(false);
}


// -------- V5.0 VIDEO: browser + silent frame-by-frame JPEG playback --------
void closeVideo() {
  if (activeVideo) activeVideo.close();
  videoPlaying = false;
  videoFrameCount = 0;
  videoFrameNumber = 0;
}

String videoName(uint8_t index) {
  if (index >= videoCount) return "";
  String filename = videoFiles[index];
  int sep = filename.lastIndexOf('/');
  return sep >= 0 ? filename.substring(sep + 1) : filename;
}

void refreshVideoList() {
  videoCount = 0;
  if (!sdReady) return;
  fs::File dir = SD_MMC.open("/videos");
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    Serial.println("VIDEOS: /videos missing");
    return;
  }
  fs::File file = dir.openNextFile();
  while (file && videoCount < MAX_VIDEOS) {
    if (!file.isDirectory()) {
      String name = String(file.name());
      String lower = name;
      lower.toLowerCase();
      if (lower.endsWith(".evm")) {
        const char* filePath = file.path();
        String path = filePath ? String(filePath) : String("");
        if (!path.startsWith("/videos/")) {
          int slash = name.lastIndexOf('/');
          path = String("/videos/") + (slash >= 0 ? name.substring(slash + 1) : name);
        }
        videoFiles[videoCount++] = path;
      }
    }
    file.close();
    file = dir.openNextFile();
  }
  dir.close();
  for (uint8_t i = 0; i < videoCount; ++i) {
    for (uint8_t j = i + 1; j < videoCount; ++j) {
      if (videoFiles[j].compareTo(videoFiles[i]) < 0) {
        String temp = videoFiles[i]; videoFiles[i] = videoFiles[j]; videoFiles[j] = temp;
      }
    }
  }
  Serial.printf("VIDEOS: %u EVM file(s) in /videos\n", videoCount);
}

void drawVideoList() {
  currentPage = PAGE_VIDEO;
  drawTop("VIDEO");
  label("ANIMATED PORTRAIT", 19, 86, C_CYAN, 2);
  label("Silent MJPEG-frame clips from SD", 19, 112, C_MUTED, 1);
  if (!sdReady) {
    drawInfoBox(162, "SD OFFLINE", "Insert FAT32 microSD", "Then restart the device");
    drawFooter();
    return;
  }
  if (videoCount == 0) {
    tft.fillRoundRect(14, 146, 292, 218, 12, C_PANEL);
    label("NO VIDEOS FOUND", 28, 190, C_WHITE, 2);
    label("Copy .evm files to /videos", 28, 230, C_CYAN, 1);
    label("Use convert_videos.py on PC", 28, 251, C_MUTED, 1);
    drawFooter();
    return;
  }
  const uint8_t pages = (videoCount + VIDEOS_PER_PAGE - 1) / VIDEOS_PER_PAGE;
  if (videoPage >= pages) videoPage = pages - 1;
  char heading[42];
  snprintf(heading, sizeof(heading), "%u CLIPS   PAGE %u/%u", videoCount, videoPage+1, pages);
  label(heading, 20, 127, C_MUTED, 1);
  for (uint8_t row=0; row<VIDEOS_PER_PAGE; ++row) {
    uint8_t idx = videoPage*VIDEOS_PER_PAGE + row;
    if (idx >= videoCount) break;
    int y = 150 + row * 61;
    tft.fillRoundRect(14, y, 292, 54, 9, C_PANEL);
    tft.drawRoundRect(14, y, 292, 54, 9, C_BORDER);
    label(String(idx+1), 25, y+16, C_CYAN, 2);
    String name = videoName(idx);
    if (name.length()>28) name = name.substring(0,25) + "...";
    label(name, 61, y+20, C_WHITE, 1);
    label(">", 284, y+20, C_CYAN, 1);
  }
  tft.fillRoundRect(12, 404, 130, 38, 9, C_PANEL2);
  tft.fillRoundRect(178, 404, 130, 38, 9, C_PANEL2);
  label("< PAGE", 43, 418, C_WHITE, 1);
  label("PAGE >", 220, 418, C_WHITE, 1);
  drawFooter();
}

bool readVideoU16(uint16_t& result) {
  int a=activeVideo.read(), b=activeVideo.read();
  if (a<0 || b<0) return false;
  result = (uint16_t)a | ((uint16_t)b<<8);
  return true;
}

bool readVideoU32(uint32_t& result) {
  uint8_t b[4];
  if (activeVideo.read(b,4) != 4) return false;
  result = (uint32_t)b[0] | ((uint32_t)b[1]<<8) |
           ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
  return true;
}

void videoOverlay(bool resetTimer = true) {
  // Overlays are refreshed only on user actions, or after JPEG frame updates.
  if (!videoUIVisible) return;
  tft.fillRoundRect(6, 5, 71, 36, 9, C_PANEL);
  label("< BACK", 15, 17, C_WHITE, 1);
  tft.fillRect(0, 420, 320, 60, C_PANEL);
  tft.drawFastHLine(0, 420, 320, C_BORDER);
  label("< PREV", 10, 443, C_WHITE, 1);
  label(videoPlaying ? "PAUSE" : "PLAY", 95, 443, C_CYAN, 1);
  label("NEXT >", 177, 443, C_WHITE, 1);
  label(videoLoop ? "LOOP" : "ONCE", 266, 443, C_WHITE, 1);
  char details[52];
  snprintf(details, sizeof(details), "%lu/%lu  %u FPS", (unsigned long)videoFrameNumber,
           (unsigned long)videoFrameCount, videoFps);
  label(details, 110, 426, C_MUTED, 1);
  if (resetTimer) videoUIShownMs = millis();
}

void drawVideoPlayerShell() {
  currentPage = PAGE_VIDEO_PLAYER;
  tft.fillScreen(TFT_BLACK);
  if (videoError.length()) {
    label("VIDEO ERROR", 50, 160, C_WHITE, 2);
    String msg = videoError;
    if (msg.length() > 38) msg = msg.substring(0,38);
    label(msg, 24, 206, C_CYAN, 1);
    videoUIVisible = true;
    videoOverlay();
    return;
  }
  videoUIVisible = true;
  videoOverlay();
}

bool startVideo(uint8_t index) {
  if (!videoCount || !sdReady) return false;
  closeVideo();
  videoError = "";
  videoIndex = index % videoCount;
  if (!videoFrameBuffer) {
    videoFrameBuffer = (uint8_t*)heap_caps_malloc(VIDEO_MAX_JPEG, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!videoFrameBuffer) {
      videoError = "PSRAM unavailable (set OPI PSRAM)";
      Serial.println("VIDEO ERROR: PSRAM buffer unavailable");
      drawVideoPlayerShell();
      return false;
    }
  }
  activeVideo = SD_MMC.open(videoFiles[videoIndex], FILE_READ);
  if (!activeVideo) {
    videoError = "Cannot open SD video file";
    drawVideoPlayerShell();
    return false;
  }
  uint8_t magic[4];
  uint16_t reserved=0;
  if (activeVideo.read(magic,4) != 4 || memcmp(magic,"ERXV",4)!=0 ||
      !readVideoU16(videoWidth) || !readVideoU16(videoHeight) ||
      !readVideoU16(videoFps) || !readVideoU16(reserved) ||
      !readVideoU32(videoFrameCount) ||
      videoWidth==0 || videoHeight==0 || videoWidth>320 || videoHeight>480 ||
      videoFps<1 || videoFps>15 || videoFrameCount<1 || videoFrameCount>100000UL) {
    closeVideo();
    videoError = "Invalid .evm file/header";
    Serial.println("VIDEO ERROR: invalid header");
    drawVideoPlayerShell();
    return false;
  }
  videoFramePeriod = 1000UL / videoFps;
  videoPlaying = true;
  videoFrameNumber = 0;
  videoNextDue = millis();
  videoFramesDecoded = 0;
  videoStartMs = millis();
  Serial.printf("VIDEO START %s: %ux%u @ %u FPS, %lu frames\n",
       videoFiles[videoIndex].c_str(), videoWidth, videoHeight, videoFps,
       (unsigned long)videoFrameCount);
  drawVideoPlayerShell();
  return true;
}

void restartVideo() {
  if (!activeVideo || !activeVideo.seek(16)) {
    videoPlaying = false;
    videoError = "Failed to restart video";
    Serial.println("VIDEO RESTART ERROR");
    return;
  }
  videoFrameNumber = 0;
  videoNextDue = millis();
}

void renderNextVideoFrame() {
  if (currentPage != PAGE_VIDEO_PLAYER || !videoPlaying || !activeVideo) return;
  uint32_t now = millis();
  if ((int32_t)(now-videoNextDue) < 0) return;
  if (videoFrameNumber >= videoFrameCount) {
    if (videoLoop) restartVideo();
    else { videoPlaying=false; videoUIVisible=true; videoOverlay(); }
    return;
  }
  uint32_t jpegLength=0;
  if (!readVideoU32(jpegLength) || jpegLength<100 || jpegLength>VIDEO_MAX_JPEG ||
      activeVideo.position()>activeVideo.size() ||
      (uint32_t)(activeVideo.size()-activeVideo.position()) < jpegLength) {
    videoPlaying = false;
    videoError = "Bad or oversized JPEG frame";
    Serial.println("VIDEO ERROR: bad frame length");
    videoUIVisible=true;
    drawVideoPlayerShell();
    return;
  }
  if (activeVideo.read(videoFrameBuffer,jpegLength) != jpegLength ||
      videoFrameBuffer[0]!=0xFF || videoFrameBuffer[1]!=0xD8) {
    videoPlaying=false;
    videoError="SD read error";
    videoUIVisible=true;
    drawVideoPlayerShell();
    return;
  }
  // TJpgDec callback already set for photos; also renders video frames.
  TJpgDec.setJpgScale(1);
  const int x = (320-(int)videoWidth)/2;
  const int y = (480-(int)videoHeight)/2;
  uint32_t decodeStart = millis();
  JRESULT result = TJpgDec.drawJpg(x,y,videoFrameBuffer,jpegLength);
  videoLastFrameDurationMs = millis()-decodeStart;
  if (result != JDR_OK) {
    videoPlaying=false;
    videoError=String("JPEG decode error ")+String((unsigned)result);
    Serial.printf("VIDEO ERROR: decode %u\n",(unsigned)result);
    drawVideoPlayerShell();
    return;
  }
  videoFrameNumber++;
  videoFramesDecoded++;
  // Sync next frame relative to completion rather than trying to catch up
  // (which can stall the touchscreen when decoding is slower than target FPS).
  videoNextDue = millis() + (videoLastFrameDurationMs < videoFramePeriod ?
                                   videoFramePeriod-videoLastFrameDurationMs : 1UL);
  if (videoUIVisible && (videoHeight>=420 || (videoFrameNumber % videoFps)==0)) videoOverlay(false);
  if (videoFrameNumber >= videoFrameCount && videoLoop) {
    // Restart next iteration, keep final frame displayed for one frame interval.
  }
}

void onVideoListTouch(int x,int y) {
  if (y>=150 && y<394) {
    int row = (y-150)/61;
    int within = (y-150)%61;
    int index = videoPage*VIDEOS_PER_PAGE + row;
    if (within<54 && index<videoCount) startVideo(index);
    return;
  }
  if (y>=404 && y<442 && videoCount) {
    uint8_t pages=(videoCount+VIDEOS_PER_PAGE-1)/VIDEOS_PER_PAGE;
    if (x<150 && videoPage>0) { videoPage--; drawVideoList(); }
    else if (x>170 && videoPage+1<pages) { videoPage++; drawVideoList(); }
  }
}

void onVideoPlayerTouch(int x,int y) {
  if (x<80 && y<55) {
    closeVideo(); drawVideoList(); return;
  }
  if (!videoUIVisible) {
    videoUIVisible=true;
    videoOverlay();
    return;
  }
  if (y>=420) {
    if (x<80) { startVideo((videoIndex+videoCount-1)%videoCount); return; }
    if (x<160) {
      if (videoPlaying) {
        videoPlaying=false;
      } else {
        if (videoFrameNumber>=videoFrameCount && activeVideo) restartVideo();
        videoPlaying=activeVideo && !videoError.length();
      }
      videoNextDue=millis();
    } else if (x<240) { startVideo((videoIndex+1)%videoCount); return; }
    else { videoLoop=!videoLoop; }
    videoOverlay();
  } else {
    videoUIVisible=false;
    // Clean the control bar without erasing the current frame;
    // overlapping JPEG pixels are restored when the next frame is decoded.
    tft.fillRect(0,420,320,60,TFT_BLACK);
    tft.fillRect(0,0,85,50,TFT_BLACK);
    videoUIShownMs=millis();
  }
}


// -------- V5.1 WI-FI SETUP: SCAN, PICK, TYPE, CONNECT, SAVE --------
void scanNearbyWiFi() {
  wifiCount = 0;
  wifiListPage = 0;
  wifiMessage = "Scanning nearby Wi-Fi...";
  WiFi.mode(WIFI_STA);
  WiFi.scanDelete();
  wifiScanning = true;
  wifiScanStarted = millis();
  int16_t rc = WiFi.scanNetworks(true, false); // async, hide hidden SSIDs
  if (rc == WIFI_SCAN_FAILED) {
    wifiScanning = false;
    wifiMessage = "Scan failed. Tap SCAN.";
  }
}

void drawWifiList() {
  drawTop("WI-FI");
  tft.fillRoundRect(15, 76, 290, 46, 10, C_PANEL2);
  label(WiFi.status() == WL_CONNECTED ? "CONNECTED:" : "NETWORK SETUP", 23, 83, C_CYAN, 1);
  if (WiFi.status() == WL_CONNECTED) {
    String current = WiFi.SSID();
    if (current.length() > 24) current = current.substring(0, 24);
    label(current, 23, 100, C_WHITE, 1);
  } else {
    label(wifiMessage.length() > 29 ? wifiMessage.substring(0,29) : wifiMessage,
          23, 101, C_MUTED, 1);
  }
  tft.fillRoundRect(227, 83, 69, 31, 7, C_PANEL);
  label("FORGET", 242, 94, C_CYAN, 1);
  for (int i=0; i<4; ++i) {
    uint8_t idx = wifiListPage*4 + i;
    int y = 132+i*61;
    tft.fillRoundRect(15,y,290,54,8,C_PANEL);
    tft.drawRoundRect(15,y,290,54,8,C_BORDER);
    if (idx < wifiCount) {
      String name = wifiSSIDs[idx];
      if (name.length()>23) name = name.substring(0,20)+"...";
      label(name, 24,y+9,C_WHITE,2);
      label(String(wifiRSSI[idx])+" dBm",24,y+34,C_MUTED,1);
      label(wifiSecured[idx] ? "LOCK" : "OPEN",257,y+34,C_CYAN,1);
    }
  }
  if (wifiCount == 0) {
    label(wifiScanning ? "Scanning..." : "No networks found; tap SCAN", 26,174,C_WHITE,1);
  }
  tft.fillRoundRect(15,391,90,44,8,C_BLUE);
  tft.fillRoundRect(115,391,90,44,8,C_PANEL2);
  tft.fillRoundRect(215,391,90,44,8,C_PANEL2);
  label("SCAN", 36,405,C_WHITE,2);
  label("PREV", 135,405,C_WHITE,2);
  label("NEXT", 236,405,C_WHITE,2);
  drawFooter();
}

void drawPasswordField() {
  tft.fillRoundRect(15,147,290,49,8,C_PANEL2);
  tft.drawRoundRect(15,147,290,49,8,C_BORDER);
  String mask;
  size_t len=passwordInput.length();
  size_t displayLen=len>29?29:len;
  for(size_t i=0;i<displayLen;i++) mask += "*";
  label(mask.length()?mask:String("Enter Wi-Fi password"),23,162,
        mask.length()?C_WHITE:C_MUTED,1);
  label(String(len)+" / 63",252,180,C_MUTED,1);
}

String keyboardRow(int row) {
  if (row==0) return "1234567890";
  if (passwordSymbols) {
    if (row==1) return "@#$%&*-_+=";
    if (row==2) return "!?.,:;()[]";
    return "{}<>^~|/\\";
  }
  if (row==1) return passwordCaps ? "QWERTYUIOP" : "qwertyuiop";
  if (row==2) return passwordCaps ? "ASDFGHJKL" : "asdfghjkl";
  return passwordCaps ? "ZXCVBNM" : "zxcvbnm";
}

int keyboardRowStartX(int row) {
  if (row==2 && !passwordSymbols) return 27;
  if (row==3) return passwordSymbols?42:56;
  return 13;
}

void drawPasswordKeyboard() {
  for (int row=0;row<4;++row) {
    String keys=keyboardRow(row);
    int startX=keyboardRowStartX(row);
    int y=217+row*41;
    for (unsigned i=0;i<keys.length();++i) {
      int x=startX+(int)i*29;
      tft.fillRoundRect(x,y,27,34,5,C_PANEL2);
      String key=""; key += keys[i];
      tft.setTextColor(C_WHITE);
      tft.setTextSize(2);
      tft.drawCentreString(key,x+13,y+8,1);
    }
  }
  const char* names[]={"Aa","#+=","SPACE","DEL","JOIN"};
  int xs[]={10,63,115,190,245};
  int ws[]={49,49,71,51,65};
  for (int i=0;i<5;++i) {
    tft.fillRoundRect(xs[i],390,ws[i],43,6,i==4?C_BLUE:C_PANEL2);
    label(names[i],xs[i]+(i==2?13:8),404,C_WHITE,1);
  }
}

void drawWifiPassword() {
  drawTop("PASSWORD");
  tft.fillRoundRect(15,78,290,59,10,C_PANEL);
  label("NETWORK",25,88,C_CYAN,1);
  String shortSsid=pendingSSID.length()>29?pendingSSID.substring(0,26)+"...":pendingSSID;
  label(shortSsid,25,107,C_WHITE,2);
  drawPasswordField();
  label(wifiMessage.length()>44?wifiMessage.substring(0,44):wifiMessage,16,201,C_MUTED,1);
  drawPasswordKeyboard();
  drawFooter();
}

void beginWifiConnection(const String& ssid, const String& pass) {
  if (!ssid.length()) return;
  pendingSSID=ssid;
  pendingPassword=pass;
  wifiMessage="Connecting...";
  wifiConnecting=true;
  wifiSaveWhenConnected=true;
  wifiConnectStarted=millis();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  WiFi.begin(pendingSSID.c_str(), pendingPassword.c_str());
  Serial.println("Wi-Fi: connection requested (password hidden)");
  if (currentPage==PAGE_WIFI_PASSWORD) drawWifiPassword();
  if (currentPage==PAGE_WIFI_LIST) drawWifiList();
}

void forgetSavedWiFi() {
  preferences.begin("erinxx-net",false);
  preferences.remove("ssid");
  preferences.remove("pass");
  preferences.end();
  savedSSID=""; savedPassword="";
  pendingSSID=""; pendingPassword="";
  wifiConnecting=false;
  WiFi.disconnect(false,true);
  wifiMessage="Saved Wi-Fi forgotten";
  drawWifiList();
}

void wifiScanLoop() {
  if (!wifiScanning) return;
  int16_t result=WiFi.scanComplete();
  if (result==WIFI_SCAN_RUNNING) {
    if(millis()-wifiScanStarted>15000UL) {
      wifiScanning=false;
      WiFi.scanDelete();
      wifiMessage="Scan timeout. Tap SCAN.";
      if(currentPage==PAGE_WIFI_LIST) drawWifiList();
    }
    return;
  }
  wifiScanning=false;
  if (result<0) {
    wifiMessage="Scan failed. Tap SCAN.";
  } else {
    for (int i=0;i<result && wifiCount<MAX_WIFI_NETWORKS;++i) {
      String n=WiFi.SSID(i);
      if (n.length()==0) continue;
      bool duplicate=false;
      for (int j=0;j<wifiCount;++j) if(wifiSSIDs[j]==n) duplicate=true;
      if(duplicate) continue;
      wifiSSIDs[wifiCount]=n;
      wifiRSSI[wifiCount]=WiFi.RSSI(i);
      wifiSecured[wifiCount]=WiFi.encryptionType(i)!=WIFI_AUTH_OPEN;
      ++wifiCount;
    }
    wifiMessage=String(wifiCount)+" network(s) found";
  }
  WiFi.scanDelete();
  if(currentPage==PAGE_WIFI_LIST) drawWifiList();
}

void wifiConnectLoop() {
  if (!wifiConnecting) return;
  if (WiFi.status()==WL_CONNECTED) {
    wifiConnecting=false;
    wifiMessage="Connected successfully";
    ntpConfigured=false;
    weatherAttempted=false;
    if (wifiSaveWhenConnected) {
      preferences.begin("erinxx-net",false);
      preferences.putString("ssid",pendingSSID);
      preferences.putString("pass",pendingPassword);
      preferences.end();
      savedSSID=pendingSSID;
      savedPassword=pendingPassword;
    }
    pendingPassword="";
    passwordInput="";
    if (currentPage==PAGE_WIFI_PASSWORD) currentPage=PAGE_WIFI_LIST;
    if (currentPage==PAGE_WIFI_LIST) drawWifiList();
    Serial.println("Wi-Fi: connected and saved");
  } else if (millis()-wifiConnectStarted>18000UL) {
    wifiConnecting=false;
    pendingPassword="";
    wifiMessage="Failed. Check password, retry.";
    if(currentPage==PAGE_WIFI_PASSWORD) drawWifiPassword();
    if(currentPage==PAGE_WIFI_LIST) drawWifiList();
  }
}

void wifiListTouch(uint16_t x,uint16_t y) {
  if (y>=83 && y<117 && x>=227) {forgetSavedWiFi();return;}
  if (y>=391 && y<439) {
    if (x<109) {scanNearbyWiFi();drawWifiList();}
    else if(x<209) {if(wifiListPage>0) --wifiListPage;drawWifiList();}
    else {if((wifiListPage+1)*4<wifiCount) ++wifiListPage;drawWifiList();}
    return;
  }
  if (y<132 || y>=376) return;
  int row=(y-132)/61;
  if (row<0 || row>3) return;
  uint8_t idx=wifiListPage*4+row;
  if (idx>=wifiCount || wifiConnecting) return;
  pendingSSID=wifiSSIDs[idx];
  if(!wifiSecured[idx]) {beginWifiConnection(pendingSSID,"");return;}
  if(pendingSSID==savedSSID && savedPassword.length()>0) {
    beginWifiConnection(pendingSSID,savedPassword);
    return;
  }
  passwordInput=""; passwordCaps=false;passwordSymbols=false;
  wifiMessage="Enter password, then tap JOIN";
  currentPage=PAGE_WIFI_PASSWORD;
  drawWifiPassword();
}

void wifiPasswordTouch(uint16_t x,uint16_t y) {
  if(wifiConnecting) return;
  if(y>=390 && y<435) {
    if(x<62) {passwordCaps=!passwordCaps;passwordSymbols=false;drawWifiPassword();}
    else if(x<114) {passwordSymbols=!passwordSymbols;drawWifiPassword();}
    else if(x<189) {if(passwordInput.length()<63) passwordInput+=" ";drawPasswordField();}
    else if(x<245) {if(passwordInput.length()) passwordInput.remove(passwordInput.length()-1);drawPasswordField();}
    else {
      if(passwordInput.length()<8) {wifiMessage="Enter at least 8 characters";drawWifiPassword();return;}
      beginWifiConnection(pendingSSID,passwordInput);
    }
    return;
  }
  for(int row=0;row<4;++row) {
    int y0=217+row*41;
    String keys=keyboardRow(row);
    int x0=keyboardRowStartX(row);
    if(y>=y0 && y<y0+34 && x>=x0) {
      int col=(x-x0)/29;
      if(col>=0 && col<(int)keys.length() && (x-x0)%29<27 && passwordInput.length()<63) {
        passwordInput+=keys[col];
        drawPasswordField();
      }
      return;
    }
  }
}

void drawSettings() {
  drawTop("SETTINGS");
  tft.fillRoundRect(15,85,290,120,12,C_PANEL);
  label("DEVICE",27,100,C_CYAN,2);
  label("ESP32-S3 CAM N16R8",27,130,C_WHITE,1);
  label(WiFi.status()==WL_CONNECTED?"Wi-Fi CONNECTED":"Wi-Fi OFFLINE",
        27,154,WiFi.status()==WL_CONNECTED?C_GREEN:C_YELLOW,1);
  label(dhtValid?"DHT11 READY":"DHT11 NO READING",27,180,C_MUTED,1);

  tft.fillRoundRect(15,214,290,93,12,C_PANEL);
  label("MICROSD",27,225,C_CYAN,2);
  if(sdReady) label("READY - "+String((unsigned long)(SD_MMC.cardSize()/1048576ULL))+" MB",
                    27,266,C_WHITE,1);
  else label("OFFLINE",27,266,C_RED,1);

  tft.fillRoundRect(17,315,286,52,10,C_BLUE);
  label("WI-FI SETUP  >",34,334,C_WHITE,2);
  tft.fillRoundRect(17,377,286,52,10,C_PANEL2);
  label("CALIBRATE TOUCH",34,396,C_WHITE,2);
  drawFooter();
}


void showPage(Page page) {
  currentPage = page;
  switch (page) {
    case PAGE_HOME:     drawHome();          break;
    case PAGE_WEATHER:  drawWeather();       break;
    case PAGE_CLOCK:    drawClock();         break;
    case PAGE_CLIMATE:  drawClimate();       break;
    case PAGE_GALLERY:  refreshPhotoList(); galleryPage = 0; drawGallery(); break;
    case PAGE_PHOTO:    renderCurrentPhoto(true); break;
    case PAGE_VIDEO:    closeVideo(); refreshVideoList(); videoPage=0; drawVideoList(); break;
    case PAGE_VIDEO_PLAYER: drawVideoPlayerShell(); break;
    case PAGE_SETTINGS: drawSettings();      break;
    case PAGE_WIFI_LIST: scanNearbyWiFi(); drawWifiList(); break;
    case PAGE_WIFI_PASSWORD: drawWifiPassword(); break;
  }
}

void onTouch(uint16_t x, uint16_t y) {
  lastTouchX = x;
  lastTouchY = y;
  Serial.printf("TOUCH X:%u Y:%u  PAGE:%s\n", x, y, PAGE_LABELS[currentPage]);

  if (currentPage == PAGE_WEATHER && weatherDropdownOpen) {
    if (x>=12 && x<90 && y>=10 && y<59) {
      weatherDropdownOpen=false;
      showPage(PAGE_HOME);
      return;
    }
    if(y>=121 && y<317 && x>=21 && x<299) {
      int selected=(y-121)/49;
      if(selected>=0 && selected<4) selectWeatherPlace(selected);
      return;
    }
    weatherDropdownOpen=false;
    drawWeather();
    return;
  }
  if (currentPage == PAGE_WIFI_LIST) {
    if(x>=12 && x<90 && y<59) {showPage(PAGE_SETTINGS);return;}
    wifiListTouch(x,y); return;
  }
  if (currentPage == PAGE_WIFI_PASSWORD) {
    if(x>=12 && x<90 && y<59) {showPage(PAGE_WIFI_LIST);return;}
    wifiPasswordTouch(x,y); return;
  }
  if (currentPage == PAGE_PHOTO) { touchPhoto(x, y); return; }
  if (currentPage == PAGE_VIDEO_PLAYER) { onVideoPlayerTouch(x, y); return; }
  if (currentPage == PAGE_VIDEO) {
    if (x >= 12 && x < 90 && y >= 10 && y < 59) { showPage(PAGE_HOME); return; }
    onVideoListTouch(x,y); return;
  }
  if (currentPage == PAGE_GALLERY) {
    if (x >= 12 && x < 90 && y >= 10 && y < 59) { showPage(PAGE_HOME); return; }
    touchGallery(x, y);
    return;
  }

  if (currentPage != PAGE_HOME && x >= 12 && x < 90 && y >= 10 && y < 59) {
    showPage(PAGE_HOME);
    return;
  }

  if (currentPage == PAGE_WEATHER && y>=71 && y<113) {
    weatherDropdownOpen=true;
    drawWeatherDropdown();
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
  } else if (currentPage == PAGE_SETTINGS && x>=17 && x<303 && y>=315 && y<368) {
    showPage(PAGE_WIFI_LIST);
  } else if (currentPage == PAGE_SETTINGS && x>=17 && x<303 && y>=377 && y<429) {
    calibrateAndSaveTouch();
    showPage(PAGE_SETTINGS);
  } else {
    updateTouchFooter();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("\n=== ERINX SMART HUB V5.1.2 WIFI + LOCATION ===");

  tft.init();
  tft.setRotation(0); // 320 x 480 PORTRAIT (matches previous working test)
  tft.fillScreen(C_BG);
  label("ERINX V5.1.1 - BOOTING", 24, 170, C_CYAN, 2);
  label("Display initialized", 24, 208, C_WHITE, 1);

  loadOrCalibrateTouch();
  initSD();
  reportIconStatus();
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true); // Required for correct RGB565 colors with TFT_eSPI
  TJpgDec.setCallback(photoOutput);
  Serial.println("PHOTO JPEG DECODER READY - SD_MMC");
  dht.begin();
  Serial.println("DHT11 initialized on GPIO14");
  preferences.begin("erinxx-ui", true);
  selectedWeatherPlace = preferences.getUChar("city", 1);
  preferences.end();
  if (selectedWeatherPlace >= 4) selectedWeatherPlace = 1;
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

  // Video frames get priority over weather HTTP fetches. Video player
  // may decode for a short time per frame; no multitasking claimed.
  if (currentPage == PAGE_VIDEO_PLAYER) {
    renderNextVideoFrame();
    if (videoUIVisible && millis() - videoUIShownMs >= VIDEO_UI_TIMEOUT_MS &&
        !videoError.length()) {
      videoUIVisible=false;
      tft.fillRect(0,420,320,60,TFT_BLACK);
      tft.fillRect(0,0,85,50,TFT_BLACK);
    }
    delay(1);
    return;
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

  wifiScanLoop();
  wifiConnectLoop();
  if (savedSSID.length()>0 && WiFi.status()!=WL_CONNECTED &&
      !wifiConnecting && !wifiScanning &&
      currentPage!=PAGE_WIFI_PASSWORD &&
      now-lastWifiRetryMs>=30000UL) {
    lastWifiRetryMs=now;
    WiFi.begin(savedSSID.c_str(),savedPassword.c_str());
  }

  bool refreshDue = !weatherAttempted ||
      (now - lastWeatherAttemptMs >=
       (weather.valid ? WEATHER_REFRESH_MS : WEATHER_RETRY_MS));
  if (refreshDue && currentPage != PAGE_VIDEO_PLAYER &&
      currentPage != PAGE_WIFI_LIST && currentPage != PAGE_WIFI_PASSWORD &&
      !wifiScanning && !wifiConnecting && WiFi.status() == WL_CONNECTED) {
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
  // Immersive photo viewer: slideshow and timed overlay hiding.
  if (currentPage == PAGE_PHOTO) {
    if (slideShow && photoCount > 1 && millis() - photoShownAt >= SLIDE_INTERVAL_MS) {
      goPhoto(1);
    } else if (photoControlsVisible && millis() - photoControlsShownAt >= PHOTO_CONTROLS_MS) {
      renderCurrentPhoto(false); // restores JPEG pixels under previous controls
    }
  }
  if (currentPage != PAGE_PHOTO) updateTopWifiStatus();
  delay(12);
}
