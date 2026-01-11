#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "ModuleComms.h"

// --- Hardware Pins ---
#define OLED_CLK    13  // SCK
#define OLED_RST    12  // RES
#define OLED_MOSI   11  // SDA (MOSI)
#define OLED_DC     10  // DC
#define OLED_CS     9   // CS

#define ENCODER_PIN_A 8
#define ENCODER_PIN_B 7
#define ENCODER_BTN   6
#define MORSE_LED_PIN 3
#define LED_PIN       2

// --- Display Settings ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// --- Game Constants ---
#define BLINK_UNIT 250 // ms duration of a dot

// Define strings in PROGMEM to save RAM
const char m_0[] PROGMEM = "... .... . .-.. .-..";
const char m_1[] PROGMEM = ".... .- .-.. .-.. ...";
const char m_2[] PROGMEM = "... .-.. .. -.-. -.-";
const char m_3[] PROGMEM = "- .-. .. -.-. -.-";
const char m_4[] PROGMEM = "-... --- -..- . ...";
const char m_5[] PROGMEM = ".-.. . .- -.- ...";
const char m_6[] PROGMEM = "... - .-. --- -... .";
const char m_7[] PROGMEM = "-... .. ... - .-. ---";
const char m_8[] PROGMEM = "..-. .-.. .. -.-. -.-";
const char m_9[] PROGMEM = "-... --- -- -... ...";
const char m_10[] PROGMEM = "-... .-. . .- -.-";
const char m_11[] PROGMEM = "-... .-. .. -.-. -.-";
const char m_12[] PROGMEM = "... - . .- -.-";
const char m_13[] PROGMEM = "... - .. -. --.";
const char m_14[] PROGMEM = "...- . -.-. - --- .-.";
const char m_15[] PROGMEM = "-... . .- - ...";

struct RadioTarget {
    uint16_t freq;      // Frequency * 1000
    const char* morse;  // Pointer to Flash memory
};

const RadioTarget targets[] = {
    {3505, m_0},       // shell
    {3515, m_1},       // halls
    {3522, m_2},       // slick
    {3532, m_3},       // trick
    {3535, m_4},       // boxes
    {3542, m_5},       // leaks
    {3545, m_6},       // strobe
    {3552, m_7},       // bistro
    {3555, m_8},       // flick
    {3565, m_9},       // bombs
    {3572, m_10},      // break
    {3575, m_11},      // brick
    {3582, m_12},      // steak
    {3592, m_13},      // sting
    {3595, m_14},      // vector
    {3600, m_15}       // beats
};
#define NUM_TARGETS (sizeof(targets) / sizeof(targets[0]))

// --- Globals ---
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

int targetIndex = 0;            // The correct answer index
uint16_t currentFreq = 3500;    // The user's currently selected frequency (3500-3600)

// Encoder State
int lastClk = LOW;

// Button State
int lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Morse Code State
int morseIndex = 0;             // Position in the morse string
bool ledState = false;          // Current LED state
unsigned long lastMorseTime = 0;
bool isGap = false;             // Are we in a gap between symbols?

// Forward Declarations
void gameLoop();
void resetState();
void updateDisplay();

// Module Communication
#define SLAVE_ADDRESS 0x16
Slave module(SLAVE_ADDRESS, gameLoop, resetState, LED_PIN);

void setup() {
    Serial.begin(9600);

    // Pins
    pinMode(MORSE_LED_PIN, OUTPUT);
    pinMode(ENCODER_PIN_A, INPUT_PULLUP);
    pinMode(ENCODER_PIN_B, INPUT_PULLUP);
    pinMode(ENCODER_BTN, INPUT_PULLUP);

    // Initial state
    lastClk = digitalRead(ENCODER_PIN_A);

    // Display
    if(!display.begin(0, true)) { 
        for(;;); // Don't proceed, loop forever
    }
    
    display.clearDisplay();
    display.display();
    
    // Module Comms
    module.begin();
    
    // Initialize random seed (using analog pin not connected)
    Serial.println("Setup Complete!");
}

void loop() {
    module.slaveLoop();
}

void resetState() {
    randomSeed(millis() + slave.getVersion());
    targetIndex = random(NUM_TARGETS);
    
    char buffer[32];
    strcpy_P(buffer, targets[targetIndex].morse);
    
    currentFreq = 3500; 

    morseIndex = 0;
    ledState = false;
    isGap = false;
    lastMorseTime = 0;
    digitalWrite(MORSE_LED_PIN, LOW);

    updateDisplay();
}

// Called continuously when game is active
void gameLoop() {
    handleEncoder();
    handleButton();
    handleMorseCode();
}

void handleEncoder() {
    int newClk = digitalRead(ENCODER_PIN_A);
    if (newClk != lastClk && newClk == LOW) {
        if (digitalRead(ENCODER_PIN_B) == HIGH) {
            currentFreq++;
            if (currentFreq > 3600) currentFreq = 3500;
        } else {
            currentFreq--;
            if (currentFreq < 3500) currentFreq = 3600;
        }
        updateDisplay();
    }
    lastClk = newClk;
}

void handleButton() {
    int reading = digitalRead(ENCODER_BTN);
    if (reading != lastBtnState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading == LOW) { // Pressed
             if (currentFreq == targets[targetIndex].freq) {
                 module.pass();
             } else {
                 module.fail();
             }
             delay(200); 
        }
    }
    lastBtnState = reading;
}

void handleMorseCode() {
    unsigned long now = millis();
    
    // Copy current target's morse string from Flash to RAM buffer
    char morseStr[32];
    strcpy_P(morseStr, targets[targetIndex].morse);
    
    int len = strlen(morseStr);

    if (now - lastMorseTime >= (isGap ? BLINK_UNIT : (ledState ? (morseStr[morseIndex] == '-' ? 3*BLINK_UNIT : BLINK_UNIT) : BLINK_UNIT))) {
        
        if (ledState) {
            digitalWrite(MORSE_LED_PIN, LOW);
            ledState = false;
            isGap = true; 
            lastMorseTime = now;
            
            morseIndex++;
            if (morseIndex >= len) {
            }

        } else {
            // LED was OFF.
            if (isGap) {
                // We just finished a standard inter-symbol gap (1 unit).
                isGap = false;

                 if (morseIndex >= len) {
                    // Start over, long gap was needed.
                    morseIndex = 0;
                    lastMorseTime = now + (7 * BLINK_UNIT); // Add extra delay for word gap
                    return; 
                }
                
                // If character is space, it's a word/letter gap
                if (morseStr[morseIndex] == ' ') {
                    lastMorseTime = now + (2 * BLINK_UNIT); // Extra delay (already did 1, need 3 total)
                    morseIndex++;
                    return;
                }

                // Turn LED ON for next symbol
                digitalWrite(MORSE_LED_PIN, HIGH);
                ledState = true;
                lastMorseTime = now;
            } else {
                // Initial start.
                digitalWrite(MORSE_LED_PIN, HIGH);
                ledState = true;
                lastMorseTime = now;
            }
        }
    }
}


void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(10, 20);
    
    // Format: 3.505 MHz
    display.print(currentFreq / 1000);
    display.print(".");
    
    int decimal = currentFreq % 1000; // e.g., 505
    if (decimal < 100) display.print("0");
    if (decimal < 10) display.print("0");
    display.print(decimal);
    
    display.setTextSize(1);
    display.print(" MHz");
    
    display.display();
}
