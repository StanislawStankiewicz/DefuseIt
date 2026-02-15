#include <LiquidCrystal.h>
#include "ModuleComms.h"

// after a fail wait between 5 to 10 seconds while display ... animation

const int rs = 11, en = 12, d4 = 13, d5 = A0, d6 = A1, d7 = A2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int rowPins[4] = {3, 4, 5, 6};
const int colPins[4] = {7, 8, 9, 10};
char keymap[4][4] = {
  {'D', '#', '0', '*'},
  {'C', '9', '8', '7'},
  {'B', '6', '5', '4'},
  {'A', '3', '2', '1'}
}; 
const int moduleSolvedPin = 2;

enum Mode { MATH, BINARY };
Mode currentMode = MATH;

String userInput = "";
String currentEquation;
int correctAnswer;
int puzzlesSolved = 0;
int requiredPuzzles;
int mistakesLeft;

const uint8_t SLAVE_ADDRESS = 0x12;
void gameLoop();
void resetState();
Slave slave(SLAVE_ADDRESS, gameLoop, resetState, moduleSolvedPin);

bool updateDisplay = false;
String lastBinary = "";
bool waitingAfterMistake = false;
unsigned long nextEquationAt = 0;
const unsigned long KEYPAD_DEBOUNCE_MS = 50;
char lastRawKey = '\0';
char debouncedKey = '\0';
bool keyReported = false;
unsigned long keyLastChangeAt = 0;

void lcdClearLine(int row) {
  lcd.setCursor(0, row);
  for (int i = 0; i < 16; i++) lcd.print(' ');
  lcd.setCursor(0, row);
}

void lcdPrintLine(int row, const String &s) {
  lcd.setCursor(0, row);
  lcd.print(s);
  int remaining = 16 - s.length();
  for (int i = 0; i < remaining; i++) lcd.print(' ');
  lcd.setCursor(0, row);
}

Mode selectMode(int version) {
    if (version % 2 == 0) {
        requiredPuzzles = 1;
        mistakesLeft = 1;
        return BINARY;
    } else {
        requiredPuzzles = 3;
        mistakesLeft = 1;
        return MATH;
    }
}

void generateAddition() {
  int num1 = random(1, 100);
  int num2 = random(1, 100);
  correctAnswer = num1 + num2;
  currentEquation = String(num1) + " + " + String(num2) + " =";
}

void generateEquation() {
  if (currentMode == MATH) {
    switch (random(0, 4)) {
      case 0: generateAddition(); break;
      case 1: generateSubtraction(); break;
      case 2: generateMultiplication(); break;
      case 3: generateDivision(); break;
    }
    lastBinary = "";
  } else {
    int randomNumber = random(0, 256);
    correctAnswer = randomNumber;
    currentEquation = String("Number: ") + String(randomNumber);
    lastBinary = "";
    for (int b = 7; b >= 0; b--) {
      lastBinary += ((randomNumber >> b) & 1) ? '1' : '0';
    }
  }
  updateDisplay = true;
}

void generateSubtraction() {
  int num1 = random(1, 100);
  int num2 = random(1, num1 + 1); 
  correctAnswer = num1 - num2;
  currentEquation = String(num1) + " - " + String(num2) + " =";
}

void generateMultiplication() {
  int num1, num2;
  do {
    num1 = random(1, 20);
    num2 = random(1, 20);
  } while (num1 * num2 >= 200);
  correctAnswer = num1 * num2;
  currentEquation = String(num1) + " * " + String(num2) + " =";
}

void generateDivision() {
  int num2 = random(1, 10);
  int num1 = num2 * random(1, 19);
  correctAnswer = num1 / num2;
  currentEquation = String(num1) + " / " + String(num2) + " =";
}

void initKeypad() {
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
    pinMode(colPins[i], INPUT_PULLUP);
  }
  Serial.println("Keypad initialized.");
}

char scanRawKeypad() {
  for (int row = 0; row < 4; row++) {
    digitalWrite(rowPins[row], LOW);
    for (int col = 0; col < 4; col++) {
      if (digitalRead(colPins[col]) == LOW) {
        char key = keymap[row][col];
        digitalWrite(rowPins[row], HIGH);
        return key;
      }
    }
    digitalWrite(rowPins[row], HIGH);
  }
  return '\0';
}

char scanKeypad() {
  char rawKey = scanRawKeypad();
  unsigned long now = millis();

  if (rawKey != lastRawKey) {
    lastRawKey = rawKey;
    keyLastChangeAt = now;
  }

  if (now - keyLastChangeAt < KEYPAD_DEBOUNCE_MS) {
    return '\0';
  }

  if (rawKey != debouncedKey) {
    debouncedKey = rawKey;
    if (debouncedKey == '\0') {
      keyReported = false;
    }
  }

  if (debouncedKey != '\0' && !keyReported) {
    keyReported = true;
    return debouncedKey;
  }

  return '\0';
}

void checkModuleState() {
  if (puzzlesSolved >= requiredPuzzles) {
    Serial.println("Module solved");
    slave.pass();
    return;
  }

  if (mistakesLeft <= 0) {
    Serial.println("Module failed");
    slave.fail();
  }
}

void resetState() {
  int version = slave.getVersion();
  unsigned long seed = millis() + version;

  Serial.print("RNG version: ");
  Serial.println(version);
  Serial.print("RNG seed: ");
  Serial.println(seed);

  randomSeed(seed);
  currentMode = selectMode(version);
  waitingAfterMistake = false;
  nextEquationAt = 0;
  generateEquation();
  lcd.clear();
}

void gameLoop() {
  if (waitingAfterMistake) {
    if (millis() >= nextEquationAt) {
      waitingAfterMistake = false;
      generateEquation();
    } else {
      return;
    }
  }

  if (updateDisplay) {
    if (currentMode == BINARY && lastBinary.length() > 0) {
      lcd.clear();
      lcdPrintLine(0, lastBinary);
    } else {
      lcd.clear();
      lcdPrintLine(0, currentEquation);
    }
    updateDisplay = false;
  }

  char key = scanKeypad();
  if (key != '\0') {
    if (key >= '0' && key <= '9') {
      userInput += key;
      lcd.setCursor(0, 1);
      lcd.print(userInput);
      Serial.print("LCD line2: ");
      Serial.println(userInput);
    } else if (key == 'A') {
      int userAnswer = userInput.toInt();
      lcd.clear();
      if (userAnswer == correctAnswer) {
        puzzlesSolved++;
        if (puzzlesSolved >= requiredPuzzles) {
          Serial.println("Module solved");
          slave.pass();
        } else {
          generateEquation();
        }
      } else {
        mistakesLeft--;
        if (mistakesLeft <= 0) {
          Serial.println("Module failed");
          slave.fail();
        } else {
          Serial.print("Incorrect. Mistakes left: ");
          Serial.println(mistakesLeft);
          unsigned long waitMs = random(1000, 10000);
          nextEquationAt = millis() + waitMs;
          waitingAfterMistake = true;
          Serial.print("Next equation in ms: ");
          Serial.println(waitMs);
        }
      }
      userInput = "";
    } else if (key == '*') {
      Serial.println("User cleared input");
      userInput = "";
      lcdClearLine(1);
      Serial.println("LCD line2 cleared");
    }
  }
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  initKeypad();
  pinMode(moduleSolvedPin, OUTPUT);
  digitalWrite(moduleSolvedPin, LOW);
  slave.begin();
  Serial.println("Module initialized");
}

void loop() {
  slave.slaveLoop();
}
