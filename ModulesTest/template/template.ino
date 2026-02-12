#include <Wire.h>
#include "ModuleComms.h"

#define BUTTON_PIN 3
#define LED_PIN 2

Slave slave(0x10, loop, resetState, LED_PIN);

void resetState() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
}

void setup() {
    slave.begin();
}

void loop() {
    slave.slaveLoop();
}

void game_loop() {
  if (igitalRead(BUTTON_PIN) == LOW) {
      slave.
    }
}
