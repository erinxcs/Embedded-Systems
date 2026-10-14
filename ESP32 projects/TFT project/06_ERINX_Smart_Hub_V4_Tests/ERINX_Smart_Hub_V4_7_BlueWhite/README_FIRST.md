ERINX SMART HUB V4.7 - BLUE/WHITE REDESIGN

What's new:
- "ERINX SMART HUB" branding replaces "ERINX OS"
- Blue/white icon pack for Weather, Clock, DHT11, Gallery, Video, Settings
- Animated eye on the home screen (top-right)
- Wi-Fi + SD icons in headers/status areas
- Clock page redesigned based on your sketch
- Weather page redesigned with moving cloud animation area
- Portrait 320x480 layout preserved
- Existing touch calibration, SD browser, Wi-Fi/NTP, DHT11, and page navigation preserved

HOW TO USE
1. Copy the whole `icons` folder from 01_COPY_TO_SD_CARD to the ROOT of your microSD card.
   Final structure should be:
      /icons/weather.rgb565
      /icons/clock.rgb565
      /icons/dht11.rgb565
      /icons/gallery.rgb565
      /icons/video.rgb565
      /icons/settings.rgb565
2. Open:
      03_ARDUINO_CODE/ERINX_Smart_Hub_V4_7_BlueWhite/ERINX_Smart_Hub_V4_7_BlueWhite.ino
3. Keep your working TFT_eSPI User_Setup.h and wiring.
4. Verify and upload using ESP32S3 Dev Module.

REQUIRED LIBRARIES
- TFT_eSPI by Bodmer
- ArduinoJson
- DHT sensor library by Adafruit
- Adafruit Unified Sensor
(plus built-in ESP32 WiFi, Preferences, FS, SD_MMC)

NOTES
- The icons are RAW RGB565 40x40 files loaded directly from the ESP32-S3 built-in SD.
- If an icon file is missing, the code falls back to a simple letter tile.
- The weather cloud animation is lightweight and only updates on the Weather page.
- The moving eye animates on the Home screen.
