#include <Wire.h>
#include "ModuleComms.h"

#define BUTTON_PIN 2
#define LED_PIN 3

Master master(0x0100); // Version 1.0.0
bool gameInProgress = false;
bool allSolved = false;

void setup() {
    Serial.begin(9600);
    Wire.begin();
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    
    Serial.println("Master: Initializing...");
    master.begin();
    
    Serial.println("Master: Scanning for modules...");
    master.scanForModules();

    Serial.print("Master: Found ");
    Serial.print(master.getModuleCount());
    Serial.println(" module(s)");
}

void loop() {
    if (!gameInProgress && digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Master: Button pressed - starting game.");
        master.startGame();
        gameInProgress = true;
        delay(500); // Debounce delay
    }

    if (gameInProgress) {
        allSolved = true; // Assume all are solved until we check

        for (uint8_t i = 0; i < master.getModuleCount(); i++) {
            uint8_t status = master.getModuleStatus(i);
            Serial.print("Master: Module ");
            Serial.print(i);
            Serial.print(" status: ");

            if (status == STATUS_UNSOLVED) {
                Serial.println("UNSOLVED");
                allSolved = false;
            } else if (status == STATUS_PASSED) {
                Serial.println("PASSED");
            } else if (status == STATUS_FAILED) {
                Serial.println("FAILED");
                allSolved = false;
            } else {
                Serial.println("UNKNOWN");
            }
        }

        if (allSolved) {
            Serial.println("Master: All modules solved! Sending END_GAME signal.");
            master.endGame();

            Serial.println("Master: Flashing LED.");
            for (int i = 0; i < 5; i++) {  // Blink LED 5 times
                digitalWrite(LED_PIN, HIGH);
                delay(500);
                digitalWrite(LED_PIN, LOW);
                delay(500);
            }

            gameInProgress = false;
            while (1);
        }
        
        delay(1000); // Poll status every second
    }
}
