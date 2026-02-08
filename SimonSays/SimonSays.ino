#include <Arduino.h>
#include "ModuleComms.h"

// --- Constants ---
const int LED_PINS[4] = {12, 11, 10, 9};
const int BUTTON_PINS[4] = {5, 6, 7, 8};
const int SPEAKER_PIN = 3;
const int NOTES[4] = {262, 330, 392, 494};

const int MIN_SEQUENCE = 3;
const int MAX_SEQUENCE = 5;
const int SEQUENCE_ARRAY_SIZE = 5;

const unsigned long FLASH_DURATION = 500;
const unsigned long INTER_STEP_PAUSE = 200; // Small gap between notes
const unsigned long WAIT_AFTER_CORRECT = 2000; // Time before showing next sequence
const unsigned long INPUT_TIMEOUT = 4000;      // Auto-repeat if idle

// --- Debug ---
const bool DEBUG = true;

// --- Game Variables ---
enum GameState { UNENGAGED, SHOWING, WAITING_INPUT, ROUND_CLEAR };
GameState currentState = UNENGAGED;

int sequence[SEQUENCE_ARRAY_SIZE];
int sequenceLength = 0; // Total length for the module to be "solved"
int currentLength = 0;  // How many steps of the sequence are currently active
int inputIndex = 0;     // How many buttons the user has pressed in the current round
int showIndex = 0;

unsigned long stateTimer = 0;
bool ledActive = false;
int lastPressedButton = -1;

void gameLoop();
void resetState();
Slave slave(0x11, gameLoop, resetState, 2);

// --- Helpers ---

const __FlashStringHelper* stateName(GameState state) {
    switch (state) {
        case UNENGAGED: return F("UNENGAGED");
        case SHOWING: return F("SHOWING");
        case WAITING_INPUT: return F("WAITING_INPUT");
        case ROUND_CLEAR: return F("ROUND_CLEAR");
        default: return F("UNKNOWN");
    }
}

void debugStateTransition(GameState fromState, GameState toState) {
    if (!DEBUG) return;
    Serial.print(F("[STATE] "));
    Serial.print(stateName(fromState));
    Serial.print(F(" -> "));
    Serial.println(stateName(toState));
}

void debugButtonEvent(const __FlashStringHelper* action, int buttonIndex) {
    if (!DEBUG) return;
    Serial.print(F("[BTN] "));
    Serial.print(action);
    Serial.print(F(" index="));
    Serial.print(buttonIndex);
    Serial.print(F(" expected="));
    if (inputIndex >= 0 && inputIndex < sequenceLength) {
        Serial.print(sequence[inputIndex]);
    } else {
        Serial.print(F("-"));
    }
    Serial.print(F(" inputIndex="));
    Serial.print(inputIndex);
    Serial.print(F(" currentLength="));
    Serial.print(currentLength);
    Serial.print(F(" state="));
    Serial.println(stateName(currentState));
}

void debugSequence() {
    if (!DEBUG) return;
    Serial.print(F("[SEQ] length="));
    Serial.print(sequenceLength);
    Serial.print(F(" values="));
    for (int i = 0; i < sequenceLength; i++) {
        Serial.print(sequence[i]);
        if (i < sequenceLength - 1) Serial.print(F(","));
    }
    Serial.println();
}

void setFeedback(int index, bool active, bool enableSound) {
    if (index < 0 || index > 3) return;
    digitalWrite(LED_PINS[index], active ? HIGH : LOW);
    if (active && enableSound) {
        tone(SPEAKER_PIN, NOTES[index]);
    } else {
        noTone(SPEAKER_PIN);
    }
    ledActive = active;
}

int getPressedButton() {
    for (int i = 0; i < 4; i++) {
        if (digitalRead(BUTTON_PINS[i]) == LOW) return i;
    }
    return -1;
}

// --- State Handlers ---

void handleUnengaged() {
    unsigned long cycle = millis() % 4000;
    if (cycle < FLASH_DURATION) {
        digitalWrite(LED_PINS[sequence[0]], HIGH);
    } else {
        digitalWrite(LED_PINS[sequence[0]], LOW);
    }

    int btn = getPressedButton();

    // Handle Button Press: Start the buzz/LED immediately
    if (lastPressedButton == -1 && btn != -1) {
        debugButtonEvent(F("PRESS"), btn);
        lastPressedButton = btn;
        setFeedback(btn, true, true); 
    } 
    // Handle Button Release: Stop the buzz and change state
    else if (lastPressedButton != -1 && btn == -1) {
        debugButtonEvent(F("RELEASE"), lastPressedButton);
        setFeedback(lastPressedButton, false, true);
        
        if (lastPressedButton == sequence[0]) {
            currentLength = 2; // Start game by showing 2 steps
            stateTimer = millis();
            debugStateTransition(currentState, ROUND_CLEAR);
            currentState = ROUND_CLEAR;
        } else {
            if (DEBUG) Serial.println(F("[FAIL] Wrong start button"));
            slave.fail();
        }
        lastPressedButton = -1;
    }
}

void handleShowing() {
    unsigned long elapsed = millis() - stateTimer;

    if (!ledActive) {
        if (elapsed >= INTER_STEP_PAUSE) {
            if (showIndex < currentLength) {
                if (DEBUG) {
                    Serial.print(F("[SHOW] step="));
                    Serial.print(showIndex);
                    Serial.print(F(" value="));
                    Serial.println(sequence[showIndex]);
                }
                setFeedback(sequence[showIndex], true, true);
                stateTimer = millis();
                showIndex++;
            } else {
                debugStateTransition(currentState, WAITING_INPUT);
                currentState = WAITING_INPUT;
                inputIndex = 0;
                stateTimer = millis();
            }
        }
    } else {
        if (elapsed >= FLASH_DURATION) {
            setFeedback(sequence[showIndex - 1], false, true);
            stateTimer = millis();
        }
    }
}

void handleWaitingInput() {
    int btn = getPressedButton();

    // Handle Button Press
    if (lastPressedButton == -1 && btn != -1) {
        debugButtonEvent(F("PRESS"), btn);
        lastPressedButton = btn;
        setFeedback(btn, true, true);
    } 
    // Handle Button Release
    else if (lastPressedButton != -1 && btn == -1) {
        debugButtonEvent(F("RELEASE"), lastPressedButton);
        setFeedback(lastPressedButton, false, true);
        
        if (lastPressedButton == sequence[inputIndex]) {
            if (DEBUG) Serial.println(F("[OK] Correct button"));
            inputIndex++;
            stateTimer = millis(); 
            
            if (inputIndex == currentLength) {
                if (currentLength >= sequenceLength) {
                    if (DEBUG) Serial.println(F("[PASS] Module solved"));
                    slave.pass();
                } else {
                    currentLength++;
                    debugStateTransition(currentState, ROUND_CLEAR);
                    currentState = ROUND_CLEAR;
                    stateTimer = millis();
                }
            }
        } else {
            if (DEBUG) Serial.println(F("[FAIL] Wrong button in sequence"));
            slave.fail();
            resetState();
        }
        lastPressedButton = -1;
    }

    // Timeout: Repeat sequence if user is idle
    if (btn == -1 && millis() - stateTimer > INPUT_TIMEOUT) {
        if (DEBUG) Serial.println(F("[TIMEOUT] Replaying sequence"));
        debugStateTransition(currentState, SHOWING);
        currentState = SHOWING;
        showIndex = 0;
        stateTimer = millis();
    }
}

void gameLoop() {
    switch (currentState) {
        case UNENGAGED:    handleUnengaged();    break;
        case SHOWING:      handleShowing();      break;
        case WAITING_INPUT: handleWaitingInput(); break;
        case ROUND_CLEAR: 
            if (millis() - stateTimer >= WAIT_AFTER_CORRECT) {
                showIndex = 0;
                inputIndex = 0; // Reset user progress for the new round
                debugStateTransition(currentState, SHOWING);
                currentState = SHOWING;
                stateTimer = millis();
            }
            break;
    }
}

void resetState() {
    randomSeed(millis() + slave.getVersion());
    sequenceLength = random(MIN_SEQUENCE, MAX_SEQUENCE + 1);
    for (int i = 0; i < sequenceLength; i++) {
        sequence[i] = random(0, 4);
    }
    debugSequence();
    
    debugStateTransition(currentState, UNENGAGED);
    currentState = UNENGAGED;
    currentLength = 0;
    inputIndex = 0;
    showIndex = 0;
    ledActive = false;
    lastPressedButton = -1;
}

void setup() {
    Serial.begin(9600);
    Serial.println("SimonSays Initialized");
    if (DEBUG) Serial.println(F("[DEBUG] Enabled"));
    for (int i = 0; i < 4; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    }
    pinMode(SPEAKER_PIN, OUTPUT);
    slave.begin();
}

void loop() {
    slave.slaveLoop();
}