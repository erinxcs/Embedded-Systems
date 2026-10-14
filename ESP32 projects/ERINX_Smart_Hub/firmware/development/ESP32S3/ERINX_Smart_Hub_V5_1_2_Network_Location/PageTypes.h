#ifndef ERINX_PAGE_TYPES_H
#define ERINX_PAGE_TYPES_H

#include <Arduino.h>

// Keep this type in a header: the Arduino .ino preprocessor adds
// function prototypes before declarations inside the .ino file.
enum Page : uint8_t {
  PAGE_HOME,
  PAGE_WEATHER,
  PAGE_CLOCK,
  PAGE_CLIMATE,
  PAGE_GALLERY,
  PAGE_PHOTO,
  PAGE_VIDEO,
  PAGE_VIDEO_PLAYER,
  PAGE_SETTINGS,
  PAGE_WIFI_LIST,
  PAGE_WIFI_PASSWORD
};

#endif // ERINX_PAGE_TYPES_H
