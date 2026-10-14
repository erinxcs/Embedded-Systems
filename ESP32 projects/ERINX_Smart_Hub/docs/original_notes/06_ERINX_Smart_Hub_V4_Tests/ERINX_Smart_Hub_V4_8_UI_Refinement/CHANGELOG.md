# V4.8 versus V4.7

- Redesigned `drawWifiIcon()` as curved arcs + exclamation on disconnection. Added `updateTopWifiStatus()` so the top-page icon updates without switching pages.
- Replaced single large oval eye with `drawRobotEyes()` featuring two white eyes; Home (65x32) and Clock (264x90). Changes only on new pose, including ~170ms blink.
- Replaced the three-cloud scene with `drawWeatherScene()`, centered within Weather's existing top card, responding to WMO weather states.
- Retained menus, style palette, image icon files, weather API, NTP, DHT11, touch calibration, SDMMC, media browser, and settings.
- No new image format conversion is needed.
