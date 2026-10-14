/*
  ERINX Smart Hub V5.1 - EVM real FPS benchmark (PORTRAIT 320x480)
  ESP32-S3 CAM N16R8 + 3.5in ILI9488 SPI TFT + built-in microSD
  Uses your existing working TFT_eSPI User_Setup.h and wiring.

  IMPORTANT: Tools > PSRAM > OPI PSRAM, then REUPLOAD the sketch.
  This is an uncapped playback throughput measurement, not a normal
  audio/video player. It measures the exact JPEG-to-TFT path that
  ERINX V5.0 uses, but without UI overlays / weather in the loop.

  To compare SPI frequencies, change only the ONE #define SPI_FREQUENCY
  in TFT_eSPI/User_Setup.h (10000000 -> 20000000 -> 27000000),
  restart/Verify/Upload this SAME sketch for each run.
  Never change pins or firmware SD settings.
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <SD_MMC.h>
#include <FS.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <string.h>

#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY 0
#endif

TFT_eSPI tft = TFT_eSPI();
static const uint32_t MAX_JPEG = 128UL * 1024UL;
static uint8_t *jpgBuf = nullptr;
static uint64_t lcdCallbackUs = 0;
static uint32_t lcdCallbackCount = 0;
static const uint8_t PASSES = 2;
static const uint8_t WARMUP_FRAMES = 3;

struct BenchResult {
  const char *name;
  uint16_t width = 0, height = 0;
  uint32_t frames = 0;
  uint64_t totalUs = 0, sdUs = 0, jpegUs = 0, lcdUs = 0;
  uint64_t totalBytes = 0;
  uint32_t jpegBlocks = 0;
  bool success = false;
};

BenchResult results[3];
static const char *files[3] = {
  "/bench/TEST_160x240.evm",
  "/bench/TEST_240x360.evm",
  "/bench/TEST_320x480.evm"
};

uint64_t nowUs() { return (uint64_t)esp_timer_get_time(); }

// TJpgDec provides decoded RGB565 blocks. This callback measures
// the actual time spent pushing those pixel blocks to the TFT.
bool renderJpgBlock(int16_t x, int16_t y,
                    uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (x >= 320 || y >= 480) return false;
  uint64_t t0 = nowUs();
  tft.pushImage(x, y, w, h, bitmap);
  lcdCallbackUs += nowUs() - t0;
  lcdCallbackCount++;
  return true;
}

void drawStatus(const char *line1, const char *line2 = "") {
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(1);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("ERINX FPS BENCH", 13, 18, 1);
  tft.setTextSize(1);
  tft.drawString(line1, 15, 79, 1);
  tft.drawString(line2, 15, 103, 1);
}

// Read exactly n bytes from the EVM SD stream (unbuffered file read).
bool readExact(fs::File &file, uint8_t *buf, size_t n) {
  return file.read(buf, n) == n;
}

bool readU16(fs::File &file, uint16_t &v) {
  uint8_t b[2];
  if (!readExact(file, b, 2)) return false;
  v = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
  return true;
}

bool readU32(fs::File &file, uint32_t &v) {
  uint8_t b[4];
  if (!readExact(file, b, 4)) return false;
  v = (uint32_t)b[0] | ((uint32_t)b[1] << 8) |
      ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
  return true;
}

bool readAndRender(fs::File &file, BenchResult &r, bool countThisFrame) {
  uint64_t t0 = nowUs();
  uint32_t size = 0;
  if (!readU32(file, size) || size < 100 || size > MAX_JPEG) return false;
  const size_t pos = file.position();
  if (pos > file.size() || file.size() - pos < size) return false;
  if (!readExact(file, jpgBuf, size)) return false;
  if (jpgBuf[0] != 0xff || jpgBuf[1] != 0xd8) return false;
  uint64_t sdElapsed = nowUs() - t0;

  uint64_t lcdBefore = lcdCallbackUs;
  uint32_t callbacksBefore = lcdCallbackCount;
  uint64_t decodeStart = nowUs();
  JRESULT status = TJpgDec.drawJpg((320 - (int)r.width) / 2,
                                  (480 - (int)r.height) / 2,
                                  jpgBuf, size);
  uint64_t jpegElapsed = nowUs() - decodeStart;
  if (status != JDR_OK) {
    Serial.printf("ERROR: JPEG decoder returned %u\n", (unsigned)status);
    return false;
  }
  if (countThisFrame) {
    r.frames++;
    r.sdUs += sdElapsed;
    r.jpegUs += jpegElapsed;
    r.lcdUs += lcdCallbackUs - lcdBefore;
    r.jpegBlocks += lcdCallbackCount - callbacksBefore;
    r.totalBytes += size;
  }
  return true;
}

void benchmarkFile(int index) {
  BenchResult &r = results[index];
  r.name = files[index];
  Serial.printf("\n--- TEST %d/3: %s ---\n", index + 1, r.name);
  drawStatus("Benchmarking...", r.name);
  fs::File f = SD_MMC.open(r.name, FILE_READ);
  if (!f) {
    Serial.printf("ERROR: missing %s\n", r.name);
    return;
  }
  uint8_t magic[4];
  uint16_t encodedFps = 0, reserved = 0;
  uint32_t count = 0;
  if (!readExact(f, magic, 4) || memcmp(magic, "ERXV", 4) != 0 ||
      !readU16(f, r.width) || !readU16(f, r.height) ||
      !readU16(f, encodedFps) || !readU16(f, reserved) ||
      !readU32(f, count) ||
      r.width == 0 || r.height == 0 || r.width > 320 || r.height > 480 ||
      count == 0 || count > 10000) {
    Serial.println("ERROR: invalid EVM header");
    f.close();
    return;
  }
  Serial.printf("Video %ux%u, encoded fps=%u (benchmark ignores source fps), frames=%lu\n",
                r.width, r.height, encodedFps, (unsigned long)count);
  TJpgDec.setJpgScale(1);
  tft.fillScreen(TFT_BLACK);
  // Warm-up excludes initial cache/decoder/display effects.
  for (uint32_t i = 0; i < WARMUP_FRAMES && i < count; ++i) {
    if (!readAndRender(f, r, false)) {
      Serial.println("ERROR: warm-up frame failed");
      f.close();
      return;
    }
  }
  // No Serial printing, FPS cap, or button redrawing inside timed section.
  uint64_t begin = nowUs();
  for (uint8_t pass = 0; pass < PASSES; pass++) {
    if (!f.seek(16)) { Serial.println("ERROR: EVM seek failed"); f.close(); return; }
    for (uint32_t i = 0; i < count; ++i) {
      if (!readAndRender(f, r, true)) {
        Serial.printf("ERROR: frame %lu failed\n", (unsigned long)i);
        f.close();
        return;
      }
    }
  }
  r.totalUs = nowUs() - begin;
  r.success = (r.frames == (uint32_t)PASSES * count && r.totalUs > 0);
  f.close();

  if (r.success) {
    const double fps = (double)r.frames * 1000000.0 / (double)r.totalUs;
    Serial.printf("RESULT: %ux%u | %.2f FPS | %lu frames\n",
                  r.width, r.height, fps, (unsigned long)r.frames);
    Serial.printf("  average SD read = %.2f ms/frame\n", (double)r.sdUs / r.frames / 1000.0);
    Serial.printf("  average JPEG+screen = %.2f ms/frame\n", (double)r.jpegUs / r.frames / 1000.0);
    Serial.printf("  average LCD push (inside JPEG) = %.2f ms/frame\n", (double)r.lcdUs / r.frames / 1000.0);
    Serial.printf("  average frame bytes = %lu | avg draw blocks = %.1f\n",
                  (unsigned long)(r.totalBytes / r.frames),
                  (double)r.jpegBlocks / r.frames);
  }
}

void displayResults() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(1);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("ERINX FPS RESULTS", 10, 18, 1);
  tft.setTextSize(1);
  char line[70];
  snprintf(line, sizeof(line), "SPI setting: %lu MHz", (unsigned long)(SPI_FREQUENCY / 1000000UL));
  tft.drawString(line, 12, 69, 1);
  for (int i = 0; i < 3; ++i) {
    BenchResult &r = results[i];
    if (r.success) {
      const double fps = (double)r.frames * 1000000.0 / (double)r.totalUs;
      snprintf(line, sizeof(line), "%ux%u = %.2f FPS", r.width, r.height, fps);
    } else {
      snprintf(line, sizeof(line), "%d. ERROR - SEE SERIAL", i + 1);
    }
    tft.drawString(line, 12, 123 + 82 * i, 1);
  }
  tft.drawString("NO FPS CAP: REAL RENDER SPEED", 12, 402, 1);
  tft.drawString("Serial: 115200 | RST to repeat", 12, 434, 1);
}

// Save summary only AFTER the benchmark to avoid affecting FPS.
void saveCsv() {
  fs::File log = SD_MMC.open("/ERINX_FPS_RESULTS.csv", FILE_APPEND);
  if (!log) {
    Serial.println("Note: CSV log unavailable (Serial output is enough)");
    return;
  }
  for (int i = 0; i < 3; ++i) {
    const BenchResult &r = results[i];
    if (!r.success) continue;
    const double fps = (double)r.frames * 1000000.0 / (double)r.totalUs;
    log.printf("%lu,%u,%u,%lu,%.3f,%.3f,%.3f,%.3f\n",
               (unsigned long)(SPI_FREQUENCY / 1000000UL),
               r.width, r.height, (unsigned long)r.frames,
               fps, (double)r.sdUs / r.frames / 1000.0,
               (double)r.jpegUs / r.frames / 1000.0,
               (double)r.lcdUs / r.frames / 1000.0);
  }
  log.close();
  Serial.println("Wrote: /ERINX_FPS_RESULTS.csv");
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("\n=== ERINX SMART HUB V5.1 FPS BENCHMARK ===");
  Serial.printf("Configured TFT SPI: %lu Hz (compile-time User_Setup.h)\n",
                (unsigned long)SPI_FREQUENCY);
  Serial.printf("PSRAM found: %s | PSRAM size: %lu bytes\n",
                psramFound() ? "YES" : "NO", (unsigned long)ESP.getPsramSize());

  tft.init();
  tft.setRotation(0);
  drawStatus("Booting FPS test...");

  if (!psramFound() || ESP.getPsramSize() == 0) {
    Serial.println("STOP: PSRAM not detected. Tools > PSRAM > OPI PSRAM, then reupload.");
    drawStatus("PSRAM NOT DETECTED", "Tools > PSRAM > OPI PSRAM");
    return;
  }
  jpgBuf = (uint8_t*)heap_caps_malloc(MAX_JPEG, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!jpgBuf) {
    Serial.println("STOP: Failed to allocate 128KB PSRAM frame buffer");
    drawStatus("PSRAM ALLOCATION FAILED", "See Serial Monitor");
    return;
  }
  if (!SD_MMC.setPins(39,38,40) || !SD_MMC.begin("/sdcard", true) ||
      SD_MMC.cardType() == CARD_NONE) {
    Serial.println("STOP: SDMMC not ready; check SD card FAT32 and built-in SD slot");
    drawStatus("SD CARD FAILED", "FAT32 /bench/ files needed");
    return;
  }
  Serial.printf("microSD initialized: %llu MB\n", SD_MMC.cardSize() / 1048576ULL);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(renderJpgBlock);

  for (int i = 0; i < 3; ++i) benchmarkFile(i);
  displayResults();
  Serial.println("\nSPI_MHz,width,height,frames,uncapped_FPS,SD_ms_per_frame,JPEG_plus_screen_ms,LCD_push_ms");
  for (int i = 0; i < 3; ++i) {
    BenchResult &r = results[i];
    if (!r.success) continue;
    const double fps = (double)r.frames * 1000000.0 / (double)r.totalUs;
    Serial.printf("%lu,%u,%u,%lu,%.3f,%.3f,%.3f,%.3f\n",
                  (unsigned long)(SPI_FREQUENCY / 1000000UL),
                  r.width, r.height, (unsigned long)r.frames,
                  fps, (double)r.sdUs / r.frames / 1000.0,
                  (double)r.jpegUs / r.frames / 1000.0,
                  (double)r.lcdUs / r.frames / 1000.0);
  }
  saveCsv();
  Serial.println("\nBENCHMARK DONE. Paste these results back in ChatGPT.");
}

void loop() { delay(100); }
