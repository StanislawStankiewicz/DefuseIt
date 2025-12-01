#include <Wire.h>
#include "ModuleComms.h"

#define BUTTON_PIN 2
#define LED_PIN 3

Slave module(0x10, loop, resetState, LED_PIN);

void resetState() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
}

void setup() {
    module.begin();
}

void loop() {
    module.slaveLoop();
}

void game_loop() {
  if (igitalRead(BUTTON_PIN) == LOW) {
      module.
    }
}
