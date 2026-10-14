# V5.0 changes from V4.9

- Keeps V4.8 home, clock, weather animations, DHT11, SD icons, touch calibration.
- Keeps V4.9 JPEG photo gallery, full-screen photo viewer, slideshow and controls.
- Replaces VIDEO filename-only placeholder with actual SD-backed `.evm` silent-video player.
- 24-video alphabetically sorted list, 4 items/page, separate immersive player.
- Baseline-JPEG frame-by-frame decoding via existing TJpg_Decoder library; PSRAM frame buffer.
- Controls: BACK, PREV, PLAY/PAUSE, NEXT, LOOP/ONCE. Auto-hide overlay, looping default.
- Supplied Windows/Python converter using FFmpeg and Pillow plus a sample ambient video.
- SD and wiring are unchanged. No new hardware required.
