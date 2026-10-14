# V5.2 Functional Testing Checklist

Use real device runs to fill in Pass / Fail / Not Tested and attach photos or serial logs. A static file check is not a substitute for hardware verification.

## Core device tests

| ID | Test case | Expected outcome | Actual result |
|---|---|---|---|
| T01 | Boot with ESP32-S3 N16R8, TFT, and touch | Home UI appears in portrait orientation | NOT RECORDED |
| T02 | First-time touch calibration | Touch points align and calibration survives reboot | NOT RECORDED |
| T03 | Open all six home tiles and BACK | Each page displays and BACK returns home | NOT RECORDED |
| T04 | Read DHT11 | Indoor temperature/RH appears or a clear missing-sensor status is shown | NOT RECORDED |
| T05 | Scan Wi-Fi and enter password | 2.4 GHz networks are selectable and SHOW/HIDE works | NOT RECORDED |
| T06 | Connect, reboot, and forget Wi-Fi | Valid credentials persist; FORGET clears saved credentials | NOT RECORDED |
| T07 | Choose each weather location | Selected city changes and Open-Meteo refreshes | NOT RECORDED |
| T08 | Disconnect internet | Clock/weather report a clear offline or unsynchronized state | NOT RECORDED |
| T09 | Insert FAT32 card with icons | SD status is READY and icons render | NOT RECORDED |
| T10 | Browse baseline JPG files | Photo opens and previous/next navigation works | NOT RECORDED |
| T11 | Start auto slideshow | Photo changes at the expected interval | NOT RECORDED |
| T12 | Browse and play `.evm` | Silent playback and play/pause/previous/next controls work | NOT RECORDED |
| T13 | Reboot without microSD | Clear SD-offline status with no unexpected crash | NOT RECORDED |

## V5.2 wireless file-manager tests

| ID | Test case | Expected outcome | Actual result |
|---|---|---|---|
| T14 | Open displayed local URL | PIN screen loads on desktop and mobile browsers | NOT RECORDED |
| T15 | Enter invalid PIN repeatedly | Access is rejected and rate limiting activates | NOT RECORDED |
| T16 | Enter current on-device PIN | Media library and upload controls become available | NOT RECORDED |
| T17 | Upload baseline JPG under 6 MiB | File appears under `/photos/` and opens on the TFT | NOT RECORDED |
| T18 | Upload valid EVM under 60 MiB | File appears under `/videos/` and plays on the TFT | NOT RECORDED |
| T19 | Upload unsupported or duplicate file | Request is rejected with a useful error | NOT RECORDED |
| T20 | Delete a media file through the browser | Correct file is removed and the list refreshes | NOT RECORDED |
| T21 | Interrupt an upload and reboot | Temporary `.erinx_incoming` content is cleaned safely | NOT RECORDED |
| T22 | Use file manager while video is playing | Unsafe SD operation is blocked or handled without corruption | NOT RECORDED |

## Build verification

| ID | Test case | Expected outcome | Actual result |
|---|---|---|---|
| T23 | Arduino IDE Verify with all four source files together | Build succeeds for ESP32S3 Dev Module | NOT RECORDED |
| T24 | Flash with 16 MB and OPI PSRAM selected | Device boots without PSRAM or partition errors | NOT RECORDED |
| T25 | Run photo and video converters on sample inputs | Output files are accepted by the firmware | NOT RECORDED |

## Current evidence

- V5.2 source files are collocated in `firmware/final/ERINX_Smart_Hub_V5_2/`.
- The copied canonical files match the archived V5.2 package byte-for-byte.
- Desktop/mobile preview assets and converter utilities are present.
- A full Arduino toolchain build and physical regression test have not yet been executed in this workspace.
