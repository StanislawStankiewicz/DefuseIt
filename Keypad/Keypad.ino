#include <LiquidCrystal.h>
#include "ModuleComms.h"

const int rs = A0, en = A1, d4 = A2, d5 = A3, d6 = A4, d7 = A5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int rowPins[4] = {6, 7, 8, 9};
const int colPins[4] = {10, 11, 12, 13};
char keymap[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

const int dataPin = 5, clockPin = 4, latchPin = 3;
const int moduleSolvedPin = 2;

enum Mode { MATH, BINARY };
Mode currentMode = MATH;

String userInput = "";
String currentEquation;
int correctAnswer;
int puzzlesSolved = 0;
int requiredPuzzles;
int mistakesLeft;
Slave module(0x10);
bool gameStarted = false;

Mode determineMode(int version) {
    if (version % 2 == 0) {
        requiredPuzzles = 1;
        mistakesLeft = 1;
        return BINARY;
    } else {
        requiredPuzzles = 3;
        mistakesLeft = 1;
        return MATH;
    }
    Serial.print("Mistakes allowed: ");
    Serial.println(mistakesLeft);
}

void generateAddition() {
  int num1 = random(1, 100);
  int num2 = random(1, 100);
  correctAnswer = num1 + num2;
  currentEquation = String(num1) + " + " + String(num2) + " =";
}

void generateEquation() {
    Serial.println("Generating new equation");
    if (currentMode == MATH) {
        switch (random(0, 4)) {
            case 0: generateAddition(); break;
            case 1: generateSubtraction(); break;
            case 2: generateMultiplication(); break;
            case 3: generateDivision(); break;
        }
    } else {
        int randomNumber = random(0, 256);
        Serial.print("Displaying binary number: ");
        Serial.println(randomNumber);
        digitalWrite(latchPin, LOW);
        shiftOut(dataPin, clockPin, MSBFIRST, randomNumber);
        digitalWrite(latchPin, HIGH);
        correctAnswer = randomNumber;
        currentEquation = "Enter number:";
    }
    lcd.clear();
    lcd.print(currentEquation);
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

char scanKeypad() {
  for (int row = 0; row < 4; row++) {
    digitalWrite(rowPins[row], LOW);
    for (int col = 0; col < 4; col++) {
      if (digitalRead(colPins[col]) == LOW) {
        delay(50); // Debounce delay
        while (digitalRead(colPins[col]) == LOW); // Wait for key release
        char key = keymap[row][col];
        digitalWrite(rowPins[row], HIGH);
        return key;
      }
    }
    digitalWrite(rowPins[row], HIGH);
  }
  return '\0'; // No key pressed
}

void checkModuleState() {
    if (puzzlesSolved >= requiredPuzzles) {
        Serial.println("Module solved");
        module.setStatus(STATUS_PASSED);
        digitalWrite(moduleSolvedPin, HIGH);
        lcd.clear();
        lcd.print("Module solved");
        return;
    }

    if (mistakesLeft <= 0) {
        Serial.println("Module failed");
        module.setStatus(STATUS_FAILED);
        lcd.clear();
        lcd.print("Module failed");
        gameStarted = false;
    }
}

void onGameStart() {
    Serial.println("Game started");
    int version = module.getVersion();
    Serial.print("Received version: ");
    Serial.println(version);
    currentMode = determineMode(version);
    puzzlesSolved = 0;
    module.setStatus(STATUS_UNSOLVED);
    digitalWrite(moduleSolvedPin, LOW);
    gameStarted = true;
    generateEquation();
}

void onGameEnd() {
    Serial.println("Game ended");
    gameStarted = false;
}

void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2);
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    pinMode(moduleSolvedPin, OUTPUT);
    digitalWrite(moduleSolvedPin, LOW);
    initKeypad();
    module.begin();
    module.onGameStart(onGameStart);
    module.onGameEnd(onGameEnd);
    Serial.println("Module initialized");
}

void loop() {
    if (!gameStarted || module.getStatus() == STATUS_PASSED || module.getStatus() == STATUS_FAILED) return;
    char key = scanKeypad();
    if (key != '\0') {
        if (key >= '0' && key <= '9') {
            userInput += key;
            lcd.setCursor(0, 1);
            lcd.print(userInput);
        } else if (key == 'A') {
            int userAnswer = userInput.toInt();
            lcd.clear();
            if (userAnswer == correctAnswer) {
                puzzlesSolved++;
            } else {
                mistakesLeft--;
                if (mistakesLeft > 0) {
                    Serial.print("Incorrect. Mistakes left: ");
                    Serial.println(mistakesLeft);
                    lcd.print("Mistakes left: ");
                    lcd.print(mistakesLeft);
                    delay(random(0, 10000));
                }
            }
            userInput = "";
            checkModuleState();
            if (module.getStatus() == STATUS_UNSOLVED) generateEquation();
        } else if (key == '*') {
            Serial.println("User cleared input");
            userInput = "";
            lcd.setCursor(0, 1);
            lcd.print("                ");
        }
    }
}
