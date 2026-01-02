#include <Arduino.h>

void setup() {
  Serial.begin(9600);   // USB Serial (PC)
  Serial1.begin(9600);  // Hardware Serial (Pins 0 and 1)
}

void loop() {
  // Forward data received on Pins 0/1 (Serial1) to PC (Serial)
  if (Serial1.available()) {
    Serial.write(Serial1.read());
  }
}
