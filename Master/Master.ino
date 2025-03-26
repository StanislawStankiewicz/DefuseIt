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
#define DISPLAY1_CLK   9
#define DISPLAY1_DIO   10

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

  timerDisplay.setBrightness(5);

  Serial.println("Master: Initializing...");
  master.begin();

  Serial.println("Master: Scanning for modules...");
  master.scanForModules();

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
  // Start game if button is pressed
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

    // Check modules
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

    // Win condition
    if (allSolved) {
      Serial.println("Master: All modules solved! Sending END_GAME signal.");
      master.endGame();
      digitalWrite(GREEN_LED_PIN, HIGH);
      digitalWrite(RED_LED_PIN, LOW);
      while (1); // Stop here
    }

    // Lose condition
    if (failedModule || remainingTime == 0) {
      Serial.println("Master: Game lost!");
      master.endGame();
      if (failedModule) {
        Serial.println("Master: A module has failed!");
      } else {
        Serial.println("Master: Time ran out!");
      }
      digitalWrite(RED_LED_PIN, HIGH);
      tone(BUZZER_PIN, 1000, 2000);
      while (1); // Stop here
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
