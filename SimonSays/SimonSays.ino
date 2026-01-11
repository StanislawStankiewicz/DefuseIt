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
    // Flash first LED every 4 seconds without sound
    unsigned long cycle = millis() % 4000;
    if (cycle < FLASH_DURATION) {
        digitalWrite(LED_PINS[sequence[0]], HIGH);
    } else {
        digitalWrite(LED_PINS[sequence[0]], LOW);
    }

    int btn = getPressedButton();
    if (btn != -1) {
        setFeedback(btn, true, true);
        if (btn == sequence[0]) {
            currentLength = 2; // Per user story: start by showing 2 parts
            stateTimer = millis();
            currentState = ROUND_CLEAR; 
        } else {
            slave.fail();
        }
    }
}

void handleShowing() {
    unsigned long elapsed = millis() - stateTimer;

    if (!ledActive) {
        if (elapsed >= INTER_STEP_PAUSE) {
            if (showIndex < currentLength) {
                setFeedback(sequence[showIndex], true, true);
                stateTimer = millis();
                showIndex++;
            } else {
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

    // Handle Button Release
    if (lastPressedButton != -1 && btn == -1) {
        setFeedback(lastPressedButton, false, true);
        
        if (lastPressedButton == sequence[inputIndex]) {
            inputIndex++;
            stateTimer = millis(); // Reset timeout on success
            
            if (inputIndex == currentLength) {
                if (currentLength >= sequenceLength) {
                    slave.pass();
                } else {
                    currentLength++;
                    currentState = ROUND_CLEAR;
                    stateTimer = millis();
                }
            }
        } else {
            slave.fail();
            resetState();
        }
        lastPressedButton = -1;
    } 
    // Handle Button Press
    else if (lastPressedButton == -1 && btn != -1) {
        lastPressedButton = btn;
        setFeedback(btn, true, true);
    }

    // Timeout: Repeat sequence if user is idle
    if (btn == -1 && millis() - stateTimer > INPUT_TIMEOUT) {
        currentState = SHOWING;
        showIndex = 0;
        stateTimer = millis();
    }
}

// --- Main Logic ---

void gameLoop() {
    if (lastPressedButton != -1 && digitalRead(BUTTON_PINS[lastPressedButton]) == HIGH) {
        setFeedback(lastPressedButton, false, true);
        lastPressedButton = -1;
    }

    switch (currentState) {
        case UNENGAGED:    handleUnengaged(); break;
        case SHOWING:      handleShowing();   break;
        case WAITING_INPUT: handleWaitingInput(); break;
        case ROUND_CLEAR: 
            // Brief pause after a correct sequence before showing the next
            if (millis() - stateTimer >= WAIT_AFTER_CORRECT) {
                showIndex = 0;
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