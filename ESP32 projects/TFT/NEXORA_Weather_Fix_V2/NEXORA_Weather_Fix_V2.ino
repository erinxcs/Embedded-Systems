/*

  ESP32 SMART HUB V2 - ILI9488 + XPT2046, PORTRAIT 320x480

  Only ESP32 DevKit and 3.5 inch TFT touch module required.

  Touch GPIO: T_CLK=18 T_DIN=23 T_DO=19 T_CS=21. LCD SDO unconnected.

  Dependencies: TFT_eSPI (Bodmer), ArduinoJson (Benoit Blanchon, v7).

  TFT_eSPI/User_Setup.h: ILI9488_DRIVER, TFT_MISO=19, TFT_MOSI=23,

    TFT_SCLK=18, TFT_CS=5, TFT_DC=27, TFT_RST=33,

    TOUCH_CS=21, SPI_FREQUENCY=20000000, SPI_TOUCH_FREQUENCY=2500000.

  In TFT_eSPI/User_Setup.h keep LOAD_GLCD and LOAD_FONT2 enabled.

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



// Enter your Wi-Fi credentials locally; don't post the password.

const char* WIFI_SSID = "PLDTHOMEFIBRpz926";
const char* WIFI_PASSWORD = "PLDTWIFIf2b42";


TFT_eSPI tft;

TFT_eSprite iconSprite(&tft);  // Small sprite only (no font dependency)

Preferences prefs;



struct Place { const char* name; float lat; float lon; };

// Approximate town/city centers, not your exact home coordinates.

const Place cities[] = {

  {"Lipa",       13.9411f, 121.1631f},

  {"Tanauan",    14.0853f, 121.1528f},

  {"Malvar",     14.0444f, 121.1583f},

  {"Sto. Tomas", 14.1080f, 121.1420f}

};

const int CITY_COUNT = sizeof(cities) / sizeof(cities[0]);

int city = 0;

uint16_t calibration[5];



enum Page { HOME, LOCATIONS, WEATHER, SETTINGS };

Page page = HOME;

float tempC=0, apparentC=0, humidity=0, windKmh=0;

int wmo=-1;

bool haveWeather=false;

bool loading=false;
String weatherError = "Not requested";

unsigned long lastFetch=0, lastFetchAttempt=0, lastClockDraw=0, lastAnim=0, lastTouch=0;

int spin=0;

const unsigned long WEATHER_INTERVAL=10UL*60UL*1000UL;



const uint16_t BG=0x0862, CARD=0x10E4, NAVY=0x1167, CYAN=0x067B,

  WHITE=0xFFFF, MUTED=0x9CF3, GREEN=0x45D3, YELLOW=0xFE60,

  RED=0xF985, BORDER=0x2969;



void drawPage();

void drawHome();

void drawLocations();

void drawWeather();

void drawSettings();

void button(int x,int y,int w,int h,const char* caption,uint16_t color=CARD);

void textAt(const char* str,int x,int y,int font=2,uint16_t color=WHITE,uint16_t bg=BG);

void connectWiFi();

bool fetchWeather();

void updateClock();

void animateIcon();

void handleTouch(int x,int y);

void changeCity(int index);

void calibrate();

const char* conditionName(int code);



void textAt(const char* str,int x,int y,int font,uint16_t color,uint16_t bg) {

  tft.setTextDatum(TL_DATUM);

  tft.setTextFont(font);

  tft.setTextSize(1);

  tft.setTextColor(color,bg);

  tft.drawString(str,x,y,font);

}



void button(int x,int y,int w,int h,const char* caption,uint16_t color) {

  tft.fillRoundRect(x,y,w,h,12,color);

  tft.drawRoundRect(x,y,w,h,12,BORDER);

  tft.setTextDatum(MC_DATUM);

  tft.setTextFont(2);

  tft.setTextSize(1);

  tft.setTextColor(WHITE,color);

  tft.drawString(caption,x+w/2,y+h/2,2);

  tft.setTextDatum(TL_DATUM);

}



void calibrate() {

  tft.fillScreen(BG);

  textAt("TOUCH CALIBRATION",36,35,2,CYAN);

  textAt("Press each corner target",22,76,2,WHITE);

  textAt("Hold, then release.",22,106,2,MUTED);

  tft.calibrateTouch(calibration,CYAN,BG,15);

  tft.setTouch(calibration);

  for(int i=0;i<5;i++) prefs.putUShort((String("c")+i).c_str(),calibration[i]);

  prefs.putBool("calOK",true);

  delay(200);

}



void connectWiFi() {

  if (strcmp(WIFI_SSID,"YOUR_WIFI_NAME")==0) {
    weatherError = "Enter WiFi credentials";
    return;
  }

  if(WiFi.status()==WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);

  unsigned long started=millis();

  while(WiFi.status()!=WL_CONNECTED && millis()-started<10000UL) delay(100);

  if(WiFi.status()==WL_CONNECTED) {

    configTime(8*3600,0,"pool.ntp.org","time.google.com");

  }

}



const char* conditionName(int c) {

  if(c==0) return "Clear sky";

  if(c<=2) return "Partly cloudy";

  if(c==3) return "Overcast";

  if(c==45||c==48) return "Fog";

  if(c>=51&&c<=57) return "Drizzle";

  if(c>=61&&c<=67) return "Rain";

  if(c>=71&&c<=77) return "Snow";

  if(c>=80&&c<=82) return "Rain showers";

  if(c>=95) return "Thunderstorm";

  return "Weather update";

}



bool fetchWeather() {
  lastFetchAttempt = millis();
  if (WiFi.status() != WL_CONNECTED) {
    weatherError = "WiFi disconnected";
    Serial.println("Weather error: WiFi disconnected");
    return false;
  }

  // Explicitly request JSON in a simple HTTP/1.0 response (avoid chunked stream parsing).
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") +
    String(cities[city].lat, 4) + "&longitude=" + String(cities[city].lon, 4) +
    "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m" +
    "&timezone=Asia%2FManila";

  Serial.printf("Weather request for %s\n", cities[city].name);
  Serial.println(url);

  WiFiClientSecure client;
  client.setInsecure();  // Development only: no server-certificate validation.
  client.setTimeout(12000);
  HTTPClient http;
  http.useHTTP10(true); // Request non-chunked response if supported by server.
  http.setConnectTimeout(12000);
  http.setTimeout(12000);

  if (!http.begin(client, url)) {
    weatherError = "HTTPS setup failed";
    Serial.println("Weather error: HTTPS begin failed");
    return false;
  }

  int status = http.GET();
  Serial.printf("Weather HTTP status: %d\n", status);
  if (status != HTTP_CODE_OK) {
    if (status < 0) {
      weatherError = http.errorToString(status);
    } else {
      weatherError = "HTTP " + String(status);
      Serial.println(http.getString().substring(0, 250));
    }
    Serial.print("Weather error: "); Serial.println(weatherError);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  Serial.printf("Weather response bytes: %u\n", (unsigned)payload.length());
  Serial.println(payload.substring(0, 250));
  if (!payload.length()) {
    weatherError = "Empty API response";
    return false;
  }

  JsonDocument doc; // Requires ArduinoJson version 7.
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    weatherError = String("JSON: ") + err.c_str();
    Serial.print("Weather error: "); Serial.println(weatherError);
    return false;
  }
  JsonVariant v = doc["current"];
  if (v.isNull() || v["temperature_2m"].isNull() ||
      v["relative_humidity_2m"].isNull() || v["weather_code"].isNull()) {
    weatherError = "Missing weather fields";
    Serial.println("Weather error: current fields missing");
    return false;
  }

  tempC = v["temperature_2m"].as<float>();
  humidity = v["relative_humidity_2m"].as<float>();
  apparentC = v["apparent_temperature"].as<float>();
  windKmh = v["wind_speed_10m"].as<float>();
  wmo = v["weather_code"].as<int>();
  haveWeather = true;
  weatherError = "";
  lastFetch = millis();
  Serial.printf("Weather OK: %s %.1f C RH %.0f%% code %d\n",
                cities[city].name, tempC, humidity, wmo);
  return true;
}

void header(const char* name) {

  tft.fillRect(0,0,320,62,NAVY);

  textAt(name,16,18,2,WHITE,NAVY);

  textAt(WiFi.status()==WL_CONNECTED ? "WiFi OK":"Offline",238,19,1,

         WiFi.status()==WL_CONNECTED?GREEN:RED,NAVY);

}



void drawHome() {

  header("SMART HUB V2");

  tft.fillRoundRect(12,76,296,130,16,CARD);

  textAt("SELECTED LOCATION",27,89,1,CYAN,CARD);

  textAt(cities[city].name,27,109,4,WHITE,CARD);

  char line[50];

  if(haveWeather) snprintf(line,sizeof(line),"%.1f C   RH %.0f%%",tempC,humidity);

  else snprintf(line,sizeof(line),"Weather not loaded");

  textAt(line,27,155,2,YELLOW,CARD);

  textAt(haveWeather?conditionName(wmo):weatherError.c_str(),27,178,1,MUTED,CARD);

  button(12,218,144,66,"LOCATIONS",NAVY);

  button(164,218,144,66,"WEATHER",NAVY);

  button(12,296,144,66,"REFRESH",CARD);

  button(164,296,144,66,"SETTINGS",CARD);

  tft.fillRect(0,393,320,87,BG);

  textAt("PHILIPPINE TIME",14,397,1,MUTED);

  updateClock();

}



void drawLocations() {

  header("CHOOSE LOCATION");

  textAt("Tap a city for live weather",18,76,2,MUTED);

  for(int i=0;i<CITY_COUNT;i++) {

    int y=119+i*68;

    button(14,y,292,56,cities[i].name,city==i?NAVY:CARD);

    if(city==i) tft.fillCircle(35,y+28,5,GREEN);

  }

  button(15,414,290,49,"BACK HOME",CARD);

}



void drawWeather() {

  header("LIVE WEATHER");

  textAt(cities[city].name,17,77,4,WHITE);

  tft.fillRoundRect(14,122,292,252,16,CARD);

  char b[48];

  if(haveWeather) {

    snprintf(b,sizeof(b),"%.1f C",tempC);textAt(b,29,138,4,YELLOW,CARD);

    textAt(conditionName(wmo),29,183,2,CYAN,CARD);

    snprintf(b,sizeof(b),"Feels like: %.1f C",apparentC);textAt(b,29,240,2,WHITE,CARD);

    snprintf(b,sizeof(b),"Humidity: %.0f %%",humidity);textAt(b,29,277,2,WHITE,CARD);

    snprintf(b,sizeof(b),"Wind: %.1f km/h",windKmh);textAt(b,29,314,2,WHITE,CARD);

  } else {

    textAt("No weather data",29,178,2,RED,CARD);

    textAt(weatherError.c_str(),29,214,1,MUTED,CARD);

  }

  button(15,394,142,66,"BACK",NAVY);

  button(165,394,140,66,"REFRESH",NAVY);

}



void drawSettings() {

  header("SETTINGS");

  textAt("ESP32D + ILI9488",18,90,2,WHITE);

  textAt("Portrait: 320 x 480",18,122,2,MUTED);

  textAt("Location:",18,167,2,MUTED);

  textAt(cities[city].name,122,167,2,CYAN);

  textAt("Touch calibration saved",18,211,2,GREEN);

  button(15,278,290,65,"RECALIBRATE TOUCH",NAVY);

  button(15,386,290,65,"BACK HOME",CARD);

}



void drawPage() {

  tft.fillScreen(BG);

  switch(page) {

    case HOME:drawHome();break;

    case LOCATIONS:drawLocations();break;

    case WEATHER:drawWeather();break;

    case SETTINGS:drawSettings();break;

  }

}



void updateClock() {

  if(page!=HOME) return;

  time_t now=time(nullptr);

  struct tm* info=localtime(&now);

  tft.fillRect(14,416,295,55,BG);

  if(now<1700000000 || !info) {

    textAt("Time: waiting for WiFi",15,426,2,YELLOW);

  } else {

    char s[26];strftime(s,sizeof(s),"%I:%M:%S %p",info);

    textAt(s,15,425,4,WHITE);

  }

}



void animateIcon() {

  if(page!=HOME || !haveWeather || !iconSprite.created()) return;

  iconSprite.fillSprite(CARD);

  const int cx=27,cy=25;

  for(int k=0;k<8;k++) {

    float angle=(spin+k*45)*0.0174532925f;

    iconSprite.drawLine(cx+cosf(angle)*15,cy+sinf(angle)*15,

      cx+cosf(angle)*23,cy+sinf(angle)*23,YELLOW);

  }

  iconSprite.fillCircle(cx,cy,11,YELLOW);

  iconSprite.pushSprite(242,137);

  spin=(spin+12)%360;

}



void changeCity(int index) {

  if(index<0||index>=CITY_COUNT)return;

  city=index;

  prefs.putInt("city",city);

  haveWeather=false;

  page=WEATHER;

  drawPage();

  connectWiFi();

  bool ok=fetchWeather();

  Serial.println(ok?"Weather updated":"Weather unavailable");

  drawPage();

}



void handleTouch(int x,int y) {

  Serial.printf("Tap x=%d y=%d page=%d\n",x,y,(int)page);

  if(page==HOME) {

    if(y>=218&&y<284) {page=(x<160)?LOCATIONS:WEATHER;drawPage();return;}

    if(y>=296&&y<362) {

      if(x<160){ connectWiFi();fetchWeather();drawPage(); }

      else {page=SETTINGS;drawPage();}

      return;

    }

  } else if(page==LOCATIONS) {

    if(y>=119&&y<119+4*68) {

      int i=(y-119)/68;

      if(y<119+i*68+56){changeCity(i);return;}

    }

    if(y>=414) {page=HOME;drawPage();}

  } else if(page==WEATHER) {

    if(y>=394) {

      if(x<160){page=HOME;drawPage();}

      else {connectWiFi();fetchWeather();drawPage();}

    }

  } else if(page==SETTINGS) {

    if(y>=278&&y<343) {calibrate();drawPage();return;}

    if(y>=386) {page=HOME;drawPage();}

  }

}



void setup() {

  Serial.begin(115200);

  delay(300);

  tft.init();

  tft.setRotation(2); // Portrait upright as requested (switch to 0 if physically reversed)

  tft.fillScreen(BG);

  prefs.begin("smarthub",false);

  city=prefs.getInt("city",0);

  if(city<0||city>=CITY_COUNT) city=0;

  if(!prefs.getBool("calOK",false)) calibrate();

  else {

    for(int i=0;i<5;i++) calibration[i]=prefs.getUShort((String("c")+i).c_str(),0);

    tft.setTouch(calibration);

  }

  iconSprite.setColorDepth(8);

  iconSprite.createSprite(55,52); // ~3 KB RAM

  drawPage();

  connectWiFi();

  bool ok=fetchWeather();

  Serial.println(ok?"First weather update OK":"Offline/no API data");

  drawPage();

}



void loop() {

  uint16_t x=0,y=0;

  bool touching=tft.getTouch(&x,&y,600);

  static bool wasTouching=false;

  if(touching && !wasTouching && millis()-lastTouch>250) {

    lastTouch=millis();

    handleTouch((int)x,(int)y);

  }

  wasTouching=touching;

  if(millis()-lastClockDraw>1000) {lastClockDraw=millis();updateClock();}

  if(millis()-lastAnim>200) {lastAnim=millis();animateIcon();}

  // Retry API at most once a minute if a previous request failed.

  unsigned long interval=haveWeather?WEATHER_INTERVAL:60000UL;

  if(millis()-lastFetchAttempt>interval) {

    connectWiFi();

    if(fetchWeather()) drawPage();

  }

  delay(12);

}
