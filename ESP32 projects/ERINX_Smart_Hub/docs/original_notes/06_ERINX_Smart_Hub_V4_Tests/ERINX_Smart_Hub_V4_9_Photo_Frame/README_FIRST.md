# ERINX SMART HUB V4.9 — Digital Photo Frame

## Working base
Built from your working V4.8. No TFT / touch / SD wiring changes, no new GPIOs. Weather, clock, DHT11, expressive robot eyes, Wi-Fi indicator, weather effects, V4.8 six-menu UI and SD icons remain.

## New V4.9 features
- Tap GALLERY to list JPEG files in `/photos` (up to 40, alphabetical), 4 per page.
- Tap a filename to open a **full-screen portrait photo**.
- PREV / NEXT to move through pictures. PLAY starts slideshow, PAUSE stops it.
- `FRAME` from gallery opens first photo with automatic slideshow.
- Controls auto-hide after 3 seconds. Tap the image to show controls. Top-left tap goes back to Gallery even when overlay hidden.
- Automatic slideshow changes photos every 7 seconds. Tune `SLIDE_INTERVAL_MS` in .ino.
- Center + contain; no stretching. When photo is not exactly 320×480, black letterboxing may appear.

## Required extra Arduino library
**TJpg_Decoder by Bodmer** via Arduino IDE > Library Manager. Its `drawFsJpg()` implementation lets the built-in **SD_MMC** reader serve as the JPEG filesystem. Do not substitute the SPI `SD` library.
Already used: TFT_eSPI, ArduinoJson v7, DHT by Adafruit, Adafruit Unified Sensor, ESP32 core built-ins.

## 1. Copy to microSD (FAT32)
The folder `01_COPY_TO_SD_CARD` contains your six prior icon files plus three baseline-JPEG test cards.
Copy the **icons** and **photos** folders directly to your microSD root. Keep your own existing icons if preferred.

```
SD CARD ROOT/
  icons/    weather.rgb565, clock.rgb565, dht11.rgb565, gallery.rgb565, video.rgb565, settings.rgb565
  photos/   ERINX_SAMPLE_01.jpg, ERINX_SAMPLE_02.jpg, ERINX_SAMPLE_03.jpg, YOUR_PHOTO.jpg
  videos/   (optional; still file browser only)
```

Important: **Do not copy nested project folders into the SD root**. Use `/photos/filename.jpg`, not `/01_COPY_TO_SD_CARD/photos/filename.jpg`.

## 2. Add your own photos
Best: **320×480, JPEG baseline, 24-bit RGB, not progressive, not CMYK**. Use the included PC script to batch-convert files without distorting faces. Install Pillow (`python -m pip install pillow`) then run:

```
python convert_photos.py "C:\Users\erinx\Pictures" "E:\photos"
```

Replace paths to match your computer and microSD drive. The converter only processes images directly inside the chosen source folder and will overwrite matching `PIC_001.jpg` etc in output, so back up similarly named files first.

The TJpg decoder can downsample oversized baseline JPEGs by factors 2, 4 or 8, but smaller, already portrait-prepared photos are more predictable and faster.

## 3. Upload sketch
Open `02_ARDUINO_CODE/ERINX_Smart_Hub_V4_9/ERINX_Smart_Hub_V4_9.ino`. Enter your Wi-Fi SSID/password in the code, retain your proven `TFT_eSPI/User_Setup.h` config, choose `ESP32S3 Dev Module`, Flash 16MB, OPI PSRAM, USB-UART settings as before. Verify, Upload.

## 4. Test sequence
Home -> Gallery -> sample_01.jpg -> tap controls -> NEXT -> PLAY -> wait for slideshow. Check `Serial Monitor 115200` for `PHOTOS: 3 JPEG files` and `PHOTO: ... [320x480] scale 1/1`. Restart after changing the SD card contents.

## Limitations
- Photos only: .jpg / .jpeg (PNG, HEIC, GIF not yet supported).
- Up to 40 photos, scanned when you enter the Gallery.
- File playback uses the MCU; changing a photo might briefly pause touch while decoding.
- **Videos remain a file browser**; actual MJPEG playback is reserved for the next version.
- Hardware integration must still be verified on your exact ESP32-S3 and TFT.
