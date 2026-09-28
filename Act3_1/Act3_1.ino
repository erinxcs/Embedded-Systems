//Carl Jayson Eli Bonaobra
//CPE4A
// ===== Activity 3.1: LED Control via Bluetooth Serial =====

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;
const int ledPin = 2;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  SerialBT.begin("ESP32_LED_BT"); // Bluetooth device name
  Serial.println("Bluetooth LED Control Ready. Pair with 'ESP32_LED_BT'.");
}

void loop() {
  if (SerialBT.available()) {
    char command = SerialBT.read();

    if (command == '1') {
      digitalWrite(ledPin, HIGH);
      SerialBT.println("LED turned ON");
      Serial.println("LED turned ON");
    }
    else if (command == '0') {
      digitalWrite(ledPin, LOW);
      SerialBT.println("LED turned OFF");
      Serial.println("LED turned OFF");
    }
  }
}