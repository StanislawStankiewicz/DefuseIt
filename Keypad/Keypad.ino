const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};

bool keyPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }

  for (byte i = 0; i < COLS; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }
}

void loop() {
  char key = scanKeypad();

  if (key != '\0' && !keyPressed && (millis() - lastDebounceTime > debounceDelay)) {
    Serial.println(key);
    keyPressed = true;
    lastDebounceTime = millis();
  }
  else if (key == '\0') {
    keyPressed = false;
  }
}

char scanKeypad() {
  for (byte row = 0; row < ROWS; row++) {
    digitalWrite(rowPins[row], LOW);

    for (byte col = 0; col < COLS; col++) {
      if (digitalRead(colPins[col]) == LOW) {
        digitalWrite(rowPins[row], HIGH);
        return hexaKeys[row][col];
      }
    }

    digitalWrite(rowPins[row], HIGH);
  }
  return '\0';
}
