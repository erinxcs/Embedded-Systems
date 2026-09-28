//Carl Jayson Eli Bonaobra
//CPE4A
// ===== Activity 1: LED Dimming Controlled by Potentiometer =====

const int potPin = 34;   // Potentiometer wiper -> ADC1 pin
const int ledPin = 2;    // LED pin

// PWM settings
const int pwmFreq = 5000;      // 5 kHz
const int pwmResolution = 8;   // 8-bit -> duty 0-255
const int pwmChannel = 0;      // Only needed for older core (v2.x)

void setup() {
  Serial.begin(115200);

  pinMode(potPin, INPUT);

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    // New core (v3.x): analogWrite works directly
    analogWrite(ledPin, 0);
  #else
    // Old core (v2.x): must use ledc functions
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
  #endif
}

void loop() {
  int potValue = analogRead(potPin);        // 0 - 4095 (12-bit ADC)
  int brightness = map(potValue, 0, 4095, 0, 255); // scale to 8-bit PWM

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    analogWrite(ledPin, brightness);
  #else
    ledcWrite(pwmChannel, brightness);
  #endif

  Serial.print("Pot Value: ");
  Serial.print(potValue);
  Serial.print("  |  Brightness: ");
  Serial.println(brightness);

  delay(50); // small delay for stability/readability
}