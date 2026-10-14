# V5.1.3 Functional Testing Checklist

Use real device runs to fill in Pass / Fail / Not Tested and attach photos or serial logs. Do not treat a static file check as hardware verification.

| ID | Test case | Expected outcome | Actual result |
|---|---|---|---|
| T01 | Boot with ESP32-S3 N16R8, TFT, touch | Home UI appears in portrait orientation | NOT RECORDED |
| T02 | First-time touch calibration | Touch points align and calibration survives reboot | NOT RECORDED |
| T03 | Open six home tiles and BACK | Each page displays and BACK returns home | NOT RECORDED |
| T04 | DHT11 reading | Local indoor temp/RH displayed or clear missing-sensor status | NOT RECORDED |
| T05 | Wi-Fi scan + password entry | Available 2.4 GHz networks selectable; SHOW/HIDE works | NOT RECORDED |
| T06 | Wi-Fi connect/reboot/forget | Valid connection saved; FORGET clears locally stored credentials | NOT RECORDED |
| T07 | Choose each weather location | Selected city changes; Open-Meteo data refreshes with internet | NOT RECORDED |
| T08 | Disconnect internet | Clock reports unsynced/uptime; weather shows offline behavior | NOT RECORDED |
| T09 | Insert FAT32 card with icons | SD status READY and icon files render | NOT RECORDED |
| T10 | Browse photo JPG | Photo can be opened and previous/next navigation works | NOT RECORDED |
| T11 | Start auto slideshow | Changes photo about every 7 seconds | NOT RECORDED |
| T12 | Browse and play `.evm` | Silent JPEG-frame playback; play/pause/prev/next | NOT RECORDED |
| T13 | Reboot without microSD | Graceful SD offline status, no unexpected crash | NOT RECORDED |
| T14 | Upload with ESP32S3 selected and PageTypes.h in folder | Arduino IDE Verify successful | NOT RECORDED |

**Static archive review:** All included .ino files remain byte-identical to their source entries in the original ZIP; V5.1.3 .ino and PageTypes.h are collocated. Assets were consolidated by hash, with source mappings in `SOURCE_FILE_MANIFEST.csv`. **Arduino build and physical tests were not executed in this workspace.**
