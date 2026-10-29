# Activity 3.2: Bluetooth Master and Slave Communication

Two ESP32 boards exchange text over Bluetooth Classic. Characters typed into one board's Serial Monitor appear on the other board's Serial Monitor.

| Sketch | Role | Bluetooth name |
|---|---|---|
| [`Act3_2_Slave/`](Act3_2_Slave/Act3_2_Slave.ino) | Waits for a connection and prints its Bluetooth address | `ESP32_Slave` |
| [`Act3_2_Master/`](Act3_2_Master/Act3_2_Master.ino) | Connects to the slave by MAC address and reconnects when the link drops | `ESP32_Master` |

## Parts

- Two ESP32 DevKit boards (original ESP32)

## Usage

1. Upload `Act3_2_Slave.ino` to the first board and note the `Slave BT Address` printed at 115200 baud.
2. Replace `slaveAddress` in `Act3_2_Master.ino` with that address. For example, `24:6F:28:AA:BB:CC` becomes `{0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}`.
3. Upload `Act3_2_Master.ino` to the second board.
4. Open a Serial Monitor for each board and type into either one.

> **Board note:** `BluetoothSerial` uses Bluetooth Classic, which is available only on the original ESP32. It does not work on the ESP32-S2, S3, C3, or in Wokwi.
