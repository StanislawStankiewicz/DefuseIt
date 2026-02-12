#include <Wire.h>
#include "ModuleComms.h"
#include <TM1637Display.h>
#include <math.h>

#define BUTTON_PIN     2
#define RED_LED_PIN    3
#define GREEN_LED_PIN  4
#define BUZZER_PIN     5
// version display
#define SDI_PIN        6
#define SCLK_PIN       7
#define LOAD_PIN       8
// timer
#define DISPLAY_CLK   10
#define DISPLAY_DIO   11
// switch
#define SWITCH_PIN     12
#define SWITCH_GND     13

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

// Timing constants
const unsigned long SWITCH_RESET_TIMEOUT = 1000;  // 1 second
const unsigned long BEEP_DURATION = 500;          // 0.5 seconds
const unsigned long BLINK_INTERVAL = 5000;        // 5 seconds
const int BLINK_DURATION = 200;                   // 200ms
const int BLINK_GAP = 300;                        // 300ms gap between blinks

Master master(0);
TM1637Display timerDisplay(DISPLAY_CLK, DISPLAY_DIO);

const uint8_t timeSyncModules[] = {0x14};
const uint8_t timeSyncModuleCount = sizeof(timeSyncModules) / sizeof(timeSyncModules[0]);

bool isGameInProgress = false;
bool areAllModulesSolved = false;
int remainingTime = gameDurationSeconds;
unsigned long lastTimerUpdate = 0;
unsigned long lastBeepTime = 0;
int beepInterval = initialInterval;

bool isSwitchOn = false;
unsigned long lastSwitchOffTime = 0;
int switchOffCount = 0;
int mistakeCount = 0;
const int maxMistakes = 3;
bool isHandlingFailure = false;
unsigned long failureStartTime = 0;
unsigned long lastBlinkTime = 0;
int blinkCount = 0;
bool blinkingActive = false;
bool isBlinking = false;
unsigned long blinkStartTime = 0;
int currentBlink = 0;
unsigned long lastBlinkEndTime = 0;
int failedModuleIndex = -1;
bool isGameEnded = false;

void setupPins() {
  pinMode(BUTTON_PIN,    INPUT_PULLUP);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN,   OUTPUT);
  pinMode(BUZZER_PIN,    OUTPUT);

  pinMode(SDI_PIN,  OUTPUT);
  pinMode(SCLK_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(SWITCH_GND, OUTPUT);
  digitalWrite(SWITCH_GND, LOW);
}

void initializeModules() {
  delay(100);
  Serial.println("Master: Initializing...");
  master.begin();

  Serial.println("Master: Discovering modules...");
  master.discoverModules();

  Serial.print("Master: Found ");
  Serial.print(master.getModuleCount());
  Serial.println(" module(s)");
}

void clearVersionDisplay() {
  digitalWrite(LOAD_PIN, LOW);
  shiftOut(SDI_PIN, SCLK_PIN, MSBFIRST, 255);
  shiftOut(SDI_PIN, SCLK_PIN, MSBFIRST, 255);
  digitalWrite(LOAD_PIN, HIGH);
}

void initializeDisplays() {
  timerDisplay.setBrightness(5);
  displayTime(remainingTime);
  clearVersionDisplay();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  setupPins();
  initializeDisplays();
}

void displayTime(int secondsLeft) {
  int minutes = secondsLeft / 60;
  int seconds = secondsLeft % 60;
  int displayValue = (minutes * 100) + seconds;
  timerDisplay.showNumberDecEx(displayValue, 0x40, true);
}

void updateTimeSyncModules(int secondsLeft) {
  for (uint8_t i = 0; i < timeSyncModuleCount; i++) {
    master.sendRemainingSeconds(timeSyncModules[i], (uint16_t)secondsLeft);
  }
}

void displayVersion(int val) {
  if (val < 0)   val = 0;
  if (val > 99)  val = 99;

  int tens = val / 10;
  int ones = val % 10;

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

void startGame() {
  Serial.println("Master: Starting game.");
  initializeModules();
  randomSeed(millis());
  master.setVersion(random(1, 100));
  displayVersion(master.getVersion());
  master.startGame();
  isGameInProgress = true;
  remainingTime = gameDurationSeconds;
  lastTimerUpdate = millis();
  lastBeepTime = millis();
  beepInterval = initialInterval;
  updateTimeSyncModules(remainingTime);
  beep();
  delay(500);
}

void resetGame() {
  Serial.println("Master: Resetting game.");
  master.endGame();
  isGameInProgress = false;
  areAllModulesSolved = false;
  remainingTime = gameDurationSeconds;
  lastTimerUpdate = 0;
  lastBeepTime = 0;
  beepInterval = initialInterval;
  mistakeCount = 0;
  isHandlingFailure = false;
  blinkingActive = false;
  isBlinking = false;
  blinkStartTime = 0;
  currentBlink = 0;
  lastBlinkEndTime = 0;
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  displayTime(remainingTime);
  clearVersionDisplay();
  
  master.initializeModules();
}

void handleSwitch() {
  bool currentSwitchState = (digitalRead(SWITCH_PIN) == LOW);
  if (currentSwitchState != isSwitchOn) {
    isSwitchOn = currentSwitchState;
    if (!isSwitchOn) {  // Switch turned off
      Serial.println("Switch turned off");
      unsigned long currentTime = millis();
      if (currentTime - lastSwitchOffTime < SWITCH_RESET_TIMEOUT) {
        switchOffCount++;
        Serial.print("Switch off count: ");
        Serial.println(switchOffCount);
        if (switchOffCount >= 3) {
          Serial.println("Resetting game");
          resetGame();
          switchOffCount = 0;
        }
      } else {
        Serial.println("Resetting switch off count to 1");
        switchOffCount = 1;
      }
      lastSwitchOffTime = currentTime;
    } else {  // Switch turned on
      Serial.println("Switch turned on");
      if (!isGameInProgress) {
        startGame();
      }
    }
  }
}

void handleButton() {
  if (!isGameInProgress && digitalRead(BUTTON_PIN) == LOW) {
    startGame();
  }
}

void updateGameTimer() {
  if (millis() - lastTimerUpdate >= 1000 && remainingTime > 0) {
    remainingTime--;
    lastTimerUpdate = millis();
    displayTime(remainingTime);
    updateTimeSyncModules(remainingTime);
  }
}

void handleBeeps() {
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
}

void handleMistakeBlinking() {
  if (blinkingActive) {
    unsigned long currentTime = millis();
    
    if (!isBlinking) {
      if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
        if (currentBlink < mistakeCount && currentTime - lastBlinkEndTime >= BLINK_GAP) {
          digitalWrite(RED_LED_PIN, HIGH);
          blinkStartTime = currentTime;
          isBlinking = true;
          currentBlink++;
        } else if (currentBlink >= mistakeCount) {
          currentBlink = 0;
          lastBlinkTime = currentTime;
        }
      }
    } else {
      if (currentTime - blinkStartTime >= BLINK_DURATION) {
        digitalWrite(RED_LED_PIN, LOW);
        isBlinking = false;
        lastBlinkEndTime = currentTime;
      }
    }
  }
}

void handleFailure() {
  if (isHandlingFailure) {
    unsigned long currentTime = millis();
    if (currentTime - failureStartTime < BEEP_DURATION) {
      digitalWrite(RED_LED_PIN, HIGH);
      tone(BUZZER_PIN, 500, 50);  // Half the tone (500Hz instead of 1000Hz)
    } else {
      Serial.print("Master: Restarting module ");
      Serial.println(failedModuleIndex);
      master.restartFailedModule(failedModuleIndex);
      master.sendCommand(master.getModuleAddress(failedModuleIndex), CMD_START_GAME);
      blinkingActive = true;
      lastBlinkTime = currentTime;
      blinkCount = 0;
      isBlinking = false;
      blinkStartTime = 0;
      currentBlink = 0;
      lastBlinkEndTime = 0;
      isHandlingFailure = false;
      digitalWrite(RED_LED_PIN, LOW);
    }
  }
}

void checkModules() {
  areAllModulesSolved = true;
  failedModuleIndex = -1;
  for (uint8_t i = 0; i < master.getModuleCount(); i++) {
    uint8_t status = master.getModuleStatus(i);
    if (status != STATUS_PASSED) {
      areAllModulesSolved = false;
    }
    if (status == STATUS_FAILED) {
      failedModuleIndex = i;
      break;
    }
  }
}

void checkWinLose() {
  if (areAllModulesSolved) {
    Serial.println("Master: All modules solved! Sending END_GAME signal.");
    master.endGame();
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    isGameInProgress = false;
    isGameEnded = true;
  }

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
    isGameInProgress = false;
    isGameEnded = true;
  }

  if (failedModuleIndex != -1 && !isHandlingFailure) {
    mistakeCount++;
    if (mistakeCount >= maxMistakes) {
      Serial.println("Master: Game lost - too many mistakes!");
      master.endGame();
      digitalWrite(RED_LED_PIN, HIGH);
      tone(BUZZER_PIN, 1000, 2000);
      isGameInProgress = false;
      isGameEnded = true;
    } else {
      Serial.print("Master: Module failed! Mistake count: ");
      Serial.println(mistakeCount);
      isHandlingFailure = true;
      failureStartTime = millis();
    }
  }
}

void loop() {
  handleSwitch();
  handleButton();

  if (isGameEnded && !isSwitchOn) {
    resetGame();
    isGameEnded = false;
  }

  if (isGameInProgress) {
    updateGameTimer();
    handleBeeps();
    handleMistakeBlinking();

    if (isHandlingFailure) {
      handleFailure();
    } else {
      checkModules();
      checkWinLose();
    }

    delay(50);
  }
}

