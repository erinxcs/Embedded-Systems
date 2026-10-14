#include <Arduino.h>
#include <soc/soc_caps.h>
#ifndef CONFIG_IDF_TARGET_ESP32S3
#error "WRONG BOARD: Tools > Board > ESP32S3 Dev Module"
#endif
#if !defined(SOC_SDMMC_HOST_SUPPORTED) || !SOC_SDMMC_HOST_SUPPORTED
#error "SDMMC UNSUPPORTED BY SELECTED BOARD. Select ESP32S3 Dev Module."
#endif
#if defined(NO_GLOBAL_INSTANCES) || defined(NO_GLOBAL_SD_MMC)
#error "Build setting disables global SD_MMC object."
#endif
#include <FS.h>
#include <SD_MMC.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("ERINX SDMMC BOARD DIAGNOSTIC");
  Serial.println("Selected target: ESP32-S3");
  if (!SD_MMC.setPins(39, 38, 40)) {
    Serial.println("setPins failed");
    return;
  }
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("Card mount failed. Check SD card FAT32 / socket.");
    return;
  }
  Serial.printf("SDMMC READY: %llu MB\n", SD_MMC.cardSize() / 1048576ULL);
}

void loop() {}
