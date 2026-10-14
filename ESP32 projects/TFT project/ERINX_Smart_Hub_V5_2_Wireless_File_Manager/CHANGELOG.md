# V5.2 — Wireless File Manager

- Added an on-device lightweight HTTP server (ESP32 Arduino `WebServer` on TCP port 80).
- Professional responsive dark navy/cyan browser interface: drag-and-drop and multi-file JPEG/EVM uploads, storage meter, library list, photo thumbnails and confirm-delete.
- Added six-digit per-boot access PIN displayed on TFT Settings; Basic authentication for file API; failed PIN rate-limiting.
- Added filename/path allowlist, size limits, file signature checks, temp-file + rename persistence, and no-overwrite handling.
- Avoid accessing SD for media management when the Video Player screen is active.
- Preserved V5.1.3 touchscreen navigation, location selector, Wi-Fi scanner, NTP clock, sensor readings, JPEG photo frame, EVM video playback, saved touch calibration, SD icons.
- No new pin wiring or SD assets required.
