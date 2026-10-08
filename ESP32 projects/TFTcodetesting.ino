#include <SPI.h>
#include <TFT_eSPI.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

#include <time.h>
#include <math.h>

// =====================================================
// WIFI
// =====================================================

const char* WIFI_SSID =
  "PLDTHOMEFIBRpz926";

const char* WIFI_PASSWORD =
  "PLDTWIFIf2b42";


// =====================================================
// LOCATION
// coordinates from your weather link
// =====================================================

const float LATITUDE  = 14.0853;
const float LONGITUDE = 121.1528;


// =====================================================
// TFT
// =====================================================

TFT_eSPI tft = TFT_eSPI();


// =====================================================
// SPRITES
// =====================================================

// Clock sprite
TFT_eSprite clockSprite =
  TFT_eSprite(&tft);

// Weather icon sprite
TFT_eSprite weatherSprite =
  TFT_eSprite(&tft);


// =====================================================
// WEATHER DATA
// =====================================================

float temperature = 0;
float humidity = 0;
float feelsLike = 0;
float windSpeed = 0;

int weatherCode = 0;

bool weatherAvailable = false;


// =====================================================
// TIMERS
// =====================================================

unsigned long lastWeatherUpdate = 0;
unsigned long lastClockUpdate = 0;
unsigned long lastAnimationUpdate = 0;

const unsigned long WEATHER_REFRESH =
  10UL * 60UL * 1000UL;


// =====================================================
// ANIMATION
// =====================================================

int animationFrame = 0;


// =====================================================
// FUNCTION DECLARATIONS
// =====================================================

void connectWiFi();

bool fetchWeather();

void drawInterface();
void drawWeatherData();

void updateClock();
void drawClock();

void updateWeatherAnimation();
void drawWeatherIcon();

const char* getWeatherDescription(
  int code
);

void drawSunnyIcon();
void drawCloudyIcon();
void drawRainIcon();
void drawFogIcon();
void drawThunderIcon();


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(500);

  // ---------------------------------------------------
  // TFT
  // ---------------------------------------------------

  tft.init();

  tft.setRotation(1);

  tft.fillScreen(
    TFT_BLACK
  );


  // ---------------------------------------------------
  // SPRITES
  // ---------------------------------------------------

  // Clock sprite
  clockSprite.setColorDepth(8);

  clockSprite.createSprite(
    220,
    50
  );


  // Weather animation sprite
  weatherSprite.setColorDepth(8);

  weatherSprite.createSprite(
    145,
    125
  );


  // ---------------------------------------------------
  // START SCREEN
  // ---------------------------------------------------

  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  tft.setTextSize(3);

  tft.drawString(
    "ESP32 WEATHER",
    240,
    100
  );


  tft.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  tft.setTextSize(2);

  tft.drawString(
    "Connecting to WiFi...",
    240,
    170
  );


  // ---------------------------------------------------
  // WIFI
  // ---------------------------------------------------

  connectWiFi();


  // ---------------------------------------------------
  // NTP CLOCK
  // Philippines UTC+8
  // ---------------------------------------------------

  configTime(
    8 * 3600,
    0,
    "pool.ntp.org",
    "time.nist.gov"
  );


  // ---------------------------------------------------
  // UI
  // ---------------------------------------------------

  drawInterface();


  // ---------------------------------------------------
  // WEATHER API
  // ---------------------------------------------------

  fetchWeather();

  drawWeatherData();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // Clock
  if (
    millis() - lastClockUpdate
    >= 1000
  ) {

    lastClockUpdate =
      millis();

    updateClock();
  }


  // Weather animation
  if (
    millis() - lastAnimationUpdate
    >= 80
  ) {

    lastAnimationUpdate =
      millis();

    updateWeatherAnimation();
  }


  // Weather update
  if (
    millis() - lastWeatherUpdate
    >= WEATHER_REFRESH
  ) {

    if (
      WiFi.status()
      == WL_CONNECTED
    ) {

      if (
        fetchWeather()
      ) {

        drawWeatherData();
      }
    }
  }


  // Reconnect WiFi
  if (
    WiFi.status()
    != WL_CONNECTED
  ) {

    static unsigned long
      lastReconnect = 0;


    if (
      millis() - lastReconnect
      >= 15000
    ) {

      lastReconnect =
        millis();

      connectWiFi();
    }
  }
}


// =====================================================
// WIFI CONNECTION
// =====================================================

void connectWiFi() {

  Serial.println();
  Serial.print(
    "Connecting to "
  );

  Serial.println(
    WIFI_SSID
  );


  WiFi.mode(
    WIFI_STA
  );

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  unsigned long startTime =
    millis();


  while (
    WiFi.status()
      != WL_CONNECTED
    &&
    millis() - startTime
      < 20000
  ) {

    delay(500);

    Serial.print(".");
  }


  Serial.println();


  if (
    WiFi.status()
      == WL_CONNECTED
  ) {

    Serial.println(
      "WiFi connected!"
    );

    Serial.print(
      "IP Address: "
    );

    Serial.println(
      WiFi.localIP()
    );
  }

  else {

    Serial.println(
      "WiFi connection failed."
    );
  }
}


// =====================================================
// OPEN-METEO API
// =====================================================

bool fetchWeather() {

  if (
    WiFi.status()
      != WL_CONNECTED
  ) {

    return false;
  }


  Serial.println();
  Serial.println(
    "Fetching weather..."
  );


  String url =
    "https://api.open-meteo.com/v1/forecast"
    "?latitude="
    + String(LATITUDE, 4)
    + "&longitude="
    + String(LONGITUDE, 4)
    + "&current="
    "temperature_2m,"
    "relative_humidity_2m,"
    "apparent_temperature,"
    "weather_code,"
    "wind_speed_10m"
    "&timezone=Asia%2FManila";


  Serial.println(url);


  WiFiClientSecure client;

  // Prototype/testing:
  // skip certificate validation
  client.setInsecure();


  HTTPClient http;


  if (
    !http.begin(
      client,
      url
    )
  ) {

    Serial.println(
      "HTTP begin failed!"
    );

    return false;
  }


  int httpCode =
    http.GET();


  if (
    httpCode != 200
  ) {

    Serial.print(
      "HTTP error: "
    );

    Serial.println(
      httpCode
    );

    http.end();

    return false;
  }


  String payload =
    http.getString();


  http.end();


  Serial.println(
    "Weather received!"
  );


  // ---------------------------------------------------
  // JSON
  // ---------------------------------------------------

  DynamicJsonDocument
    document(4096);


  DeserializationError error =
    deserializeJson(
      document,
      payload
    );


  if (
    error
  ) {

    Serial.print(
      "JSON error: "
    );

    Serial.println(
      error.c_str()
    );

    return false;
  }


  JsonObject current =
    document["current"];


  temperature =
    current["temperature_2m"]
      | 0.0;


  humidity =
    current["relative_humidity_2m"]
      | 0.0;


  feelsLike =
    current["apparent_temperature"]
      | 0.0;


  weatherCode =
    current["weather_code"]
      | 0;


  windSpeed =
    current["wind_speed_10m"]
      | 0.0;


  weatherAvailable =
    true;


  lastWeatherUpdate =
    millis();


  // Debug
  Serial.print(
    "Temperature: "
  );

  Serial.println(
    temperature
  );


  Serial.print(
    "Humidity: "
  );

  Serial.println(
    humidity
  );


  Serial.print(
    "Feels like: "
  );

  Serial.println(
    feelsLike
  );


  Serial.print(
    "Weather code: "
  );

  Serial.println(
    weatherCode
  );


  Serial.print(
    "Wind: "
  );

  Serial.println(
    windSpeed
  );


  return true;
}


// =====================================================
// DRAW MAIN UI
// =====================================================

void drawInterface() {

  tft.fillScreen(
    TFT_BLACK
  );


  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.fillRoundRect(
    10,
    8,
    460,
    55,
    10,
    TFT_DARKCYAN
  );


  // WiFi
  tft.setTextDatum(
    TL_DATUM
  );

  tft.setTextSize(1);

  tft.setTextColor(
    TFT_WHITE,
    TFT_DARKCYAN
  );

  tft.setCursor(
    375,
    20
  );

  tft.print(
    "WiFi"
  );


  // WiFi dot

  if (
    WiFi.status()
      == WL_CONNECTED
  ) {

    tft.fillCircle(
      450,
      27,
      5,
      TFT_GREEN
    );
  }

  else {

    tft.fillCircle(
      450,
      27,
      5,
      TFT_RED
    );
  }


  // ---------------------------------------------------
  // WEATHER CARD
  // ---------------------------------------------------

  tft.fillRoundRect(
    10,
    73,
    285,
    235,
    15,
    TFT_NAVY
  );


  tft.setTextColor(
    TFT_CYAN,
    TFT_NAVY
  );

  tft.setTextSize(2);

  tft.setCursor(
    25,
    90
  );

  tft.print(
    "CURRENT WEATHER"
  );


  // ---------------------------------------------------
  // ANIMATION CARD
  // ---------------------------------------------------

  tft.fillRoundRect(
    305,
    73,
    165,
    235,
    15,
    TFT_DARKGREY
  );


  tft.setTextColor(
    TFT_WHITE,
    TFT_DARKGREY
  );

  tft.setTextSize(1);

  tft.setCursor(
    320,
    90
  );

  tft.print(
    "LIVE CONDITION"
  );


  updateClock();
}


// =====================================================
// WEATHER DATA
// =====================================================

void drawWeatherData() {

  // Clear data area
  tft.fillRect(
    20,
    120,
    265,
    175,
    TFT_NAVY
  );


  if (
    !weatherAvailable
  ) {

    tft.setTextColor(
      TFT_RED,
      TFT_NAVY
    );

    tft.setTextSize(2);

    tft.setCursor(
      35,
      170
    );

    tft.print(
      "NO WEATHER DATA"
    );

    return;
  }


  // ---------------------------------------------------
  // TEMPERATURE
  // ---------------------------------------------------

  tft.setTextColor(
    TFT_WHITE,
    TFT_NAVY
  );

  tft.setTextSize(5);

  tft.setCursor(
    25,
    125
  );

  tft.print(
    temperature,
    1
  );


  tft.setTextSize(2);

  tft.print(
    " C"
  );


  // ---------------------------------------------------
  // WEATHER DESCRIPTION
  // ---------------------------------------------------

  tft.setTextColor(
    TFT_YELLOW,
    TFT_NAVY
  );

  tft.setTextSize(2);

  tft.setCursor(
    25,
    180
  );

  tft.print(
    getWeatherDescription(
      weatherCode
    )
  );


  // ---------------------------------------------------
  // DETAILS
  // ---------------------------------------------------

  tft.setTextColor(
    TFT_WHITE,
    TFT_NAVY
  );

  tft.setTextSize(1);


  tft.setCursor(
    25,
    220
  );

  tft.print(
    "Feels like: "
  );

  tft.print(
    feelsLike,
    1
  );

  tft.print(
    " C"
  );


  tft.setCursor(
    25,
    245
  );

  tft.print(
    "Humidity:   "
  );

  tft.print(
    humidity,
    0
  );

  tft.print(
    " %"
  );


  tft.setCursor(
    25,
    270
  );

  tft.print(
    "Wind:       "
  );

  tft.print(
    windSpeed,
    1
  );

  tft.print(
    " km/h"
  );
}


// =====================================================
// CLOCK
// =====================================================

void updateClock() {

  drawClock();
}


void drawClock() {

  clockSprite.fillSprite(
    TFT_DARKCYAN
  );


  struct tm timeInfo;


  clockSprite.setTextDatum(
    ML_DATUM
  );


  if (
    getLocalTime(
      &timeInfo,
      100
    )
  ) {

    char timeText[20];


    strftime(
      timeText,
      sizeof(timeText),
      "%I:%M:%S %p",
      &timeInfo
    );


    clockSprite.setTextColor(
      TFT_WHITE,
      TFT_DARKCYAN
    );

    clockSprite.setTextSize(2);


    clockSprite.drawString(
      timeText,
      5,
      18
    );
  }

  else {

    clockSprite.setTextColor(
      TFT_WHITE,
      TFT_DARKCYAN
    );

    clockSprite.setTextSize(2);


    clockSprite.drawString(
      "--:--:--",
      5,
      18
    );
  }


  clockSprite.pushSprite(
    20,
    11
  );
}


// =====================================================
// ANIMATION
// =====================================================

void updateWeatherAnimation() {

  animationFrame++;

  if (
    animationFrame > 360
  ) {

    animationFrame = 0;
  }


  drawWeatherIcon();
}


// =====================================================
// DRAW WEATHER ICON SPRITE
// =====================================================

void drawWeatherIcon() {

  weatherSprite.fillSprite(
    TFT_DARKGREY
  );


  if (
    weatherCode == 0
  ) {

    drawSunnyIcon();
  }

  else if (
    weatherCode >= 1
    &&
    weatherCode <= 3
  ) {

    drawCloudyIcon();
  }

  else if (
    weatherCode == 45
    ||
    weatherCode == 48
  ) {

    drawFogIcon();
  }

  else if (
    (
      weatherCode >= 51
      &&
      weatherCode <= 67
    )
    ||
    (
      weatherCode >= 80
      &&
      weatherCode <= 82
    )
  ) {

    drawRainIcon();
  }

  else if (
    weatherCode >= 95
  ) {

    drawThunderIcon();
  }

  else {

    drawCloudyIcon();
  }


  weatherSprite.pushSprite(
    315,
    115
  );
}


// =====================================================
// SUNNY ICON
// =====================================================

void drawSunnyIcon() {

  int cx = 72;
  int cy = 58;


  // Rotating rays
  for (
    int i = 0;
    i < 8;
    i++
  ) {

    float angle =
      radians(
        animationFrame
        + i * 45
      );


    int x1 =
      cx
      + cos(angle) * 28;

    int y1 =
      cy
      + sin(angle) * 28;


    int x2 =
      cx
      + cos(angle) * 39;

    int y2 =
      cy
      + sin(angle) * 39;


    weatherSprite.drawLine(
      x1,
      y1,
      x2,
      y2,
      TFT_YELLOW
    );
  }


  weatherSprite.fillCircle(
    cx,
    cy,
    20,
    TFT_YELLOW
  );


  weatherSprite.fillCircle(
    cx - 6,
    cy - 6,
    5,
    TFT_WHITE
  );
}


// =====================================================
// CLOUDY ICON
// =====================================================

void drawCloudyIcon() {

  // Sun behind cloud

  int cx = 90;
  int cy = 45;


  weatherSprite.fillCircle(
    cx,
    cy,
    17,
    TFT_YELLOW
  );


  int offset =
    (animationFrame / 4)
    % 10;


  int cloudX =
    47 + offset;


  weatherSprite.fillCircle(
    cloudX,
    65,
    20,
    TFT_LIGHTGREY
  );


  weatherSprite.fillCircle(
    cloudX + 23,
    55,
    26,
    TFT_LIGHTGREY
  );


  weatherSprite.fillCircle(
    cloudX + 48,
    66,
    19,
    TFT_LIGHTGREY
  );


  weatherSprite.fillRoundRect(
    cloudX - 5,
    64,
    75,
    27,
    12,
    TFT_LIGHTGREY
  );
}


// =====================================================
// RAIN ICON
// =====================================================

void drawRainIcon() {

  int cloudX = 37;


  weatherSprite.fillCircle(
    cloudX + 18,
    44,
    20,
    TFT_LIGHTGREY
  );


  weatherSprite.fillCircle(
    cloudX + 43,
    37,
    25,
    TFT_LIGHTGREY
  );


  weatherSprite.fillCircle(
    cloudX + 69,
    47,
    18,
    TFT_LIGHTGREY
  );


  weatherSprite.fillRoundRect(
    cloudX,
    45,
    88,
    27,
    12,
    TFT_LIGHTGREY
  );


  // Moving rain drops

  for (
    int i = 0;
    i < 4;
    i++
  ) {

    int x =
      48 + i * 20;


    int y =
      80
      +
      (
        animationFrame * 3
        + i * 15
      )
      % 35;


    weatherSprite.drawLine(
      x,
      y,
      x - 4,
      y + 9,
      TFT_CYAN
    );
  }
}


// =====================================================
// FOG ICON
// =====================================================

void drawFogIcon() {

  int offset =
    animationFrame % 20;


  for (
    int i = 0;
    i < 4;
    i++
  ) {

    int y =
      35 + i * 20;


    weatherSprite.drawLine(
      20 + offset,
      y,
      115,
      y,
      TFT_LIGHTGREY
    );
  }
}


// =====================================================
// THUNDER ICON
// =====================================================

void drawThunderIcon() {

  // Cloud

  weatherSprite.fillCircle(
    45,
    45,
    20,
    TFT_LIGHTGREY
  );


  weatherSprite.fillCircle(
    70,
    37,
    25,
    TFT_LIGHTGREY
  );


  weatherSprite.fillCircle(
    95,
    47,
    18,
    TFT_LIGHTGREY
  );


  weatherSprite.fillRoundRect(
    30,
    45,
    85,
    27,
    12,
    TFT_LIGHTGREY
  );


  // Flashing lightning

  if (
    (
      animationFrame / 5
    )
    % 2
  ) {

    weatherSprite.fillTriangle(
      70,
      73,

      56,
      98,

      70,
      94,

      TFT_YELLOW
    );


    weatherSprite.fillTriangle(
      70,
      90,

      62,
      115,

      85,
      85,

      TFT_YELLOW
    );
  }
}


// =====================================================
// WEATHER DESCRIPTION
// WMO WEATHER CODES
// =====================================================

const char* getWeatherDescription(
  int code
) {

  if (
    code == 0
  ) {

    return "Clear sky";
  }


  if (
    code == 1
  ) {

    return "Mainly clear";
  }


  if (
    code == 2
  ) {

    return "Partly cloudy";
  }


  if (
    code == 3
  ) {

    return "Overcast";
  }


  if (
    code == 45
    ||
    code == 48
  ) {

    return "Fog";
  }


  if (
    code >= 51
    &&
    code <= 57
  ) {

    return "Drizzle";
  }


  if (
    code >= 61
    &&
    code <= 67
  ) {

    return "Rain";
  }


  if (
    code >= 71
    &&
    code <= 77
  ) {

    return "Snow";
  }


  if (
    code >= 80
    &&
    code <= 82
  ) {

    return "Rain showers";
  }


  if (
    code >= 95
  ) {

    return "Thunderstorm";
  }


  return "Unknown";
}