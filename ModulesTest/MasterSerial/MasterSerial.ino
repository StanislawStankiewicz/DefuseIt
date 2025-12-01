#include <Wire.h>
#include "ModuleComms.h"
#include <math.h>

// No physical pins needed - using Serial console for all interaction

const int gameDurationSeconds = 300;
const int initialInterval     = 10000;
const int finalInterval       = 1000;
const int steadyBeepThreshold = 10;

// Timing constants
const unsigned long BEEP_DURATION = 500;          // 0.5 seconds
const unsigned long BLINK_INTERVAL = 5000;        // 5 seconds
const int BLINK_DURATION = 200;                   // 200ms
const int BLINK_GAP = 300;                        // 300ms gap between blinks

Master master(0);

bool isGameInProgress = false;
bool areAllModulesSolved = false;
int remainingTime = gameDurationSeconds;
unsigned long lastTimerUpdate = 0;
unsigned long lastBeepTime = 0;
int beepInterval = initialInterval;

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

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println("=====================================");
  Serial.println("      DEFUSEIT MASTER CONTROLLER");
  Serial.println("=====================================");
  Serial.println("Commands:");
  Serial.println("- Type 'start' to begin the game");
  Serial.println("- Type 'reset' to reset the game");
  Serial.println("=====================================");
}

void displayTime(int secondsLeft) {
  int minutes = secondsLeft / 60;
  int seconds = secondsLeft % 60;
  Serial.print("TIMER: ");
  Serial.print(minutes);
  Serial.print(":");
  if (seconds < 10) Serial.print("0");
  Serial.print(seconds);
  Serial.print(" (");
  Serial.print(secondsLeft);
  Serial.println("s remaining)");
}

void displayVersion(int val) {
  Serial.print("VERSION: ");
  Serial.println(val);
}

void beep() {
  Serial.println("BEEP: Short beep played");
}

void startGame() {
  Serial.println("=====================================");
  Serial.println("Master: STARTING GAME!");
  Serial.println("=====================================");

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

  beep();
  delay(500);

  Serial.println("Master: Game started successfully!");
  Serial.println("=====================================");
}

void resetGame() {
  Serial.println("=====================================");
  Serial.println("Master: RESETTING GAME");
  Serial.println("=====================================");

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

  Serial.println("LED: Green OFF, Red OFF");

  displayTime(remainingTime);

  Serial.println("Master: Rescanning modules...");
  master.discoverModules();
  Serial.print("Master: Found ");
  Serial.print(master.getModuleCount());
  Serial.println(" module(s)");

  Serial.println("Master: Game reset complete");
  Serial.println("=====================================");
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equalsIgnoreCase("start")) {
      if (!isGameInProgress) {
        Serial.println("COMMAND: 'start' received - starting game");
        startGame();
      } else {
        Serial.println("COMMAND: Game already in progress");
      }
    } else if (command.equalsIgnoreCase("reset")) {
      Serial.println("COMMAND: 'reset' received - resetting game");
      resetGame();
    } else if (command.length() > 0) {
      Serial.print("COMMAND: Unknown command '");
      Serial.print(command);
      Serial.println("'. Available: 'start', 'reset'");
    }
  }
}

void updateGameTimer() {
  // Decrement remaining time every second
  if (millis() - lastTimerUpdate >= 1000 && remainingTime > 0) {
    remainingTime--;
    lastTimerUpdate = millis();
    displayTime(remainingTime);

    // Show time warnings
    if (remainingTime == 60) {
      Serial.println("WARNING: 1 minute remaining!");
    } else if (remainingTime == 30) {
      Serial.println("WARNING: 30 seconds remaining!");
    } else if (remainingTime == 10) {
      Serial.println("WARNING: 10 seconds remaining!");
    }
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

    Serial.print("BEEP: Interval set to ");
    Serial.print(beepInterval);
    Serial.println("ms");
  }
}

void handleMistakeBlinking() {
  // Handle passive mistake blinking reminder
  if (blinkingActive) {
    unsigned long currentTime = millis();

    if (!isBlinking) {
      // Not currently blinking, check if time for next blink
      if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
        if (currentBlink < mistakeCount && currentTime - lastBlinkEndTime >= BLINK_GAP) {
          // Start next blink in sequence
          Serial.println("LED: Red ON (blink)");
          blinkStartTime = currentTime;
          isBlinking = true;
          currentBlink++;
          Serial.print("BLINK: Starting blink ");
          Serial.print(currentBlink);
          Serial.print("/");
          Serial.println(mistakeCount);
        } else if (currentBlink >= mistakeCount) {
          // Sequence complete, reset for next cycle
          currentBlink = 0;
          lastBlinkTime = currentTime;
          Serial.println("BLINK: Sequence complete, waiting for next cycle");
        }
      }
    } else {
      // Currently blinking, check if time to turn off
      if (currentTime - blinkStartTime >= BLINK_DURATION) {
        Serial.println("LED: Red OFF (blink end)");
        isBlinking = false;
        lastBlinkEndTime = currentTime;
        Serial.println("BLINK: Blink ended");
      }
    }
  }
}

void handleFailure() {
  // Handle failure state if active
  if (isHandlingFailure) {
    unsigned long currentTime = millis();
    if (currentTime - failureStartTime < BEEP_DURATION) {
      // During the 0.5s beep and flash
      Serial.println("LED: Red ON, BUZZER: Penalty tone active");
      Serial.println("FAILURE: Penalty beep and flash active");
    } else {
      // Penalty done, start passive blinking reminder
      Serial.print("FAILURE: Restarting module ");
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
      Serial.println("LED: Red OFF");

      Serial.println("FAILURE: Module restarted, blinking reminder activated");
    }
  }
}

void checkModules() {
  // Check modules
  areAllModulesSolved = true;
  failedModuleIndex = -1;

  Serial.println("MODULES: Checking all module statuses...");

  for (uint8_t i = 0; i < master.getModuleCount(); i++) {
    uint8_t status = master.getModuleStatus(i);
    Serial.print("MODULE ");
    Serial.print(i);
    Serial.print(": ");

    switch(status) {
      case 1: Serial.println("UNSOLVED"); break;
      case 2: Serial.println("PASSED"); break;
      case 3: Serial.println("FAILED"); break;
      case 0xFF: Serial.println("UNKNOWN COMMAND"); break;
      default: Serial.print("UNKNOWN: ");
      Serial.println(status);
      break;
    }

    if (status != 2) {  // STATUS_PASSED
      areAllModulesSolved = false;
    }
    if (status == 3) {  // STATUS_FAILED
      failedModuleIndex = i;
      break;
    }
  }

  Serial.print("MODULES: Overall status - ");
  Serial.println(areAllModulesSolved ? "ALL SOLVED!" : "Some unsolved");
}

void checkWinLose() {
  // Win condition
  if (areAllModulesSolved) {
    Serial.println("=====================================");
    Serial.println("🎉 WIN CONDITION: All modules solved!");
    Serial.println("=====================================");

    master.endGame();
    Serial.println("LED: Green ON, Red OFF");
    isGameInProgress = false;
    isGameEnded = true;

    Serial.println("GAME: Ended with VICTORY!");
    Serial.println("=====================================");
  }

  // Lose condition
  if (remainingTime == 0 || mistakeCount >= maxMistakes) {
    Serial.println("=====================================");
    Serial.println("💥 LOSE CONDITION DETECTED!");
    Serial.println("=====================================");

    master.endGame();

    if (mistakeCount >= maxMistakes) {
      Serial.println("REASON: Too many mistakes!");
      Serial.print("MISTAKES: ");
      Serial.print(mistakeCount);
      Serial.print("/");
      Serial.println(maxMistakes);
    } else {
      Serial.println("REASON: Time ran out!");
      Serial.print("TIME: ");
      Serial.println(remainingTime);
    }

    Serial.println("LED: Red ON, BUZZER: Long defeat tone");
    isGameInProgress = false;
    isGameEnded = true;

    Serial.println("GAME: Ended with DEFEAT!");
    Serial.println("=====================================");
  }

  // Handle module failure
  if (failedModuleIndex != -1 && !isHandlingFailure) {
    mistakeCount++;

    Serial.println("=====================================");
    Serial.print("MODULE FAILURE: Module ");
    Serial.print(failedModuleIndex);
    Serial.println(" failed!");
    Serial.print("MISTAKE COUNT: ");
    Serial.print(mistakeCount);
    Serial.print("/");
    Serial.println(maxMistakes);

    if (mistakeCount >= maxMistakes) {
      // Final mistake - end game immediately without penalty
      Serial.println("FINAL MISTAKE: Game lost!");
      master.endGame();
      Serial.println("LED: Red ON, BUZZER: Long defeat tone");
      isGameInProgress = false;
      isGameEnded = true;
      Serial.println("GAME: Ended with DEFEAT (final mistake)!");
    } else {
      // Normal failure - apply penalty
      Serial.println("PENALTY: Applying failure penalty...");
      isHandlingFailure = true;
      failureStartTime = millis();
    }
    Serial.println("=====================================");
  }
}

void loop() {
  handleSerialCommands();

  // Check if game has ended, then allow restart
  if (isGameEnded) {
    Serial.println("GAME: Ended - type 'reset' to play again");
    isGameEnded = false;
  }

  // If the game is in progress, update timer, beep interval, check module statuses
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
  } else {
    // Game not in progress - show status occasionally
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
      Serial.println("STATUS: Waiting for 'start' command...");
      lastStatus = millis();
    }
    delay(100);
  }
}