/*
 * ERINX SMART HUB V3 — PORTRAIT BLUE TOUCH UI
 * Hardware: ESP32 DevKit + 3.5-inch ILI9488 TFT + XPT2046 touch ONLY
 * Screen: 320 x 480 portrait (rotation 2; use 0 if upside-down).
 *
 * TFT_eSPI User_Setup.h must have:
 *   #define ILI9488_DRIVER
 *   #define TFT_MISO 19
 *   #define TFT_MOSI 23
 *   #define TFT_SCLK 18
 *   #define TFT_CS 5
 *   #define TFT_DC 27
 *   #define TFT_RST 33
 *   #define TOUCH_CS 21
 *   #define SPI_FREQUENCY 20000000
 *   #define SPI_TOUCH_FREQUENCY 2500000
 *   #define LOAD_GLCD
 *   #define LOAD_FONT2
 *   #define LOAD_FONT4
 *
 * Touch: T_CLK=18, T_CS=21, T_DIN=23, T_DO=19. LCD SDO unconnected.
 * Keep your ALREADY WORKING VCC, GND and LED/backlight connections.
 *
 * Libraries: TFT_eSPI (Bodmer), ArduinoJson (Benoit Blanchon) v7.
 * WiFi / HTTP / Preferences / time are part of the ESP32 Arduino core.
 *
 * Notes:
 * - This UI is drawn using TFT primitives; NO bitmap file or SD card needed.
 * - Small sprites update the clock and animated weather graphic.
 * - Weather needs Internet; NTP updates time independently.
 * - HTTPS uses setInsecure() ONLY for a personal prototype.
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
#include <math.h>

// Set Wi-Fi credentials LOCALLY. Do not upload credentials to a public repo.
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite clockSprite = TFT_eSprite(&tft);
TFT_eSprite weatherSprite = TFT_eSprite(&tft);
Preferences prefs;

struct Place { const char* name; float lat; float lon; };
const Place cities[] = {
  {"Lipa",       13.9411f, 121.1631f},
  {"Tanauan",    14.0853f, 121.1528f},
  {"Malvar",     14.0444f, 121.1583f},
  {"Sto. Tomas", 14.1080f, 121.1420f}
};
const int CITY_COUNT = sizeof(cities) / sizeof(cities[0]);

// Keep same settings namespace/keys as the previous Smart Hub V2.
uint16_t calibration[5] = {0};
int city = 1;  // Tanauan unless a saved selection exists

enum Page { HOME, LOCATIONS, WEATHER, SETTINGS };
Page page = HOME;

float tempC = 0.0f, apparentC = 0.0f, humidity = 0.0f, windKmh = 0.0f;
int wmo = -1;
bool haveWeather = false;
String weatherError = "Weather not loaded";

unsigned long lastFetchAttempt = 0;
unsigned long lastWeatherOK = 0;
unsigned long lastClockDraw = 0;
unsigned long lastAnim = 0;
unsigned long lastTouch = 0;
unsigned long lastReconnect = 0;
uint16_t spin = 0;

const unsigned long WEATHER_INTERVAL = 10UL * 60UL * 1000UL;
const unsigned long RETRY_INTERVAL   = 60UL * 1000UL;

// Colors tuned for a blue/cyan portrait dashboard (RGB565).
const uint16_t BG       = 0x010C;
const uint16_t BG2      = 0x018F;
const uint16_t CARD     = 0x09D6;
const uint16_t CARD2    = 0x0A16;
const uint16_t BTN      = 0x0A79;
const uint16_t BTN_DARK = 0x09F5;
const uint16_t EDGE     = 0x059F;
const uint16_t SKY      = 0x35DF;
const uint16_t WHITE    = 0xFFFF;
const uint16_t MUTED    = 0x9D5A;
const uint16_t GREEN    = 0x4EEC;
const uint16_t YELLOW   = 0xFE60;
const uint16_t RED      = 0xF9C8;
const uint16_t CLOUD    = 0xDEFB;

// Explicit function declarations prevent Arduino auto-prototype issues.
void drawPage();
void drawHome();
void drawLocations();
void drawWeather();
void drawSettings();
void drawBackground();
void drawTitle(const char* name);
void drawCard(int x, int y, int w, int h, uint16_t color = CARD);
void textAt(const char* str, int x, int y, int font = 2, uint16_t color = WHITE, uint16_t bg = BG);
void centerText(const char* str, int x, int y, int font, uint16_t color, uint16_t bg);
void drawButton(int x, int y, int w, int h, const char* title, int iconType);
void drawPin(int x, int y, uint16_t color);
void drawCloud(int x, int y, uint16_t color);
void drawRefresh(int x, int y, uint16_t color);
void drawGear(int x, int y, uint16_t color);
void drawClock();
void drawWeatherSymbol();
void updateHomeWeatherValues();
void connectWiFi();
bool fetchWeather();
void calibrateTouchscreen();
void handleTouch(int x, int y);
void changeCity(int index);
const char* conditionName(int code);
void reportWeatherStatus(const String& status);

void textAt(const char* str, int x, int y, int font, uint16_t color, uint16_t bg) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(color, bg);
  tft.drawString(str, x, y, font);
}

void centerText(const char* str, int x, int y, int font, uint16_t color, uint16_t bg) {
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(color, bg);
  tft.drawString(str, x, y, font);
  tft.setTextDatum(TL_DATUM);
}

void drawBackground() {
  tft.fillScreen(BG);
  // A restrained blue bottom accent, drawn once per page.
  tft.fillRect(0, 473, 320, 7, BG2);
}

void drawCard(int x, int y, int w, int h, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, 13, color);
  tft.drawRoundRect(x, y, w, h, 13, EDGE);
}

void drawTitle(const char* name) {
  tft.drawLine(12, 21, 34, 21, SKY);
  tft.drawLine(286, 21, 308, 21, SKY);
  tft.fillCircle(34, 21, 2, SKY);
  tft.fillCircle(286, 21, 2, SKY);
  centerText(name, 160, 23, 2, WHITE, BG);
  // Tiny Wi-Fi indicator is drawn below the title so text stays readable.
  if (WiFi.status() == WL_CONNECTED) {
    tft.fillCircle(301, 44, 3, GREEN);
  } else {
    tft.drawCircle(301, 44, 3, RED);
  }
}

// Icon functions: simple primitive shapes, no external graphic assets.
void drawPin(int x, int y, uint16_t color) {
  tft.fillCircle(x, y - 4, 8, color);
  tft.fillTriangle(x - 7, y - 1, x + 7, y - 1, x, y + 12, color);
  tft.fillCircle(x, y - 4, 3, CARD);
}
void drawCloud(int x, int y, uint16_t color) {
  tft.fillCircle(x - 9, y + 3, 7, color);
  tft.fillCircle(x + 1, y - 3, 10, color);
  tft.fillCircle(x + 12, y + 4, 7, color);
  tft.fillRoundRect(x - 16, y + 3, 35, 10, 4, color);
}
void drawRefresh(int x, int y, uint16_t color) {
  // Circular ring with small arrow head.
  tft.drawCircle(x, y, 11, color);
  tft.drawCircle(x, y, 10, color);
  tft.fillRect(x + 8, y - 11, 7, 8, BTN);
  tft.fillTriangle(x + 7, y - 11, x + 17, y - 10, x + 13, y - 1, color);
}
void drawGear(int x, int y, uint16_t color) {
  for (int a = 0; a < 360; a += 45) {
    float rad = a * 0.0174532925f;
    tft.drawLine(x + cosf(rad)*9, y + sinf(rad)*9,
                 x + cosf(rad)*14, y + sinf(rad)*14, color);
  }
  tft.fillCircle(x, y, 10, color);
  tft.fillCircle(x, y, 4, BTN);
}

// iconType: 0 location, 1 weather, 2 refresh, 3 settings
void drawButton(int x, int y, int w, int h, const char* title, int iconType) {
  tft.fillRoundRect(x, y, w, h, 13, BTN);
  tft.drawRoundRect(x, y, w, h, 13, EDGE);
  tft.fillCircle(x + 29, y + h/2, 20, BTN_DARK);
  const int iconX = x + 29, iconY = y + h/2;
  if (iconType == 0) drawPin(iconX, iconY, SKY);
  if (iconType == 1) drawCloud(iconX, iconY, SKY);
  if (iconType == 2) drawRefresh(iconX, iconY, SKY);
  if (iconType == 3) drawGear(iconX, iconY, SKY);
  centerText(title, x + 95, y + h/2, 2, WHITE, BTN);
}

void drawHome() {
  drawTitle("ERINX SMART HUB");

  // Top time card (replaces old clock footer).
  drawCard(12, 53, 296, 94, CARD);
  tft.drawCircle(34, 75, 9, SKY);
  tft.drawLine(34, 75, 34, 69, SKY);
  tft.drawLine(34, 75, 39, 78, SKY);
  textAt("PHILIPPINE TIME", 51, 67, 2, SKY, CARD);
  drawClock();

  // Big weather card: location + temperature/humidity + icon.
  drawCard(12, 157, 296, 177, CARD);
  drawPin(30, 181, SKY);
  textAt(cities[city].name, 48, 172, 4, WHITE, CARD);
  updateHomeWeatherValues();
  drawWeatherSymbol();

  // Four working touch controls.
  drawButton(12, 344, 144, 56, "LOCATION", 0);
  drawButton(164, 344, 144, 56, "WEATHER", 1);
  drawButton(12, 407, 144, 56, "REFRESH", 2);
  drawButton(164, 407, 144, 56, "SETTINGS", 3);
}

void updateHomeWeatherValues() {
  if (page != HOME) return;
  // Restrict erasing to the small text area, never the icon region.
  tft.fillRect(25, 205, 162, 120, CARD);
  if (!haveWeather) {
    textAt("Weather unavailable", 27, 213, 2, YELLOW, CARD);
    String msg = weatherError;
    if (msg.length() > 23) msg = msg.substring(0, 23);
    textAt(msg.c_str(), 27, 239, 1, MUTED, CARD);
    textAt("-- C", 27, 267, 4, WHITE, CARD);
    textAt("RH --%", 27, 305, 2, MUTED, CARD);
    return;
  }
  textAt(conditionName(wmo), 27, 211, 2, SKY, CARD);
  char s[40];
  snprintf(s, sizeof(s), "%.1f C", tempC);
  textAt(s, 27, 257, 4, WHITE, CARD);
  snprintf(s, sizeof(s), "Humidity %.0f%%", humidity);
  textAt(s, 27, 298, 2, MUTED, CARD);
  // Current weather data is from Open-Meteo, not a room sensor.
}

void drawClock() {
  if (page != HOME) return;
  time_t now = time(nullptr);
  if (!clockSprite.created()) {
    // Draw directly if sprite RAM allocation fails.
    tft.fillRect(24, 96, 270, 41, CARD);
    if (now < 1700000000) {
      centerText("WAITING FOR TIME", 160, 115, 2, YELLOW, CARD);
    } else {
      struct tm tmFallback;
      localtime_r(&now, &tmFallback);
      char timeFallback[24];
      strftime(timeFallback, sizeof(timeFallback), "%I:%M:%S %p", &tmFallback);
      centerText(timeFallback, 160, 115, 4, WHITE, CARD);
    }
    return;
  }
  clockSprite.fillSprite(CARD);
  clockSprite.setTextDatum(MC_DATUM);
  if (now < 1700000000) {
    clockSprite.setTextColor(YELLOW, CARD);
    clockSprite.drawString("WAITING FOR TIME", 135, 18, 2);
  } else {
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    char timeString[20], ampm[6];
    strftime(timeString, sizeof(timeString), "%I:%M:%S", &tmNow);
    strftime(ampm, sizeof(ampm), "%p", &tmNow);
    clockSprite.setTextColor(WHITE, CARD);
    clockSprite.drawString(timeString, 114, 19, 4);
    clockSprite.setTextColor(SKY, CARD);
    clockSprite.drawString(ampm, 237, 20, 2);
  }
  clockSprite.pushSprite(24, 96);
}

const char* conditionName(int code) {
  if (code == 0) return "Clear sky";
  if (code == 1) return "Mainly clear";
  if (code == 2) return "Partly cloudy";
  if (code == 3) return "Overcast";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Rain showers";
  if (code >= 95) return "Thunderstorm";
  return "Weather update";
}

// Render one weather glyph into a 16-bit sprite, then push only the small area.
void drawWeatherSymbol() {
  if (page != HOME || !weatherSprite.created()) return;
  weatherSprite.fillSprite(CARD);
  const int x = 54, y = 42;
  const bool unknown = !haveWeather;
  const bool rainy = haveWeather && ((wmo >= 51 && wmo <= 67) || (wmo >= 80 && wmo <= 82));
  const bool snowy = haveWeather && (wmo >= 71 && wmo <= 77);
  const bool foggy = haveWeather && (wmo == 45 || wmo == 48);
  const bool storm = haveWeather && wmo >= 95;
  const bool cloudOnly = haveWeather && wmo == 3;
  const bool partlyCloudy = haveWeather && (wmo == 1 || wmo == 2);
  const bool sunny = haveWeather && wmo >= 0 && wmo <= 2;

  if (sunny) {
    for (int i = 0; i < 8; i++) {
      float angle = (spin + i*45) * 0.0174532925f;
      int xa = x + (int)(cosf(angle)*22);
      int ya = y + (int)(sinf(angle)*22);
      int xb = x + (int)(cosf(angle)*31);
      int yb = y + (int)(sinf(angle)*31);
      weatherSprite.drawLine(xa, ya, xb, yb, YELLOW);
    }
    weatherSprite.fillCircle(x, y, 18, YELLOW);
  }

  // Clouds sit below or in front of the sun. No fake sunshine if API failed.
  const int cloudShift = (spin / 25) % 5;
  if (unknown || cloudOnly || partlyCloudy || rainy || snowy || storm || foggy ||
      (haveWeather && wmo > 3)) {
    int cx = 15 + cloudShift;
    weatherSprite.fillCircle(cx+18, 65, 16, CLOUD);
    weatherSprite.fillCircle(cx+43, 57, 22, CLOUD);
    weatherSprite.fillCircle(cx+69, 66, 15, CLOUD);
    weatherSprite.fillRoundRect(cx+5, 64, 79, 19, 8, CLOUD);
  }
  if (rainy || storm) {
    for (int i = 0; i < 3; i++) {
      int xx = 36 + i*19;
      int yy = 83 + ((spin/8+i*5)%9);
      weatherSprite.drawLine(xx, yy, xx-3, yy+7, SKY);
    }
  }
  if (snowy) {
    for (int i = 0; i < 3; i++) {
      int xx = 35 + i*19;
      int yy = 86 + ((spin/12+i*6)%10);
      weatherSprite.fillCircle(xx, yy, 2, WHITE);
    }
  }
  if (foggy) {
    weatherSprite.drawLine(26, 87, 89, 87, MUTED);
    weatherSprite.drawLine(35, 96, 95, 96, MUTED);
  }
  if (storm && ((spin/5)%2 == 0)) {
    weatherSprite.fillTriangle(55, 80, 44, 95, 57, 92, YELLOW);
    weatherSprite.fillTriangle(54, 92, 49, 102, 67, 87, YELLOW);
  }
  weatherSprite.pushSprite(192, 202);
}

void drawLocations() {
  drawTitle("CHOOSE LOCATION");
  textAt("Tap a city to load weather", 17, 73, 2, MUTED, BG);
  for (int i = 0; i < CITY_COUNT; i++) {
    int y = 115 + i * 70;
    uint16_t c = (i == city) ? BTN : CARD;
    drawCard(14, y, 292, 58, c);
    drawPin(40, y + 29, SKY);
    centerText(cities[i].name, 165, y + 29, 2, WHITE, c);
    if (i == city) tft.fillCircle(283, y+29, 5, GREEN);
  }
  drawCard(15, 418, 290, 48, BTN);
  centerText("BACK HOME", 160, 442, 2, WHITE, BTN);
}

void drawWeather() {
  drawTitle("LIVE WEATHER");
  textAt(cities[city].name, 17, 74, 4, WHITE, BG);
  drawCard(12, 119, 296, 256, CARD);
  char s[60];
  if (haveWeather) {
    snprintf(s, sizeof(s), "%.1f C", tempC);
    textAt(s, 29, 139, 4, YELLOW, CARD);
    textAt(conditionName(wmo), 29, 182, 2, SKY, CARD);
    snprintf(s, sizeof(s), "Feels like: %.1f C", apparentC);
    textAt(s, 29, 238, 2, WHITE, CARD);
    snprintf(s, sizeof(s), "Humidity: %.0f%%", humidity);
    textAt(s, 29, 279, 2, WHITE, CARD);
    snprintf(s, sizeof(s), "Wind: %.1f km/h", windKmh);
    textAt(s, 29, 320, 2, WHITE, CARD);
  } else {
    textAt("WEATHER UNAVAILABLE", 29, 167, 2, RED, CARD);
    String msg = weatherError;
    if (msg.length() > 36) msg = msg.substring(0, 36);
    textAt(msg.c_str(), 29, 208, 1, MUTED, CARD);
    textAt("Tap REFRESH to retry", 29, 256, 2, WHITE, CARD);
  }
  drawCard(15, 394, 142, 64, BTN);
  centerText("BACK", 86, 426, 2, WHITE, BTN);
  drawCard(165, 394, 140, 64, BTN);
  centerText("REFRESH", 235, 426, 2, WHITE, BTN);
}

void drawSettings() {
  drawTitle("SETTINGS");
  drawCard(12, 77, 296, 268, CARD);
  textAt("ESP32D + ILI9488", 26, 97, 2, WHITE, CARD);
  textAt("Portrait 320 x 480", 26, 137, 2, MUTED, CARD);
  textAt("Touch: XPT2046", 26, 175, 2, MUTED, CARD);
  textAt("Weather source: Open-Meteo", 26, 213, 1, SKY, CARD);
  drawCard(23, 270, 274, 58, BTN);
  centerText("RECALIBRATE TOUCH", 160, 299, 2, WHITE, BTN);
  drawCard(15, 397, 290, 62, BTN);
  centerText("BACK HOME", 160, 428, 2, WHITE, BTN);
}

void drawPage() {
  drawBackground();
  switch (page) {
    case HOME:      drawHome(); break;
    case LOCATIONS: drawLocations(); break;
    case WEATHER:   drawWeather(); break;
    case SETTINGS:  drawSettings(); break;
  }
}

void calibrateTouchscreen() {
  tft.fillScreen(BG);
  textAt("TOUCH CALIBRATION", 31, 30, 2, SKY, BG);
  textAt("Press each corner target", 20, 75, 2, WHITE, BG);
  textAt("Hold then release", 20, 108, 2, MUTED, BG);
  tft.calibrateTouch(calibration, SKY, BG, 15);
  tft.setTouch(calibration);
  for (int i = 0; i < 5; i++) {
    String key = String("c") + i;
    prefs.putUShort(key.c_str(), calibration[i]);
  }
  prefs.putBool("calOK", true);
  delay(200);
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (strcmp(WIFI_SSID, "YOUR_WIFI_NAME") == 0) {
    weatherError = "Enter WiFi credentials";
    return;
  }
  Serial.print("WiFi connecting to: "); Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000UL) {
    delay(100);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK, IP: "); Serial.println(WiFi.localIP());
    configTime(8 * 3600, 0, "pool.ntp.org", "time.google.com");
  } else {
    weatherError = "WiFi connection failed";
    Serial.println("WiFi connection failed");
  }
}

void reportWeatherStatus(const String& status) {
  weatherError = status;
  Serial.print("Weather status: "); Serial.println(status);
}

bool fetchWeather() {
  lastFetchAttempt = millis();
  if (WiFi.status() != WL_CONNECTED) {
    reportWeatherStatus("WiFi disconnected");
    return false;
  }
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") +
    String(cities[city].lat, 4) + "&longitude=" + String(cities[city].lon, 4) +
    "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m" +
    "&timezone=Asia%2FManila";

  Serial.print("Fetching weather: "); Serial.println(cities[city].name);
  Serial.println(url);
  WiFiClientSecure client;
  client.setInsecure(); // PROTOTYPE ONLY: does not verify server certificate.
  client.setTimeout(12000);
  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(12000);
  http.setTimeout(12000);
  if (!http.begin(client, url)) {
    reportWeatherStatus("HTTPS setup failed");
    return false;
  }
  int status = http.GET();
  Serial.printf("Weather HTTP: %d\n", status);
  if (status != HTTP_CODE_OK) {
    if (status < 0) reportWeatherStatus(http.errorToString(status));
    else {
      reportWeatherStatus("HTTP " + String(status));
      Serial.println(http.getString().substring(0, 180));
    }
    http.end();
    return false;
  }
  String payload = http.getString();
  http.end();
  Serial.printf("Weather payload size: %u\n", (unsigned)payload.length());
  if (payload.length() == 0) {
    reportWeatherStatus("Empty API response");
    return false;
  }
  JsonDocument doc; // ArduinoJson 7
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    reportWeatherStatus(String("JSON: ") + err.c_str());
    return false;
  }
  JsonVariant current = doc["current"];
  if (current.isNull() || current["temperature_2m"].isNull() ||
      current["relative_humidity_2m"].isNull() || current["weather_code"].isNull()) {
    reportWeatherStatus("Missing weather fields");
    return false;
  }
  tempC = current["temperature_2m"].as<float>();
  humidity = current["relative_humidity_2m"].as<float>();
  apparentC = current["apparent_temperature"].as<float>();
  windKmh = current["wind_speed_10m"].as<float>();
  wmo = current["weather_code"].as<int>();
  haveWeather = true;
  weatherError = "";
  lastWeatherOK = millis();
  Serial.printf("Weather OK: %s temp %.1fC RH %.0f%% code %d\n",
                cities[city].name, tempC, humidity, wmo);
  return true;
}

void changeCity(int index) {
  if (index < 0 || index >= CITY_COUNT) return;
  city = index;
  prefs.putInt("city", city);
  haveWeather = false;
  weatherError = "Fetching weather...";
  page = WEATHER;
  drawPage();
  connectWiFi();
  fetchWeather();
  drawPage();
}

void handleTouch(int x, int y) {
  Serial.printf("Tap x=%d y=%d page=%d\n", x, y, (int)page);
  if (page == HOME) {
    if (y >= 344 && y < 400) {
      page = (x < 160) ? LOCATIONS : WEATHER;
      drawPage();
      return;
    }
    if (y >= 407 && y < 463) {
      if (x < 160) {
        weatherError = "Fetching weather...";
        if (!haveWeather) updateHomeWeatherValues();
        connectWiFi();
        fetchWeather();
        drawPage();
      } else {
        page = SETTINGS;
        drawPage();
      }
      return;
    }
  } else if (page == LOCATIONS) {
    for (int i = 0; i < CITY_COUNT; i++) {
      int yy = 115 + i * 70;
      if (y >= yy && y < yy + 58) {
        changeCity(i);
        return;
      }
    }
    if (y >= 418) { page = HOME; drawPage(); }
  } else if (page == WEATHER) {
    if (y >= 394) {
      if (x < 160) { page = HOME; drawPage(); }
      else { connectWiFi(); fetchWeather(); drawPage(); }
    }
  } else if (page == SETTINGS) {
    if (y >= 270 && y < 328) {
      calibrateTouchscreen();
      drawPage();
      return;
    }
    if (y >= 397) { page = HOME; drawPage(); }
  }
}

void setup() {
  Serial.begin(115200);
  delay(400);
  tft.init();
  tft.setRotation(2); // upright portrait on your physical installation
  tft.fillScreen(BG);
  prefs.begin("smarthub", false); // Same as old V2; reuses touch calibration.
  city = prefs.getInt("city", 1);
  if (city < 0 || city >= CITY_COUNT) city = 1;
  if (!prefs.getBool("calOK", false)) {
    calibrateTouchscreen();
  } else {
    for (int i = 0; i < 5; i++) {
      String key = String("c") + i;
      calibration[i] = prefs.getUShort(key.c_str(), 0);
    }
    tft.setTouch(calibration);
  }

  clockSprite.setColorDepth(16);
  if (!clockSprite.createSprite(270, 41)) Serial.println("Clock sprite allocation failed");
  weatherSprite.setColorDepth(16);
  if (!weatherSprite.createSprite(105, 102)) Serial.println("Weather sprite allocation failed");

  drawPage();
  connectWiFi();
  weatherError = "Fetching weather...";
  drawPage();
  fetchWeather();
  drawPage();
}

void loop() {
  uint16_t x = 0, y = 0;
  bool touching = tft.getTouch(&x, &y, 600);
  static bool wasTouching = false;
  if (touching && !wasTouching && millis() - lastTouch > 250) {
    lastTouch = millis();
    handleTouch((int)x, (int)y);
  }
  wasTouching = touching;

  if (millis() - lastClockDraw >= 1000UL) {
    lastClockDraw = millis();
    drawClock();
    // Also update Wi-Fi status indicator if status changed.
  }
  if (millis() - lastAnim >= 220UL) {
    lastAnim = millis();
    spin = (spin + 8) % 360;
    drawWeatherSymbol();
  }
  unsigned long refreshAfter = haveWeather ? WEATHER_INTERVAL : RETRY_INTERVAL;
  if (millis() - lastFetchAttempt >= refreshAfter) {
    connectWiFi();
    if (fetchWeather()) drawPage();
    else if (page == HOME || page == WEATHER) drawPage();
  }
  // If WiFi disconnects, try reconnection periodically.
  if (WiFi.status() != WL_CONNECTED && millis() - lastReconnect >= 30000UL) {
    lastReconnect = millis();
    connectWiFi();
  }
  delay(10);
}
