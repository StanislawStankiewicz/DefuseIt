#include <Arduino.h>
#include "ModuleComms.h"

const int LED_PINS[4] = {5, 6, 7, 8};
const int BUTTON_PINS[4] = {9, 10, 11, 12};
const int SPEAKER_PIN = 3;

const int NOTES[4] = {262, 330, 392, 494};

const int MIN_SEQUENCE = 3;
const int MAX_SEQUENCE = 7;
const int SEQUENCE_ARRAY_SIZE = 6;

const unsigned long FLASH_INTERVAL = 4000;
const unsigned long FLASH_DURATION = 500;
const unsigned long WAIT_AFTER_COMPLETION = 4000;
const unsigned long REPEAT_TIMEOUT = 4000;

int sequence[SEQUENCE_ARRAY_SIZE];
int sequenceLength = 0;
int currentLength = 0;
int inputIndex = 0;
bool userStartedGuessing = false;

enum GameState { UNENGAGED, SHOWING, WAITING_INPUT };
GameState currentState = UNENGAGED;

unsigned long lastFlashTime = 0;
unsigned long lastActionTime = 0;
unsigned long lastCompletionTime = 0;

const uint8_t SLAVE_ADDRESS = 0x11;

void gameLoop();
void setInitialState();

Slave slave(SLAVE_ADDRESS, gameLoop, setInitialState, 2);
bool isWaiting = false;
unsigned long showTimer = 0;
int showIndex = 0;
bool showingLed = false;

bool buttonPressed = false;
int pressedButton = -1;
unsigned long pressStartTime = 0;

void gameLoop() {
  Serial.print("Sequemce:");
  for (int i = 0; i < sequenceLength; i++) {
    Serial.print(" ");
    Serial.print(sequence[i]);
  }
  Serial.println();
  switch (currentState) {
    case UNENGAGED:
      handleUnengagedState();
      break;
    case SHOWING:
      handleShowingState();
      break;
    case WAITING_INPUT:
      handleWaitingInputState();
      break;
  }
}

void handleUnengagedState() {
  if (lastCompletionTime > 0 && millis() - lastCompletionTime >= WAIT_AFTER_COMPLETION) {
    currentState = SHOWING;
    currentLength = 2;  // Show first two parts
    inputIndex = 0;
    userStartedGuessing = false;
    lastCompletionTime = 0;
    isWaiting = false;
    lastActionTime = millis();
  }

  if (!buttonPressed && !isWaiting) {
    if (millis() - lastFlashTime >= FLASH_INTERVAL) {
      digitalWrite(LED_PINS[sequence[0]], HIGH);
      lastFlashTime = millis();
    }
    if (millis() - lastFlashTime >= FLASH_DURATION && digitalRead(LED_PINS[sequence[0]]) == HIGH) {
      digitalWrite(LED_PINS[sequence[0]], LOW);
    }
  }

  handleButtonPressInUnengaged();
}

void handleButtonPressInUnengaged() {
  if (!buttonPressed) {
    for (int i = 0; i < 4; i++) {
      if (digitalRead(BUTTON_PINS[i]) == LOW) {
        buttonPressed = true;
        pressedButton = i;
        pressStartTime = millis();
        digitalWrite(LED_PINS[i], HIGH);
        tone(SPEAKER_PIN, NOTES[i]);
        break;
      }
    }
  } else {
    if (digitalRead(BUTTON_PINS[pressedButton]) == HIGH) {
      digitalWrite(LED_PINS[pressedButton], LOW);
      noTone(SPEAKER_PIN);
      buttonPressed = false;

      if (pressedButton == sequence[0]) {
        lastCompletionTime = millis();
        isWaiting = true;
      } else {
        Serial.print("handleUnengagedState: Wrong initial press ");
        Serial.print(pressedButton);
        Serial.print(" expected ");
        Serial.println(sequence[0]);
        slave.fail();
        currentState = UNENGAGED;
      }
      pressedButton = -1;
    }
  }
}

void handleShowingState() {
  if (!showingLed) {
    if (showIndex < currentLength) {
      digitalWrite(LED_PINS[sequence[showIndex]], HIGH);
      tone(SPEAKER_PIN, NOTES[sequence[showIndex]]);
      showingLed = true;
      showTimer = millis();
      showIndex++;
    } else {
      currentState = WAITING_INPUT;
      inputIndex = 0;
      userStartedGuessing = false;
      lastCompletionTime = 0;
      lastActionTime = millis();
      showIndex = 0;
      showingLed = false;
    }
  } else {
    if (millis() - showTimer >= 500) {
      digitalWrite(LED_PINS[sequence[showIndex - 1]], LOW);
      noTone(SPEAKER_PIN);
      showingLed = false;
    }
  }
}

void handleWaitingInputState() {
  if (!buttonPressed) {
    // Check for button press
    for (int i = 0; i < 4; i++) {
      if (digitalRead(BUTTON_PINS[i]) == LOW) {
        Serial.print("handleWaitingInputState: Button pressed: ");
        Serial.println(i);
        buttonPressed = true;
        pressedButton = i;
        pressStartTime = millis();
        // Turn on LED and buzzer
        digitalWrite(LED_PINS[i], HIGH);
        tone(SPEAKER_PIN, NOTES[i]);
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
        Serial.print("handleWaitingInputState: Correct input ");
        Serial.print(pressedButton);
        Serial.print(" at index ");
        Serial.println(inputIndex);
        inputIndex++;
        if (inputIndex == currentLength) {
          // Completed current length
          if (currentLength == sequenceLength) {
            // Sequence completed successfully
            Serial.println("handleWaitingInputState: Full sequence completed - PASS");
            slave.pass();
          } else {
            // Wait 4 seconds before showing next part
            lastCompletionTime = millis();
          }
        }
      } else {
        // Incorrect: module failed
        Serial.print("handleWaitingInputState: Wrong input ");
        Serial.print(pressedButton);
        Serial.print(" expected ");
        Serial.println(sequence[inputIndex]);
        slave.fail();
      }
      pressedButton = -1;
    }
  }
  if (inputIndex == currentLength && currentLength < sequenceLength && millis() - lastCompletionTime >= WAIT_AFTER_COMPLETION) {
    Serial.print("handleWaitingInputState: Advancing to length ");
    Serial.println(currentLength + 1);
    currentLength++;
    currentState = SHOWING;
  }
  if (!userStartedGuessing && millis() - lastActionTime >= REPEAT_TIMEOUT) {
    Serial.println("handleWaitingInputState: Timeout - repeating sequence");
    currentState = SHOWING;
  }
}

void setInitialState() {
  randomSeed(millis() + slave.getVersion());
  currentState = UNENGAGED;
  currentLength = 0;
  inputIndex = 0;
  userStartedGuessing = false;
  lastCompletionTime = 0;
  isWaiting = false;
  showTimer = 0;
  showIndex = 0;
  showingLed = false;
  sequenceLength = random(MIN_SEQUENCE, MAX_SEQUENCE + 1);
  for (int i = 0; i < sequenceLength; i++) {
    sequence[i] = random(0, 4);
  }
}

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
  for (int i = 0; i < 4; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  pinMode(SPEAKER_PIN, OUTPUT);
  Serial.begin(9600);
    Serial1.begin(9600);
  slave.begin();
}

void loop() {
  while (Serial1.available()) {
        char c = Serial1.read();
        Serial.write(c);
    }
  slave.slaveLoop();
}