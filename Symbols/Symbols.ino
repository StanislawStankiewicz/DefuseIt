#include <Wire.h>
#include "ModuleComms.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SLAVE_ADDRESS 0x10

const uint8_t MUX_ADDR = 0x70;
const uint8_t MUX_LCD_ADDR = 0x3C; // OLED I2C address on each channel
const uint8_t CHANNEL_COUNT = 6;   // number of downstream OLEDs
const uint8_t CHANNEL_SHIFT = 1;   // if your wiring shifts channels

const uint8_t SCREEN_WIDTH = 128;
const uint8_t SCREEN_HEIGHT = 64;
const int8_t OLED_RESET = -1; // not used for many I2C modules

const uint8_t buttonPins[CHANNEL_COUNT] = {A0, A1, A2, A3, A4, A5};

#define LED_PIN 13

Slave slave(SLAVE_ADDRESS, gameLoop, resetState, LED_PIN);

volatile bool startPending = false;
bool displaysInitialized = false;
uint8_t targetScreen = 1; // 1..CHANNEL_COUNT
bool lastButtonState[CHANNEL_COUNT];

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

void drawSymbolOnChannel(uint8_t channel, char symbol) {
    uint8_t muxIndex = channel - 1 + CHANNEL_SHIFT;
    if (muxIndex > 7) return;
    selectMuxChannel(muxIndex);
    delay(30); // allow bus to settle

    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if (!display.begin(SSD1306_SWITCHCAPVCC, MUX_LCD_ADDR)) {
        disableAllChannels();
        return;
    }

    display.clearDisplay();
    display.setTextSize(6);
    display.setTextColor(SSD1306_WHITE);
    int16_t x = (SCREEN_WIDTH - 36) / 2;
    int16_t y = (SCREEN_HEIGHT - 48) / 2;
    display.setCursor(x, y);
    display.print(symbol);
    display.display();

    disableAllChannels();
}

void resetState() {
    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
        pinMode(buttonPins[i], INPUT_PULLUP);
        lastButtonState[i] = digitalRead(buttonPins[i]);
    }
    displaysInitialized = false;
    startPending = true;
}

void gameLoop() {
    if (startPending && !displaysInitialized) {
        randomSeed(slave.getVersion());
        targetScreen = (uint8_t)random(1, CHANNEL_COUNT + 1); // 1..CHANNEL_COUNT

        for (uint8_t ch = 1; ch <= CHANNEL_COUNT; ++ch) {
            char sym = (ch == targetScreen) ? 'O' : 'X';
            drawSymbolOnChannel(ch, sym);
            delay(80);
        }

        displaysInitialized = true;
        startPending = false;
    }

    for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) {
        bool state = digitalRead(buttonPins[i]); // HIGH when released
        if (lastButtonState[i] == HIGH && state == LOW) {
            uint8_t pressedScreen = i + 1; // human-friendly 1-based index
            if (pressedScreen == targetScreen) slave.pass();
            else slave.fail();
            return;
        }
        lastButtonState[i] = state;
    }
}

void setup() {
    Wire.begin();
    slave.begin();
}

void loop() {
    slave.slaveLoop();
}
