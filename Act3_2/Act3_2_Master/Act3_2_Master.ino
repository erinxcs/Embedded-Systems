//Carl Jayson Eli Bonaobra
//CPE4A
// ===== Activity 3.2: Bluetooth MASTER =====

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// Replace with your Slave ESP32's MAC address (from Slave's Serial Monitor output)
uint8_t slaveAddress[6] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Master", true); // true = master mode

  Serial.println("Connecting to slave...");
  bool connected = SerialBT.connect(slaveAddress);

  if (connected) {
    Serial.println("Connected to Slave!");
  } else {
    Serial.println("Failed to connect. Retrying in loop()...");
  }
}

void loop() {
  if (!SerialBT.connected()) {
    SerialBT.connect(slaveAddress);
    delay(2000);
    return;
  }

  if (SerialBT.available()) {
    char incoming = SerialBT.read();
    Serial.write(incoming);
  }
  if (Serial.available()) {
    char outgoing = Serial.read();
    SerialBT.write(outgoing);
  }
}