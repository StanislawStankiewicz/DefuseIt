#include <Wire.h>
#include "ModuleComms.h"

#define BUTTON_PIN 2
#define LED_PIN 3

// Function prototypes
void gameLoop();
void setInitialState();

Slave module(0x10, gameLoop, setInitialState, LED_PIN);
bool isSolved = false;

void setInitialState() {
    isSolved = false;
}

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);  // For forwarding Master's Serial output
    Serial.println("Slave 1: initializing with Serial bridge");
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(9, OUTPUT);
    digitalWrite(9, HIGH);
    
    module.begin();
}

void gameLoop() {
    if (!isSolved && digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Slave 1: Solved!");
        module.pass();
        isSolved = true;
    }
}

void loop() {
    // Forward Master's Serial output to PC
    while (Serial1.available()) {
        char c = Serial1.read();
        Serial.write(c);
    }

    // Handle I2C slave functionality
    module.slaveLoop();
}
