#include <ModuleComms.h>

const uint8_t SLAVE_ADDRESS = 0x14;
const uint8_t SWITCH_PIN = 8;
const uint8_t RGB_PIN_R = 9;
const uint8_t RGB_PIN_G = 10;
const uint8_t RGB_PIN_B = 11;
const uint8_t SOLVED_LED_PIN = 2;

const unsigned long FAST_WINDOW_MS = 900;
const unsigned long HOLD_DURATION_MS = 1800;
const unsigned long HOLD_RELEASE_GRACE_MS = 120;

enum ActionType { ACTION_TOGGLE_FAST, ACTION_HOLD_LONG };

struct Challenge {
	const char *name;
	uint8_t r;
	uint8_t g;
	uint8_t b;
	ActionType action;
};

const Challenge CHALLENGES[] = {
	{"RED",    255,   0,   0, ACTION_TOGGLE_FAST},
	{"BLUE",     0,   0, 255, ACTION_TOGGLE_FAST},
	{"GREEN",    0, 255,   0, ACTION_HOLD_LONG},
	{"YELLOW", 255, 120,   0, ACTION_HOLD_LONG}
};
const uint8_t CHALLENGE_COUNT = sizeof(CHALLENGES) / sizeof(CHALLENGES[0]);

bool challengeArmed = false;
const Challenge *activeChallenge = nullptr;
int initialSwitchState = HIGH;
unsigned long firstToggleTime = 0;
unsigned long holdStartTime = 0;

void gameLoop();
void resetState();
void armChallenge();
void processToggleFast(int currentState);
void processHoldLong(int currentState);
void setColor(uint8_t r, uint8_t g, uint8_t b);

Slave slave(SLAVE_ADDRESS, gameLoop, resetState, SOLVED_LED_PIN);

void setup() {
	pinMode(SWITCH_PIN, INPUT_PULLUP);
	pinMode(RGB_PIN_R, OUTPUT);
	pinMode(RGB_PIN_G, OUTPUT);
	pinMode(RGB_PIN_B, OUTPUT);
	setColor(0, 0, 0);
	Serial.begin(9600);
	randomSeed(analogRead(A0) ^ millis());
	slave.begin();
}

void loop() {
	slave.slaveLoop();
}

void gameLoop() {
	if (!challengeArmed) {
		armChallenge();
		return;
	}

	int currentState = digitalRead(SWITCH_PIN);

	if (activeChallenge->action == ACTION_TOGGLE_FAST) {
		processToggleFast(currentState);
	} else {
		processHoldLong(currentState);
	}
}

void resetState() {
	challengeArmed = false;
	activeChallenge = nullptr;
	firstToggleTime = 0;
	holdStartTime = 0;
	initialSwitchState = digitalRead(SWITCH_PIN);
	setColor(0, 0, 0);
}

void armChallenge() {
	uint8_t seed = slave.getVersion();
	randomSeed((millis() << 4) ^ seed);
	uint8_t index = random(CHALLENGE_COUNT);
	activeChallenge = &CHALLENGES[index];
	setColor(activeChallenge->r, activeChallenge->g, activeChallenge->b);
	initialSwitchState = digitalRead(SWITCH_PIN);
	firstToggleTime = 0;
	holdStartTime = 0;
	challengeArmed = true;
	Serial.print("Switch challenge: ");
	Serial.println(activeChallenge->name);
}

void processToggleFast(int currentState) {
	unsigned long now = millis();

	if (firstToggleTime == 0) {
		if (currentState != initialSwitchState) {
			firstToggleTime = now;
		}
		return;
	}

	if (currentState == initialSwitchState) {
		if (now - firstToggleTime <= FAST_WINDOW_MS) {
			slave.pass();
		} else {
			slave.fail();
		}
		return;
	}

	if (now - firstToggleTime > FAST_WINDOW_MS) {
		slave.fail();
	}
}

void processHoldLong(int currentState) {
	unsigned long now = millis();
	bool isActive = (currentState == LOW);

	if (!isActive) {
		if (holdStartTime == 0) {
			return;
		}

		unsigned long heldFor = now - holdStartTime;
		if (heldFor >= HOLD_DURATION_MS) {
			slave.pass();
		} else if (heldFor > HOLD_RELEASE_GRACE_MS) {
			slave.fail();
		} else {
			holdStartTime = 0;
		}
		return;
	}

	if (holdStartTime == 0) {
		holdStartTime = now;
	} else if (now - holdStartTime >= HOLD_DURATION_MS) {
		slave.pass();
	}
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
	analogWrite(RGB_PIN_R, 255 - r);
	analogWrite(RGB_PIN_G, 255 - g);
	analogWrite(RGB_PIN_B, 255 - b);
}
