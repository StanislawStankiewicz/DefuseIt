#include <Wire.h>
#include "ModuleComms.h"
#include <TM1637Display.h>
#include <math.h>

#define BUTTON_PIN     2  // pin 4
#define GREEN_LED_PIN  3  // pin 5
#define RED_LED_PIN    4  // pin 6
#define BUZZER_PIN     9  // pin 15

#define DISPLAY1_CLK   5  // pin 11
#define DISPLAY1_DIO   6  // pin 12
#define DISPLAY2_CLK   7  // pin 13
#define DISPLAY2_DIO   8  // pin 14

const int gameDurationSeconds = 300;
const int initialInterval = 10000;
const int finalInterval = 1000;
const int steadyBeepThreshold = 10;

Master master(0);

TM1637Display timerDisplay(DISPLAY1_CLK, DISPLAY1_DIO);
TM1637Display versionDisplay(DISPLAY2_CLK, DISPLAY2_DIO);

bool gameInProgress = false;
bool allSolved = false;
int remainingTime = gameDurationSeconds;
unsigned long lastTimerUpdate = 0;
unsigned long lastBeepTime = 0;
int beepInterval = 10000;

void setup() {
    Serial.begin(9600);
    Wire.begin();
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    versionDisplay.clear();

    timerDisplay.setBrightness(5);
    versionDisplay.setBrightness(5);

    Serial.println("Master: Initializing...");
    master.begin();

    Serial.println("Master: Scanning for modules...");
    master.scanForModules();

    Serial.print("Master: Found ");
    Serial.print(master.getModuleCount());
    Serial.println(" module(s)");

    displayTime(remainingTime);
}

void loop() {
    if (!gameInProgress && digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("Master: Button pressed - starting game.");

        randomSeed(millis());
        master.setVersion(random(1, 100));
        displayVersion(master.getVersion());

        master.startGame();
        gameInProgress = true;
        remainingTime = gameDurationSeconds;
        lastTimerUpdate = millis();
        lastBeepTime = millis();
        beepInterval = initialInterval;
        beep();
        delay(500);
    }

    if (gameInProgress) {
        if (millis() - lastTimerUpdate >= 1000 && remainingTime > 0) {
            remainingTime--;
            lastTimerUpdate = millis();
            displayTime(remainingTime);
        }

        if (millis() - lastBeepTime >= beepInterval) {
            beep();
            lastBeepTime = millis();

            if (remainingTime > steadyBeepThreshold) {
                float progress = (float)(remainingTime - steadyBeepThreshold) / (gameDurationSeconds - steadyBeepThreshold);
                beepInterval = finalInterval + (initialInterval - finalInterval) * progress;
            } else {
                beepInterval = finalInterval;
            }

            Serial.print("Beep interval set to: ");
            Serial.println(beepInterval);
        }

        allSolved = true;
        bool failedModule = false;
        for (uint8_t i = 0; i < master.getModuleCount(); i++) {
            uint8_t status = master.getModuleStatus(i);
            if (status == STATUS_UNSOLVED) {
                allSolved = false;
            }
            if (status == STATUS_FAILED) {
                failedModule = true;
            }
        }

        if (allSolved) {
          Serial.println("Master: All modules solved! Sending END_GAME signal.");
          master.endGame();
          digitalWrite(GREEN_LED_PIN, HIGH);
          digitalWrite(RED_LED_PIN, LOW);
          while (1);
        } 

        if (failedModule || remainingTime == 0) {
          Serial.println("Master: Game lost!");
          master.endGame();
          
          if (failedModule) {
              Serial.println("Master: A module has failed!");
          } else {
              Serial.println("Master: Time ran out!");
          }
          
          digitalWrite(RED_LED_PIN, HIGH);  // Red LED ON for failure
          tone(BUZZER_PIN, 1000, 2000);
          while (1);
        }

        delay(50);
    }
}

void displayTime(int secondsLeft) {
    int minutes = secondsLeft / 60;
    int seconds = secondsLeft % 60;
    int displayValue = (minutes * 100) + seconds;
    timerDisplay.showNumberDecEx(displayValue, 0x40, true);
}

void displayVersion(int version) {
    versionDisplay.showNumberDecEx(version, 0x40, true);
}

void beep() {
    tone(BUZZER_PIN, 1000, 100);
}
