#include <Wire.h>
#include "ModuleComms.h"

#define BUTTON_PIN 2
#define LED_PIN 3

Slave module(0x10); // I2C address 0x10
bool isSolved = false;
bool gameStarted = false;

void handleGameStart() {
    Serial.println("Slave 1: Game started!");
    digitalWrite(LED_PIN, LOW);
    isSolved = false;
    gameStarted = true; // Now the module can be solved
}

void handleGameEnd() {
    Serial.println("Slave 1: Game ended!");
    digitalWrite(LED_PIN, LOW);
    gameStarted = false;
}

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(9, OUTPUT);  // Set PB1 (Pin 9) as an output
    digitalWrite(9, HIGH);
    
    module.begin();
    module.onGameStart(handleGameStart);
    module.onGameEnd(handleGameEnd);
}

void loop() {
    if (gameStarted && !isSolved && digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Slave 1: Solved!");
        module.setStatus(STATUS_PASSED);
        digitalWrite(LED_PIN, HIGH);
        isSolved = true; // Lock state to solved
        delay(500); // Debounce delay
    }
}
