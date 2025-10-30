#include <Arduino.h>
#include "ModuleComms.h"

const int LED_PINS[4] = {5, 6, 7, 8};  // Red, Green, Yellow, Blue
const int BUTTON_PINS[4] = {9, 10, 11, 12};  // Corresponding buttons
const int SPEAKER_PIN = 3;

const int NOTES[4] = {262, 330, 392, 494};  // C, E, G, B frequencies

const int MIN_SEQUENCE = 3;
const int MAX_SEQUENCE = 7;  // random(3,7) gives 3 to 6

int sequence[6];
int sequenceLength;
int currentLength = 0;
int inputIndex = 0;
bool userStartedGuessing = false;

enum State { UNENGAGED, SHOWING, WAITING_INPUT, SUCCESS, FAILED };
State state = UNENGAGED;

unsigned long lastFlash = 0;
const unsigned long FLASH_INTERVAL = 4000;
const unsigned long FLASH_DURATION = 500;
unsigned long lastAction = 0;
unsigned long lastCompletion = 0;
bool isWaiting = false;
volatile bool gameRunning = false;

#define SLAVE_ADDRESS 0x12
Slave simonSlave(SLAVE_ADDRESS);

void startGameCallback() {
  randomSeed(simonSlave.getVersion());
  gameRunning = true;
  state = UNENGAGED;
  currentLength = 0;
  inputIndex = 0;
  userStartedGuessing = false;
  lastCompletion = 0;
  isWaiting = false;
  simonSlave.setStatus(STATUS_UNSOLVED);
  sequenceLength = random(MIN_SEQUENCE, MAX_SEQUENCE);
  for (int i = 0; i < sequenceLength; i++) {
    sequence[i] = random(0, 4);
  }
}

void endGameCallback() {
  gameRunning = false;
  noTone(SPEAKER_PIN);
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}

bool buttonPressed = false;
int pressedButton = -1;
unsigned long pressStart = 0;

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  pinMode(SPEAKER_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Simon Says: Initializing module.");

  simonSlave.begin();
  simonSlave.onGameStart(startGameCallback);
  simonSlave.onGameEnd(endGameCallback);

  Serial.println("Simon Says: Module initialized.");
}

void loop() {
  if (!gameRunning) {
    Serial.println("Simon Says: Waiting for game to start...");
    return;
  };
  Serial.println("Simon Says: Game running.");

  switch (state) {
    case UNENGAGED: {
      // Check if ready to start after completion
      if (lastCompletion > 0 && millis() - lastCompletion >= 4000) {
        Serial.println("Starting sequence after 4 second wait.");
        state = SHOWING;
        currentLength = 2;  // Show first two parts
        inputIndex = 0;
        userStartedGuessing = false;
        lastCompletion = 0;
        isWaiting = false;
        lastAction = millis();
      }

      // Flash the first LED of the sequence every 4 seconds for 0.5s without beeping (only if no button pressed and not waiting)
      if (!buttonPressed && !isWaiting) {
        if (millis() - lastFlash >= FLASH_INTERVAL) {
          digitalWrite(LED_PINS[sequence[0]], HIGH);
          lastFlash = millis();
        }
        if (millis() - lastFlash >= FLASH_DURATION && digitalRead(LED_PINS[sequence[0]]) == HIGH) {
          digitalWrite(LED_PINS[sequence[0]], LOW);
        }
      }

      // Handle button press like in WAITING_INPUT
      if (!buttonPressed) {
        // Check for button press
        for (int i = 0; i < 4; i++) {
          if (digitalRead(BUTTON_PINS[i]) == LOW) {
            buttonPressed = true;
            pressedButton = i;
            pressStart = millis();
            // Turn on LED and buzzer
            digitalWrite(LED_PINS[i], HIGH);
            tone(SPEAKER_PIN, NOTES[i]);
            Serial.print("Button pressed: ");
            Serial.println(i);
            break;
          }
        }
      } else {
        // Check if button released
        if (digitalRead(BUTTON_PINS[pressedButton]) == HIGH) {
          // Button released: turn off LED and buzzer
          digitalWrite(LED_PINS[pressedButton], LOW);
          noTone(SPEAKER_PIN);
          buttonPressed = false;

          // Check if correct (only first button)
          if (pressedButton == sequence[0]) {
            Serial.println("Correct first button released! Waiting 4 seconds.");
            // Start 4 second wait
            lastCompletion = millis();
            isWaiting = true;
          } else {
            // Incorrect: module failed
            Serial.println("Incorrect button! Module failed.");
            state = FAILED;
            simonSlave.setStatus(STATUS_FAILED);
            // Flash the wrong LED
            digitalWrite(LED_PINS[pressedButton], HIGH);
            delay(1000);
            digitalWrite(LED_PINS[pressedButton], LOW);
          }
          pressedButton = -1;
        }
      }
      break;
    }

    case SHOWING: {
      // Show the sequence from start up to current length
      Serial.print("Showing sequence up to length: ");
      Serial.println(currentLength);
      for (int i = 0; i < currentLength; i++) {
        int led = sequence[i];
        Serial.print("Showing part: ");
        Serial.println(led);
        digitalWrite(LED_PINS[led], HIGH);
        tone(SPEAKER_PIN, NOTES[led], 500);
        delay(500);
        digitalWrite(LED_PINS[led], LOW);
        delay(500);
      }
      state = WAITING_INPUT;
      inputIndex = 0;
      userStartedGuessing = false;
      lastCompletion = 0;
      lastAction = millis();
      break;
    }

    case WAITING_INPUT: {
      if (!buttonPressed) {
        // Check for button press
        for (int i = 0; i < 4; i++) {
          if (digitalRead(BUTTON_PINS[i]) == LOW) {
            buttonPressed = true;
            pressedButton = i;
            pressStart = millis();
            // Turn on LED and buzzer
            digitalWrite(LED_PINS[i], HIGH);
            tone(SPEAKER_PIN, NOTES[i]);
            Serial.print("Button pressed: ");
            Serial.println(i);
            userStartedGuessing = true;
            break;
          }
        }
      } else {
        // Check if button released
        if (digitalRead(BUTTON_PINS[pressedButton]) == HIGH) {
          // Button released: turn off LED and buzzer
          digitalWrite(LED_PINS[pressedButton], LOW);
          noTone(SPEAKER_PIN);
          buttonPressed = false;

          // Check if correct
          if (pressedButton == sequence[inputIndex]) {
            Serial.print("Correct press: ");
            Serial.println(pressedButton);
            inputIndex++;
            if (inputIndex == currentLength) {
              // Completed current length
              if (currentLength == sequenceLength) {
              // Sequence completed successfully
              state = SUCCESS;
              Serial.println("Sequence completed successfully!");
              simonSlave.setStatus(STATUS_PASSED);
              // Flash all LEDs
                for (int j = 0; j < 4; j++) {
                  digitalWrite(LED_PINS[j], HIGH);
                }
                delay(1000);
                for (int j = 0; j < 4; j++) {
                  digitalWrite(LED_PINS[j], LOW);
                }
              } else {
                // Wait 4 seconds before showing next part
                lastCompletion = millis();
                Serial.println("Completed level, waiting 4 seconds before next.");
              }
            }
          } else {
            // Incorrect: module failed
            state = FAILED;
            Serial.println("Incorrect! Module failed.");
            simonSlave.setStatus(STATUS_FAILED);
            // Flash red LED
            digitalWrite(LED_PINS[0], HIGH);
            delay(1000);
            digitalWrite(LED_PINS[0], LOW);
          }
          pressedButton = -1;
        }
      }

      // Check if ready to show next part after 4 second delay
      if (inputIndex == currentLength && currentLength < sequenceLength && millis() - lastCompletion >= 4000) {
        currentLength++;
        Serial.println("Advancing to next length.");
        state = SHOWING;
      }

      // If no input for 4 seconds and user hasn't started guessing, repeat the current sequence
      if (!userStartedGuessing && millis() - lastAction >= 4000) {
        Serial.println("Repeating current sequence.");
        state = SHOWING;
      }
      break;
    }

    case SUCCESS: {
      // Module solved - reset to unengaged
      noTone(SPEAKER_PIN);
      state = UNENGAGED;
      currentLength = 0;
      inputIndex = 0;
      userStartedGuessing = false;
      lastCompletion = 0;
      isWaiting = false;
      lastAction = millis();
      break;
    }

    case FAILED: {
      // Module failed - reset to unengaged
      noTone(SPEAKER_PIN);
      state = UNENGAGED;
      currentLength = 0;
      inputIndex = 0;
      userStartedGuessing = false;
      lastCompletion = 0;
      isWaiting = false;
      lastAction = millis();
      break;
    }
  }
}
