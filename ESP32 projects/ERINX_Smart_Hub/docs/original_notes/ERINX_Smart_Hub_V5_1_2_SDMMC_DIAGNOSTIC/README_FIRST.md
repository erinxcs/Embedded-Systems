ERINX SMART HUB V5.1.2 — SD_MMC BUILD DIAGNOSTIC

Why did V5.1.1 fail?
V5.1.1 ALREADY included <SD_MMC.h>. The ESP32 core only defines the
SD_MMC object when the selected chip supports the SDMMC host controller
and global instances have not been disabled. A wrong/changed Board setting,
or unusual macro configuration, can therefore cause exactly the error seen.

BEFORE COMPILING:
1. In Arduino IDE select Tools > Board > esp32 > ESP32S3 Dev Module.
   Do NOT select "ESP32 Dev Module", "ESP32C3 Dev Module" or "ESP32C6 Dev Module".
2. Tools > Flash Size = 16MB; PSRAM = OPI PSRAM; Port = ESP32-S3 COM port.
3. Save/close Arduino IDE, reopen this NEW sketch:
   ERINX_Smart_Hub_V5_1_2_Network_Location/
     ERINX_Smart_Hub_V5_1_2_Network_Location.ino
   PageTypes.h must remain in the same folder.
4. Click Verify, then Upload.
5. Do not change working TFT_eSPI User_Setup.h or wiring.

IF YOU STILL HAVE THE ERROR:
- Open SDMMC_Board_Probe/SDMMC_Board_Probe.ino and click Verify.
- This small sketch will explicitly explain incorrect board selection or
  a disabled SDMMC global.
- Send the FULL FIRST 25 lines of the compile error (not just the final 5),
  and a screenshot of the Arduino IDE Tools > Board menu.
- A line saying Multiple libraries were found for "SD.h" is normally only
  informational; it is NOT the cause of the SD_MMC symbol error.

Your existing SD icons/photos/videos are unchanged. The optional SD assets
folder has copies only; there is no need to format or recopy the SD card.

VALIDATION LIMITATION:
The sketch was checked using a local C++ syntax harness. A complete
Arduino ESP32 3.3.12 compile and on-device test must still be done in the IDE.
