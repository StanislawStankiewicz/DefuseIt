#include <Wire.h>
#include "ModuleComms.h"
#include <TM1637Display.h>
#include <math.h>

#define BUTTON_PIN     2
#define RED_LED_PIN    3
#define GREEN_LED_PIN  4
#define BUZZER_PIN     5

// Shift register pins for the version display
#define SDI_PIN        6
#define SCLK_PIN       7
#define LOAD_PIN       8

// TM1637 pins for the timer display
#define DISPLAY1_CLK   10
#define DISPLAY1_DIO   11

// Switch pins
#define SWITCH_PIN     12
#define SWITCH_GND     13

// Active-low digit patterns for 0–9 (dp g f e d c b a)
const byte digitCode[10] = {
  0xC0, // 0
  0xF9, // 1
  0xA4, // 2
  0xB0, // 3
  0x99, // 4
  0x92, // 5
  0x82, // 6
  0xF8, // 7
  0x80, // 8
  0x90  // 9
};

const int gameDurationSeconds = 300;
const int initialInterval     = 10000;
const int finalInterval       = 1000;
const int steadyBeepThreshold = 10;

Master master(0);
TM1637Display timerDisplay(DISPLAY1_CLK, DISPLAY1_DIO);

bool gameInProgress = false;
bool allSolved = false;
int remainingTime = gameDurationSeconds;
unsigned long lastTimerUpdate = 0;
unsigned long lastBeepTime = 0;
int beepInterval = initialInterval;

// Switch and mistake variables
bool switchOn = false;
unsigned long lastSwitchOffTime = 0;
int switchOffCount = 0;
int mistakeCount = 0;
const int maxMistakes = 3;
bool handlingFailure = false;
unsigned long failureStartTime = 0;
unsigned long lastBlinkTime = 0;
int blinkCount = 0;
bool blinkingActive = false;
int failedModuleIndex = -1;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(BUTTON_PIN,    INPUT_PULLUP);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN,   OUTPUT);
  pinMode(BUZZER_PIN,    OUTPUT);

  pinMode(SDI_PIN,  OUTPUT);
  pinMode(SCLK_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);

  // Switch pins setup
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(SWITCH_GND, OUTPUT);
  digitalWrite(SWITCH_GND, LOW);

  timerDisplay.setBrightness(5);

  Serial.println("Master: Initializing...");
  master.begin();

  Serial.println("Master: Discovering modules...");
  master.discoverModules();

  Serial.print("Master: Found ");
  Serial.print(master.getModuleCount());
  Serial.println(" module(s)");

  displayTime(remainingTime);

  // Clear the version display at startup (255 => all off for active-low)
  digitalWrite(LOAD_PIN, LOW);
  shiftOut(SDI_PIN, SCLK_PIN, MSBFIRST, 255);
  shiftOut(SDI_PIN, SCLK_PIN, MSBFIRST, 255);
  digitalWrite(LOAD_PIN, HIGH);
}

void loop() {
  // Handle switch for game start and reset
  bool currentSwitchState = (digitalRead(SWITCH_PIN) == LOW);  // LOW means switch is on (connected to ground)
  if (currentSwitchState != switchOn) {
    switchOn = currentSwitchState;
    if (!switchOn) {  // Switch turned off
      unsigned long currentTime = millis();
      if (currentTime - lastSwitchOffTime < 1000) {
        switchOffCount++;
        if (switchOffCount >= 3) {
          // Reset everything
          Serial.println("Master: Switch flipped off 3 times quickly - resetting game.");
          master.endGame();
          gameInProgress = false;
          allSolved = false;
          remainingTime = gameDurationSeconds;
          lastTimerUpdate = 0;
          lastBeepTime = 0;
          beepInterval = initialInterval;
          mistakeCount = 0;
          handlingFailure = false;
          blinkingActive = false;
          digitalWrite(GREEN_LED_PIN, LOW);
          digitalWrite(RED_LED_PIN, LOW);
          displayTime(remainingTime);
          displayVersion(0);  // FIX! this shows 0.0
          switchOffCount = 0;
        }
      } else {
        switchOffCount = 1;
      }
      lastSwitchOffTime = currentTime;
    } else {  // Switch turned on
      if (!gameInProgress) {
        Serial.println("Master: Switch flipped on - starting game.");
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
    }
  }

  // Start game if button is pressed (keeping for backward compatibility)
  if (!gameInProgress && digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Master: Button pressed - starting game.");

    randomSeed(millis());
    master.setVersion(random(1, 100));
    displayVersion(master.getVersion()); // Display version only once here

    master.startGame();
    gameInProgress = true;
    remainingTime = gameDurationSeconds;
    lastTimerUpdate = millis();
    lastBeepTime = millis();
    beepInterval = initialInterval;
    beep();
    delay(500);
  }

  // If the game is in progress, update timer, beep interval, check module statuses
  if (gameInProgress) {
    // Decrement remaining time every second
    if (millis() - lastTimerUpdate >= 1000 && remainingTime > 0) {
      remainingTime--;
      lastTimerUpdate = millis();
      displayTime(remainingTime);
    }

    // Handle beeps
    if (millis() - lastBeepTime >= beepInterval) {
      beep();
      lastBeepTime = millis();

      if (remainingTime > steadyBeepThreshold) {
        float progress = float(remainingTime - steadyBeepThreshold) 
                         / float(gameDurationSeconds - steadyBeepThreshold);
        beepInterval = finalInterval + (initialInterval - finalInterval) * progress;
      } else {
        beepInterval = finalInterval;
      }

      Serial.print("Beep interval set to: ");
      Serial.println(beepInterval);
    }

    // Handle failure state if active
    if (handlingFailure) {
      unsigned long currentTime = millis();
      if (currentTime - failureStartTime < 500) {
        // During the 0.5s beep and flash
        digitalWrite(RED_LED_PIN, HIGH);
        tone(BUZZER_PIN, 500, 50);  // Half the tone (500Hz instead of 1000Hz)
      } else {
        // After beep, start blinking
        if (!blinkingActive) {
          blinkingActive = true;
          lastBlinkTime = currentTime;
          blinkCount = 0;
        }
        // Blink red LED every 5 seconds, number of blinks = mistake count
        if (currentTime - lastBlinkTime >= 5000) {
          if (blinkCount < mistakeCount) {
            digitalWrite(RED_LED_PIN, HIGH);
            delay(200);
            digitalWrite(RED_LED_PIN, LOW);
            blinkCount++;
            lastBlinkTime = currentTime;
          } else {
            // Blinking done, restart the failed module
            Serial.print("Master: Restarting module ");
            Serial.println(failedModuleIndex);
            master.restartFailedModule(failedModuleIndex);
            handlingFailure = false;
            blinkingActive = false;
            digitalWrite(RED_LED_PIN, LOW);
          }
        }
      }
    } else {
      // Check modules
      allSolved = true;
      failedModuleIndex = -1;
      for (uint8_t i = 0; i < master.getModuleCount(); i++) {
        uint8_t status = master.getModuleStatus(i);
        if (status == STATUS_UNSOLVED) {
          allSolved = false;
        }
        if (status == STATUS_FAILED) {
          failedModuleIndex = i;
          // Assuming only one module fails at a time, break after finding the first
          break;
        }
      }

      // Win condition
      if (allSolved) {
        Serial.println("Master: All modules solved! Sending END_GAME signal.");
        master.endGame();
        digitalWrite(GREEN_LED_PIN, HIGH);
        digitalWrite(RED_LED_PIN, LOW);
        while (1); // Stop here
      }

      // Lose condition - only if time runs out or max mistakes reached
      if (remainingTime == 0 || mistakeCount >= maxMistakes) {
        Serial.println("Master: Game lost!");
        master.endGame();
        if (mistakeCount >= maxMistakes) {
          Serial.println("Master: Too many mistakes!");
        } else {
          Serial.println("Master: Time ran out!");
        }
        digitalWrite(RED_LED_PIN, HIGH);
        tone(BUZZER_PIN, 1000, 2000);
        while (1); // Stop here
      }

      // Handle module failure
      if (failedModuleIndex != -1 && !handlingFailure) {
        mistakeCount++;
        Serial.print("Master: Module failed! Mistake count: ");
        Serial.println(mistakeCount);
        handlingFailure = true;
        failureStartTime = millis();
        // Don't end game yet, just handle failure
      }
    }

    delay(50);
  }
}

void displayTime(int secondsLeft) {
  int minutes = secondsLeft / 60;
  int seconds = secondsLeft % 60;
  int displayValue = (minutes * 100) + seconds;
  // Colon bit: 0x40 => turns on the center colon
  timerDisplay.showNumberDecEx(displayValue, 0x40, true);
}

// Displays the version on the 2-digit, shift-register-based display
void displayVersion(int val) {
  if (val < 0)   val = 0;
  if (val > 99)  val = 99;

  int tens = val / 10;
  int ones = val % 10;

  // Force decimal point on the tens digit (bit 7 => 0 to turn it on)
  byte tensPattern = digitCode[tens] & 0x7F;
  byte onesPattern = digitCode[ones];

  digitalWrite(LOAD_PIN, LOW);
  shiftOut(SDI_PIN, SCLK_PIN, MSBFIRST, onesPattern);
  shiftOut(SDI_PIN, SCLK_PIN, MSBFIRST, tensPattern);
  digitalWrite(LOAD_PIN, HIGH);
}

void beep() {
  tone(BUZZER_PIN, 1000, 100);
}
