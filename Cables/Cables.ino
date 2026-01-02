#define DATA_PIN 9
#define LATCH_PIN 10
#define CLOCK_PIN 11
#define GREEN_LED 12
#define RED_LED 13

const int inputPins[8] = {0, 1, 2, 3, 4, 5, 6, 7};
uint8_t activePattern = 0;

void setup() {
  Serial.begin(115200);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);

    for (int i = 0; i < 8; i++) {
        pinMode(inputPins[i], INPUT_PULLUP);
    }

    randomSeed(analogRead(A1));

    activePattern = random(1, 256);
    Serial.println(activePattern);
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, activePattern);
    digitalWrite(LATCH_PIN, HIGH);
}

void loop() {
    bool correct = true;
    bool anyWrong = false;

    for (int i = 0; i < 8; i++) {
        bool shouldBeUnplugged = (activePattern >> i) & 1;
        bool isUnplugged = digitalRead(inputPins[i]) == HIGH;

        if (shouldBeUnplugged && !isUnplugged) {
            correct = false;
        }
        if (!shouldBeUnplugged && isUnplugged) {
            anyWrong = true;
        }
    }

    if (correct && !anyWrong) {
        shutdownAndFlash(GREEN_LED);
    } else if (anyWrong) {
        shutdownAndFlash(RED_LED);
    }
}

void shutdownAndFlash(int ledPin) {
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0);
    digitalWrite(LATCH_PIN, HIGH);

    digitalWrite(ledPin, HIGH);

    while (true);  // Halt system
}
