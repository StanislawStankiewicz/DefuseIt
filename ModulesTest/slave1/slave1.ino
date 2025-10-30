#include <Wire.h>
#include "ModuleComms.h"

#define BUTTON_PIN 2
#define LED_PIN 3

Slave module(0x10);
bool isSolved = false;
bool gameStarted = false;

void handleGameStart() {
    digitalWrite(LED_PIN, LOW);
    isSolved = false;
    gameStarted = true;
}

void handleGameEnd() {
    digitalWrite(LED_PIN, LOW);
    gameStarted = false;
}

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);  // For forwarding Master's Serial output
    Serial.println("Slave 1: initializing with Serial bridge");
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(9, OUTPUT);
    digitalWrite(9, HIGH);
    
    module.begin();
    module.onGameStart(handleGameStart);
    module.onGameEnd(handleGameEnd);
}

void loop() {
    // Forward Master's Serial output to PC
    while (Serial1.available()) {
        char c = Serial1.read();
        Serial.write(c);
    }

    // Handle I2C slave functionality
    if (gameStarted && !isSolved && digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Slave 1: Solved!");
      module.setStatus(STATUS_PASSED);
      digitalWrite(LED_PIN, HIGH);
      isSolved = true;
      delay(500);
    }
}
