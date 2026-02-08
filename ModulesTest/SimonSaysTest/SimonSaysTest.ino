#include <Arduino.h>

const int LED_PINS[4] = {12, 11, 10, 9};
const int BUTTON_PINS[4] = {5, 6, 7, 8};

bool lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};

void setup() {
  Serial.begin(9600);
  delay(500); 
  
  Serial.println("\n--- Simon Says Hardware Test ---");
  Serial.println("Standard LEDs: HIGH=ON");

  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW); // Start OFF
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
}

void loop() {
  for (int i = 0; i < 4; i++) {
    int reading = digitalRead(BUTTON_PINS[i]);

    if (reading != lastButtonState[i]) {
      if (reading == LOW) {
         Serial.print("BTN "); Serial.print(BUTTON_PINS[i]); Serial.println(" ON");
         digitalWrite(LED_PINS[i], HIGH);
      } else {
         Serial.print("BTN "); Serial.print(BUTTON_PINS[i]); Serial.println(" OFF");
         digitalWrite(LED_PINS[i], LOW);
      }
      lastButtonState[i] = reading;
      delay(50); 
    }
  }
}