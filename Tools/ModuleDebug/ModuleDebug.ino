
#include <Wire.h>
#include "ModuleComms.h"

const uint8_t SLAVE_ADDRESS = 0x12;
const uint8_t MODULE_SOLVED_PIN = 2;

const unsigned long AUTO_PASS_DELAY_MS = 3000;

unsigned long gameStartTime = 0;
bool pendingAutoPass = false;

void gameLoop();
void resetState();

Slave slave(SLAVE_ADDRESS, gameLoop, resetState, MODULE_SOLVED_PIN);

void resetState() {
	gameStartTime = millis();
	pendingAutoPass = true;
	Serial.println("Debug: resetState - game start time set");

	long seedValue = analogRead(A3) + millis();
	Serial.print("Debug: random seed = ");
	Serial.println(seedValue);
	randomSeed(seedValue);

	int randCheck = random(0, 1000);
	Serial.print("Debug: random check value = ");
	Serial.println(randCheck);
}

void gameLoop() {
	if (pendingAutoPass && (millis() - gameStartTime >= AUTO_PASS_DELAY_MS)) {
		Serial.println("Debug: auto-pass delay reached, passing module");
		pendingAutoPass = false;
		slave.pass();
	}
}

void setup() {
	Serial.begin(9600);
	Serial.println("Debug: ModuleDebug setup start");

	pinMode(MODULE_SOLVED_PIN, OUTPUT);
	digitalWrite(MODULE_SOLVED_PIN, LOW);

	long seedValue = analogRead(A3) + millis();
	Serial.print("Debug: random seed (setup) = ");
	Serial.println(seedValue);
	randomSeed(seedValue);

	int randCheck = random(0, 1000);
	Serial.print("Debug: random check (setup) = ");
	Serial.println(randCheck);

	slave.begin();
	Serial.print("Debug: ModuleComms slave started at address 0x");
	Serial.println(SLAVE_ADDRESS, HEX);
}

void loop() {
	slave.slaveLoop();
}
