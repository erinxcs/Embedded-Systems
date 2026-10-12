/*
  03 - REAL WEATHER (NO TOUCH), Open-Meteo + NTP
  ESP32 DevKit + 3.5-inch ILI9488 TFT, portrait 320 x 480.
  Requires TFT_eSPI + ArduinoJson 7. Touch is not read by this sketch.
  No DHT11 needed yet. Display temperature is from the internet,
  NOT actual room temperature. Latitude/longitude are Tanauan city center.
*/
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>

// Set credentials LOCALLY; do not commit Wi-Fi passwords to GitHub.
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const float LATITUDE = 14.0853f;
const float LONGITUDE = 121.1528f;
const char* CITY_NAME = "TANAUAN";

const unsigned long REFRESH_MS = 10UL * 60UL * 1000UL;
const unsigned long RETRY_MS = 60UL * 1000UL;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sunSprite = TFT_eSprite(&tft);

const uint16_t BG = 0x010C;
const uint16_t CARD = 0x09D6;
const uint16_t SKY = 0x35DF;
const uint16_t WHITE16 = 0xFFFF;
const uint16_t MUTED = 0x9D5A;
const uint16_t YELLOW16 = 0xFE60;

float temperature = 0.0f, humidity = 0.0f, feelsLike = 0.0f, windKmh = 0.0f;
int weatherCode = -1;
bool weatherReady = false;
String errorText = "Waiting for Wi-Fi";
unsigned long lastAttempt = 0, lastClock = 0, lastAnim = 0;
uint16_t angleDeg = 0;

void connectWiFi();
bool fetchWeather();
void drawScreen();
void updateTime();
void drawWeatherIcon();
const char* condition(int code);
void textAt(const char* s, int x, int y, int font, uint16_t fg, uint16_t bg);

void textAt(const char* s, int x, int y, int font, uint16_t fg, uint16_t bg) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(fg, bg);
  tft.drawString(s, x, y, font);
}

const char* condition(int code) {
  if (code == 0) return "Clear sky";
  if (code >= 1 && code <= 2) return "Partly cloudy";
  if (code == 3) return "Overcast";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Rain showers";
  if (code >= 95) return "Thunderstorm";
  return "Unknown condition";
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (strcmp(WIFI_SSID, "YOUR_WIFI_NAME") == 0) {
    errorText = "Set WiFi credentials";
    Serial.println("Set WIFI_SSID and WIFI_PASSWORD in the sketch.");
    return;
  }
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000UL) delay(100);
  if (WiFi.status() == WL_CONNECTED) {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.google.com");
    errorText = "Loading weather...";
    Serial.print("WiFi connected. IP: "); Serial.println(WiFi.localIP());
  } else {
    errorText = "WiFi not connected";
    Serial.println(errorText);
  }
}

bool fetchWeather() {
  lastAttempt = millis();
  if (WiFi.status() != WL_CONNECTED) {
    errorText = "WiFi disconnected";
    return false;
  }
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") +
               String(LATITUDE, 4) + "&longitude=" + String(LONGITUDE, 4) +
               "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m" +
               "&timezone=Asia%2FManila";
  WiFiClientSecure client;
  client.setInsecure(); // Prototype only; use CA certificate validation for final project.
  client.setTimeout(12000);
  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(12000);
  http.setTimeout(12000);
  if (!http.begin(client, url)) {
    errorText = "HTTPS setup failed";
    return false;
  }
  int status = http.GET();
  Serial.printf("Open-Meteo HTTP status: %d\n", status);
  if (status != HTTP_CODE_OK) {
    errorText = (status < 0) ? http.errorToString(status) : String("HTTP ") + status;
    http.end();
    return false;
  }
  String response = http.getString();
  http.end();
  if (response.isEmpty()) {
    errorText = "Empty HTTP response";
    return false;
  }
  JsonDocument doc; // ArduinoJson 7
  DeserializationError e = deserializeJson(doc, response);
  if (e) {
    errorText = String("JSON: ") + e.c_str();
    return false;
  }
  JsonVariant v = doc["current"];
  if (v.isNull() || v["temperature_2m"].isNull() || v["relative_humidity_2m"].isNull() || v["weather_code"].isNull()) {
    errorText = "Missing weather data";
    return false;
  }
  temperature = v["temperature_2m"].as<float>();
  humidity = v["relative_humidity_2m"].as<float>();
  feelsLike = v["apparent_temperature"].as<float>();
  windKmh = v["wind_speed_10m"].as<float>();
  weatherCode = v["weather_code"].as<int>();
  weatherReady = true;
  errorText = "";
  Serial.printf("%s: %.1f C, RH %.0f%%, code %d\n", CITY_NAME, temperature, humidity, weatherCode);
  return true;
}

void drawScreen() {
  tft.fillScreen(BG);
  textAt("ERINX WEATHER V1", 16, 18, 2, SKY, BG);
  tft.fillRoundRect(12, 67, 296, 91, 13, CARD);
  textAt("PHILIPPINE TIME", 25, 83, 2, SKY, CARD);
  updateTime();
  tft.fillRoundRect(12, 170, 296, 273, 14, CARD);
  textAt(CITY_NAME, 27, 188, 4, WHITE16, CARD);
  if (weatherReady) {
    char buffer[56];
    snprintf(buffer, sizeof(buffer), "%.1f C", temperature);
    textAt(buffer, 27, 260, 4, WHITE16, CARD);
    textAt(condition(weatherCode), 27, 302, 2, SKY, CARD);
    snprintf(buffer, sizeof(buffer), "Humidity: %.0f%%", humidity);
    textAt(buffer, 27, 342, 2, WHITE16, CARD);
    snprintf(buffer, sizeof(buffer), "Feels like: %.1f C", feelsLike);
    textAt(buffer, 27, 370, 2, MUTED, CARD);
    snprintf(buffer, sizeof(buffer), "Wind: %.1f km/h", windKmh);
    textAt(buffer, 27, 401, 2, MUTED, CARD);
  } else {
    textAt("Weather unavailable", 27, 282, 2, YELLOW16, CARD);
    String shortError = errorText.substring(0, 32);
    textAt(shortError.c_str(), 27, 317, 1, MUTED, CARD);
    textAt("Auto retry in 60 sec", 27, 373, 2, SKY, CARD);
  }
  textAt("NO TOUCH - WIFI + TFT ONLY", 28, 452, 2, MUTED, BG);
  drawWeatherIcon();
}

void updateTime() {
  tft.fillRect(20, 109, 282, 39, CARD);
  time_t now = time(nullptr);
  if (now < 1700000000) {
    textAt("Waiting for NTP...", 29, 112, 2, YELLOW16, CARD);
  } else {
    struct tm ts;
    localtime_r(&now, &ts);
    char buffer[28];
    strftime(buffer, sizeof(buffer), "%I:%M:%S %p", &ts);
    textAt(buffer, 28, 111, 4, WHITE16, CARD);
  }
}

void drawWeatherIcon() {
  if (!sunSprite.created()) return;
  sunSprite.fillSprite(CARD);
  const int cx = 45, cy = 39;
  bool sunny = weatherReady && (weatherCode >= 0 && weatherCode <= 2);
  if (sunny) {
    for (int i = 0; i < 8; i++) {
      float r = (angleDeg + i * 45) * 0.0174532925f;
      sunSprite.drawLine(cx + cosf(r)*20, cy + sinf(r)*20,
                         cx + cosf(r)*29, cy + sinf(r)*29, YELLOW16);
    }
    sunSprite.fillCircle(cx, cy, 16, YELLOW16);
  }
  if (!sunny || weatherCode == 1 || weatherCode == 2) {
    sunSprite.fillCircle(24, 60, 13, WHITE16);
    sunSprite.fillCircle(44, 54, 17, WHITE16);
    sunSprite.fillCircle(62, 62, 12, WHITE16);
    sunSprite.fillRoundRect(17, 60, 59, 15, 6, WHITE16);
  }
  sunSprite.pushSprite(207, 216);
  angleDeg = (angleDeg + 7) % 360;
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(2); // portrait; set to 0 if upside down
  sunSprite.setColorDepth(8);
  if (!sunSprite.createSprite(88, 85)) Serial.println("Icon sprite memory unavailable");
  drawScreen();
  connectWiFi();
  drawScreen();
  fetchWeather();
  drawScreen();
}

void loop() {
  if (millis() - lastClock >= 1000) {
    lastClock = millis();
    updateTime();
  }
  if (millis() - lastAnim >= 180) {
    lastAnim = millis();
    drawWeatherIcon();
  }
  unsigned long interval = weatherReady ? REFRESH_MS : RETRY_MS;
  if (millis() - lastAttempt >= interval) {
    connectWiFi();
    fetchWeather();
    drawScreen();
  }
  delay(10);
}
