// Symbols module - implementation
// Controls 6 SSD1306 OLEDs through an I2C multiplexer and reads 6 buttons.

#include <Wire.h>
#include "ModuleComms.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Slave I2C address for this module
#define SLAVE_ADDRESS 0x10

// Mux settings
const uint8_t MUX_ADDR = 0x70;
const uint8_t MUX_LCD_ADDR = 0x3C; // OLED I2C address on each channel
const uint8_t CHANNEL_COUNT = 6;   // number of downstream OLEDs
const uint8_t CHANNEL_SHIFT = 1;   // if your wiring shifts channels

// OLED settings
const uint8_t SCREEN_WIDTH = 128;
const uint8_t SCREEN_HEIGHT = 64;
const int8_t OLED_RESET = -1; // not used for many I2C modules

// Button pins (one per screen). Change to match your wiring.
const uint8_t buttonPins[CHANNEL_COUNT] = {A0, A1, A2, A3, A4, A5};

// LED pin used to indicate pass state by ModuleComms
#define LED_PIN 13

// ModuleComms Slave instance (provides begin(), slaveLoop(), pass(), fail())
Slave module(SLAVE_ADDRESS, gameLoop, resetState, LED_PIN);

// Runtime state
volatile bool startPending = false;
bool displaysInitialized = false;
uint8_t targetScreen = 1; // 1..CHANNEL_COUNT
bool lastButtonState[CHANNEL_COUNT];

// Helper: select mux channel (0..7)
void selectMuxChannel(uint8_t ch) {
    if (ch > 7) return;
    Wire.beginTransmission(MUX_ADDR);
    Wire.write((uint8_t)(1 << ch));
    Wire.endTransmission();
}

void disableAllChannels() {
    Wire.beginTransmission(MUX_ADDR);
    Wire.write((uint8_t)0);
    Wire.endTransmission();
}

// Draw a single symbol ('O' or 'X') on a specific channel (1-based)
void drawSymbolOnChannel(uint8_t channel, char symbol) {
    uint8_t muxIndex = channel - 1 + CHANNEL_SHIFT;
    if (muxIndex > 7) return;
    selectMuxChannel(muxIndex);
    delay(30); // allow bus to settle

    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if (!display.begin(SSD1306_SWITCHCAPVCC, MUX_LCD_ADDR)) {
        // initialization failed for this channel; clear selection and return
        disableAllChannels();
        return;
    }

    display.clearDisplay();
    display.setTextSize(6);
    display.setTextColor(SSD1306_WHITE);
    // crude centering for single char (works for 128x64 and large font)
    int16_t x = (SCREEN_WIDTH - 36) / 2;
    int16_t y = (SCREEN_HEIGHT - 48) / 2;
    display.setCursor(x, y);
    display.print(symbol);
    display.display();

    disableAllChannels();
}

// setInitialState is called from the I2C receive handler when the master starts the game.
// Keep it small and avoid Serial or heavy I2C work here.
void resetState() {
    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
        pinMode(buttonPins[i], INPUT_PULLUP);
        lastButtonState[i] = digitalRead(buttonPins[i]);
    }
    displaysInitialized = false;
    startPending = true;
}

// Main game loop executed while the slave is active
void gameLoop() {
    // initialize displays once when start is pending
    if (startPending && !displaysInitialized) {
        // Use master version to pick deterministic screen
        randomSeed(module.getVersion());
        targetScreen = (uint8_t)random(1, CHANNEL_COUNT + 1); // 1..CHANNEL_COUNT

        for (uint8_t ch = 1; ch <= CHANNEL_COUNT; ++ch) {
            char sym = (ch == targetScreen) ? 'O' : 'X';
            drawSymbolOnChannel(ch, sym);
            delay(80);
        }

        displaysInitialized = true;
        startPending = false;
    }

    // Poll buttons (simple edge detect)
    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
        bool state = digitalRead(buttonPins[i]); // HIGH when released
        if (lastButtonState[i] == HIGH && state == LOW) {
            uint8_t pressedScreen = i + 1; // human-friendly 1-based index
            if (pressedScreen == targetScreen) module.pass();
            else module.fail();
            // after pass()/fail() the slave stops game activity until master restarts
            return;
        }
        lastButtonState[i] = state;
    }
}

void setup() {
    Wire.begin();
    module.begin();
}

void loop() {
    module.slaveLoop();
}
