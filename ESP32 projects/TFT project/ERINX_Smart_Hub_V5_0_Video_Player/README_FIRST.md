# ERINX SMART HUB V5.0 — Video Player / Animated Portrait

**Hardware:** ESP32-S3 CAM N16R8, built-in SDMMC card slot, 3.5-inch portrait TFT (320×480).  **No wiring or TFT_eSPI `User_Setup.h` changes.**

This version extends V4.9: all previous Weather / Clock / Eyes / DHT11 / Photo Gallery / touch calibration / Wi-Fi and SD icons remain.

## 1. First test: sample video

1. Power off / unplug the ESP32-S3, remove your FAT32 microSD.
2. From `01_COPY_TO_SD_CARD`, copy **only the `videos` folder** to the root of your SD card. You can keep your existing `icons` and `photos` folders unchanged.
3. Confirm the file is `/videos/ERINX_SAMPLE_LOOP.evm` (not nested beneath an extra folder).
4. Safely eject the microSD, put it back into the **ESP32-S3 camera board's built-in slot**, reconnect USB-C.
5. Open `02_ARDUINO_CODE/ERINX_Smart_Hub_V5_0/ERINX_Smart_Hub_V5_0.ino` in Arduino IDE.
6. Use the **same working** TFT_eSPI `User_Setup.h`, `ESP32S3 Dev Module`, Flash 16MB, **OPI PSRAM**, and USB-UART settings. Wi-Fi credentials are optional for local video playback. Install `TJpg_Decoder by Bodmer` if not already installed for V4.9.
7. Verify + Upload. Open Serial Monitor at **115200 baud**, then Home → VIDEO → tap `ERINX_SAMPLE_LOOP.evm`.

### Touch controls

- **Top-left BACK** returns to video list (including when controls hidden).
- **Bottom bar:** PREV / PLAY or PAUSE / NEXT / LOOP or ONCE.
- **Tap the picture** to reveal or hide controls. Controls auto-hide after 3 s.
- **LOOP** is enabled by default: the clip repeats indefinitely. **ONCE** stops after one play.

The sample is intentionally silent and short, for testing. It is **not an MP4**.

## 2. Convert your own MP4 or MOV on Windows

You already have Pillow installed. You also need **FFmpeg** to read video:

```powershell
winget install --id Gyan.FFmpeg -e
```

Restart PowerShell afterward, then check `ffmpeg -version`. If Winget cannot find it, install FFmpeg from https://ffmpeg.org/download.html and add FFmpeg's `bin` folder to your PATH.

Example folder:

```text
C:\Users\erinx\Documents\VIDEOS\
    convert_videos.py
    my_clip.mp4
    another_video.mov
```

Put `convert_videos.py` from `03_PC_VIDEO_CONVERTER` into that folder; open PowerShell there and run:

```powershell
python .\convert_videos.py "." ".\ConvertedVideos"
```

Expected output:

```text
ConvertedVideos\
    my_clip.evm
    another_video.evm
```

Copy **only the `.evm` files** into the SD card's `/videos` folder. Power off before removing/inserting the SD card; safely eject from Windows first.

Optionally choose the first 20 seconds at 6 fps:

```powershell
python .\convert_videos.py "." ".\ConvertedVideos" --seconds 20 --fps 6
```

For a smoother test when video is slow, try smaller JPEG frames:

```powershell
python .\convert_videos.py "." ".\ConvertedVideos" --fps 5 --width 160 --height 240 --quality 55
```

If your setup can handle it, optional larger portrait images (potentially much slower):

```powershell
python .\convert_videos.py "." ".\ConvertedVideos" --width 320 --height 480 --fps 5
```

## 3. Format and performance

We use `.evm` (ERINX Video MJPEG), a **custom length-prefixed stream of individual baseline JPEG frames**. It is not directly compatible with normal MP4/AVI files or arbitrary `.mjpg` downloads. Use the provided converter.

- Default: **240×360**, **7 target FPS**, **12-second maximum clip**; black/blue letterboxing, no stretching, no audio.
- MCU JPEG buffer is allocated in **8 MB PSRAM** (`128 KiB` maximum compressed frame size). If the Serial Monitor says `PSRAM unavailable`, check **Tools → PSRAM → OPI PSRAM**.
- The achieved FPS depends on TFT SPI speed, decoder and SD speed, and will be lower if frames take too long. **7 FPS is a file playback target, not a promised measurement.** No frame skipping or A/V sync yet.
- First release is intentionally **silent**: no microphone or external speaker hardware required. Future I2S audio is a separate project.
- Up to 24 videos per folder, 4 filenames visible at a time.
- For photos, continue to use `/photos` and the V4.9 Gallery.

## 4. SD root layout

```text
SD ROOT/
  icons/     weather.rgb565, clock.rgb565, dht11.rgb565,
             gallery.rgb565, video.rgb565, settings.rgb565
  photos/    your JPEG pictures from V4.9
  videos/    ERINX_SAMPLE_LOOP.evm, your_clip.evm
```

## 5. Troubleshooting

- **VIDEO page says NO VIDEOS FOUND:** verify `.evm` files are in SD root `/videos`, not `/01_COPY_TO_SD_CARD/videos`. Files are scanned when you enter VIDEO; return Home and reenter after updating card.
- **Invalid .evm file/header:** your clip is MP4 or ordinary MJPEG with the extension renamed. Convert it using the Python script; do not simply rename.
- **VIDEO ERROR: PSRAM unavailable:** select `OPI PSRAM` and confirm N16R8 board.
- **Bad or oversized JPEG frame:** lower quality/resolution and reconvert; maximum encoded JPEG frame is 128 KiB.
- **Slow/sluggish playback:** try `--fps 5 --width 160 --height 240`; test with the sample clip; optimize TFT SPI later only after stability is confirmed.
- **Compilation error:** ensure `TJpg_Decoder by Bodmer`, `TFT_eSPI by Bodmer`, `ArduinoJson`, `DHT sensor library by Adafruit`, `Adafruit Unified Sensor` are installed; restart Arduino IDE if previously installed library is not found.

**Testing done here:** local C++ mock-header syntax check, converter test from both GIF and MP4 sources, validated the sample EVM container and every embedded baseline JPEG. Physical ESP32-S3/TFT upload & FPS not tested here.
