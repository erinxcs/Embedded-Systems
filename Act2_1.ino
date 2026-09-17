
//Carl Jayson Eli Bonaobra
//CPE4A
// ===== Activity 2.1: LED Control Using Serial Communication =====

const int ledPin = 2;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.println("LED Serial Control Ready.");
  Serial.println("Send '1' to turn LED ON, '0' to turn LED OFF.");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (command == '1') {
      digitalWrite(ledPin, HIGH);
      Serial.println("LED turned ON");
    } 
    else if (command == '0') {
      digitalWrite(ledPin, LOW);
      Serial.println("LED turned OFF");
    }
    // Ignore other characters (like newline '\n' or carriage return '\r')
  }
}