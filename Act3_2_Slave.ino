//Carl Jayson Eli Bonaobra
//CPE4A
// ===== Activity 3.2: Bluetooth SLAVE =====

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Slave"); // Bluetooth device name
  Serial.println("Slave started. Waiting for connection...");

  // Temporarily print this device's MAC address so you can copy it for the Master code:
  Serial.print("Slave BT Address: ");
  Serial.println(SerialBT.getBtAddressString());
}

void loop() {
  if (SerialBT.available()) {
    char incoming = SerialBT.read();
    Serial.write(incoming);
  }
  if (Serial.available()) {
    char outgoing = Serial.read();
    SerialBT.write(outgoing);
  }
}