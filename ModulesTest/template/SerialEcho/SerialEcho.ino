#include <Arduino.h>

// Bridge between USB Serial (PC) and hardware Serial1 (external MCU).
// Assumes the board provides Serial1 (e.g., Leonardo, Micro, Mega).

void setup() {
  Serial.begin(115200); // USB <> PC
  Serial1.begin(9600);  // Serial1 <> external MCU (adjust if needed)
  Serial.println("Serial <-> Serial1 bridge started");
}

void loop() {
  // External MCU -> PC
  while (Serial1.available()) {
    int b = Serial1.read();
    Serial.write((uint8_t)b);
  }

  // PC -> External MCU
  while (Serial.available()) {
    int b = Serial.read();
    Serial1.write((uint8_t)b);
  }
}