# ERINX Smart Hub — version roadmap

## Already included

1. **V0.1 Display boot/loading** — verify LCD SPI, font, color blocks, loading bar, moving animation, no touch, no Wi-Fi.
2. **V0.2 Touch diagnostic** — XPT2046 touch calibration and coordinate mapping, save reusable calibration.
3. **V1 Weather (no touch)** — Wi-Fi + Open-Meteo + Philippines NTP clock; live outdoor conditions and an animated symbol.
4. **V2 Weather + touchscreen** — basic page navigation, locations, refresh and weather details.
5. **V3 ERINX Smart Hub** — blue portrait home design, rounded weather/time cards, animated icon and touch menu.

## Future work (not yet implemented in this ZIP)

**V4 — Indoor vs. outdoor weather:** add optional **DHT11** sensor, clearly label indoor temperature/humidity separately from Open-Meteo outdoor conditions. Potential wiring for a common 3-pin DHT11 breakout: VCC 3.3V, GND GND, DATA GPIO32. Check your board and any pull-up requirement before wiring. Add Adafruit DHT sensor library (or equivalent) when implementing.

**V5 — UI optimization:** reduce full-screen redraws, prefer bounded sprites for time/icon/text, clean Wi-Fi reconnection and error labels, improve debounced touch navigation.

**V6 — Weather explorer:** hourly forecast and daily forecast pages using Open-Meteo; pagination/swipe; caching, no blocking requests in gesture path where possible.

**V7 — Touch productivity:** calendar, Pomodoro timer, reminders, a simple checklist saved in ESP32 Preferences; no added components required.

**V8 — Polished final integration:** profile/theme settings, persistent selected location, better low-memory behavior, HTTPS certificate validation, README/wiring/tests/screenshots for a public GitHub repository.

## Optimization approach

- Keep a stable, working version folder for each release; don't overwrite older demos.
- Store Wi-Fi credentials locally and never upload secrets to GitHub.
- Draw fixed backgrounds once; update only changed parts or use small `TFT_eSprite` buffers to save memory.
- Keep display logic, network, touch, and data model conceptually separate. Later split functions into `.h/.cpp` if a single sketch becomes unwieldy.
- Use non-blocking `millis()` schedules for UI animations; API requests currently block for up to their configured timeouts and should be separated later for better responsiveness.
- Use Arduino IDE serial output (115200) to troubleshoot firmware; verify touch calibration after changing rotation.
