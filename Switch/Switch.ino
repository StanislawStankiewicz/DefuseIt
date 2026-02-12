#include <ModuleComms.h>

const uint8_t SLAVE_ADDRESS = 0x14;
const uint8_t SWITCH_PIN = 8;
const uint8_t RGB_PIN_R = 9;
const uint8_t RGB_PIN_G = 10;
const uint8_t RGB_PIN_B = 11;
const uint8_t SOLVED_LED_PIN = 2;

const unsigned long CLICK_WINDOW_MS = 1000;

enum ActionType { ACTION_CLICK, ACTION_HOLD };
enum ColorId { COLOR_RED, COLOR_BLUE, COLOR_GREEN, COLOR_YELLOW, COLOR_WHITE, COLOR_PURPLE };

struct ColorDef {
	const char *name;
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

const ColorDef COLORS[] = {
	{"RED",    255,   0,   0},
	{"BLUE",     0,   0, 255},
	{"GREEN",    0, 255,   0},
	{"YELLOW", 255, 120,   0},
	{"WHITE",  255, 255, 255},
	{"PURPLE", 180,   0, 255}
};
const uint8_t COLOR_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);

const ColorId INITIAL_COLOR_POOL[] = {
	COLOR_RED,
	COLOR_BLUE,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_WHITE,
	COLOR_PURPLE
};
const uint8_t INITIAL_COLOR_COUNT = sizeof(INITIAL_COLOR_POOL) / sizeof(INITIAL_COLOR_POOL[0]);

const ColorId RELEASE_COLOR_POOL[] = {
	COLOR_BLUE,
	COLOR_WHITE,
	COLOR_YELLOW,
	COLOR_PURPLE,
	COLOR_RED
};
const uint8_t RELEASE_COLOR_COUNT = sizeof(RELEASE_COLOR_POOL) / sizeof(RELEASE_COLOR_POOL[0]);

bool challengeArmed = false;
ColorId initialColor = COLOR_RED;
ActionType activeAction = ACTION_HOLD;
ColorId releaseColor = COLOR_BLUE;
uint8_t releaseDigit = 8;
int initialSwitchState = HIGH;
unsigned long interactionStartTime = 0;
bool holdStripActive = false;

void gameLoop();
void resetState();
void armChallenge();
void processClick(int currentState);
void processHold(int currentState);
ActionType determineAction(ColorId color, uint8_t version);
ColorId pickReleaseColor();
uint8_t digitForReleaseColor(ColorId color);
uint8_t getLastTimerDigit();
void setColor(uint8_t r, uint8_t g, uint8_t b);
void setColorById(ColorId color);

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

	if (activeAction == ACTION_CLICK) {
		processClick(currentState);
	} else {
		processHold(currentState);
	}
}

void resetState() {
	challengeArmed = false;
	interactionStartTime = 0;
	holdStripActive = false;
	activeAction = ACTION_HOLD;
	releaseColor = COLOR_BLUE;
	releaseDigit = 8;
	initialSwitchState = digitalRead(SWITCH_PIN);
	setColor(0, 0, 0);
}

void armChallenge() {
	uint8_t version = slave.getVersion();
	randomSeed((millis() << 4) ^ version ^ (slave.getRemainingSeconds() << 1));
	initialColor = INITIAL_COLOR_POOL[random(INITIAL_COLOR_COUNT)];
	activeAction = determineAction(initialColor, version);
	setColorById(initialColor);
	initialSwitchState = digitalRead(SWITCH_PIN);
	interactionStartTime = 0;
	holdStripActive = false;
	releaseColor = COLOR_BLUE;
	releaseDigit = 8;
	challengeArmed = true;

	Serial.print("Switch color: ");
	Serial.print(COLORS[initialColor].name);
	Serial.print(" | version: ");
	Serial.print(version);
	Serial.print(" | action: ");
	Serial.println(activeAction == ACTION_CLICK ? "CLICK" : "HOLD");
}

void processClick(int currentState) {
	unsigned long now = millis();

	if (interactionStartTime == 0) {
		if (currentState != initialSwitchState) {
			interactionStartTime = now;
		}
		return;
	}

	if (currentState == initialSwitchState) {
		if (now - interactionStartTime < CLICK_WINDOW_MS) {
			slave.pass();
		} else {
			slave.fail();
		}
		return;
	}

	if (now - interactionStartTime >= CLICK_WINDOW_MS) {
		slave.fail();
	}
}


void processHold(int currentState) {
	bool isPressed = (currentState == LOW);

	if (!holdStripActive) {
		if (isPressed) {
			holdStripActive = true;
			interactionStartTime = millis();
			releaseColor = pickReleaseColor();
			releaseDigit = digitForReleaseColor(releaseColor);
			setColorById(releaseColor);

			Serial.print("Release color: ");
			Serial.print(COLORS[releaseColor].name);
			Serial.print(" | target digit: ");
			Serial.println(releaseDigit);
		}
		return;
	}

	if (isPressed) {
		return;
	}

	uint8_t timerDigit = getLastTimerDigit();
	if (timerDigit == releaseDigit) {
		slave.pass();
	} else {
		slave.fail();
	}
}

ActionType determineAction(ColorId color, uint8_t version) {
	if (color == COLOR_GREEN) {
		return ACTION_CLICK;
	}

	if ((color == COLOR_RED || color == COLOR_BLUE) && (version % 2 == 0)) {
		return ACTION_CLICK;
	}

	if ((color == COLOR_BLUE || color == COLOR_YELLOW) && (version % 3 == 0)) {
		return ACTION_HOLD;
	}

	if (color == COLOR_YELLOW && (version % 2 == 0) && (version % 3 == 0)) {
		return ACTION_CLICK;
	}

	return ACTION_HOLD;
}

ColorId pickReleaseColor() {
	return RELEASE_COLOR_POOL[random(RELEASE_COLOR_COUNT)];
}

uint8_t digitForReleaseColor(ColorId color) {
	if (color == COLOR_BLUE) {
		return 4;
	}
	if (color == COLOR_WHITE) {
		return 1;
	}
	if (color == COLOR_YELLOW) {
		return 5;
	}
	if (color == COLOR_PURPLE) {
		return 0;
	}
	return 8;
}

uint8_t getLastTimerDigit() {
	uint16_t remainingSeconds = slave.getRemainingSeconds();
	return remainingSeconds % 10;
}

void setColorById(ColorId color) {
	setColor(COLORS[color].r, COLORS[color].g, COLORS[color].b);
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
	analogWrite(RGB_PIN_R, 255 - r);
	analogWrite(RGB_PIN_G, 255 - g);
	analogWrite(RGB_PIN_B, 255 - b);
}
