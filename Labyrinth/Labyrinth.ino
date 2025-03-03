#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "bitmaps.h"  // External file containing all bitmaps

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BTN_LEFT 2
#define BTN_FORWARD 3
#define BTN_RIGHT 4
#define VICTORY_PIN 5

uint8_t labyrinth[16][16] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1},
    {1,1,1,0,1,1,1,1,0,1,1,1,0,1,1,1},
    {1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1},
    {1,0,1,1,1,1,0,1,1,1,0,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,0,1,1,1,1,1,1,1,1,0,1,1,1},
    {1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,1,1,1,0,1,1,1,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1},
    {1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,0,1,1,1,1,1,0,1,1,1},
    {1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1},
    {1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Player state
int playerX, playerY;
int playerDir;

// Goal position
int goalX, goalY;

// Button states for debouncing
bool lastBtnLeft = HIGH;
bool lastBtnForward = HIGH;
bool lastBtnRight = HIGH;

void updateDisplay() {
    display.display();
}

bool canMove(int x, int y) {
    return (x >= 0 && x < 16 && y >= 0 && y < 16 && labyrinth[y][x] == 0);
}

void placeGoal() {
    do {
        goalX = random(1, 15);
        goalY = random(1, 15);
    } while (labyrinth[goalY][goalX] == 1);
}

void placePlayer() {
    do {
        playerX = random(1, 15);
        playerY = random(1, 15);
    } while (labyrinth[playerY][playerX] == 1);
    playerDir = random(0, 3);
}

void renderView() {
    display.clearDisplay();

    int frontX = playerX, frontY = playerY;
    int frontFarX = playerX, frontFarY = playerY;
    int leftX = playerX, leftY = playerY;
    int leftFarX = playerX, leftFarY = playerY;
    int rightX = playerX, rightY = playerY;
    int rightFarX = playerX, rightFarY = playerY;

    if (playerDir == 0) { 
        frontY--; frontFarY -= 2;
        leftX--; leftFarX = leftX; leftFarY = leftY - 1;
        rightX++; rightFarX = rightX; rightFarY = rightY - 1;
    } else if (playerDir == 1) {
        frontX++; frontFarX += 2;
        leftY--; leftFarY = leftY; leftFarX = leftX + 1;
        rightY++; rightFarY = rightY; rightFarX = rightX + 1;
    } else if (playerDir == 2) {
        frontY++; frontFarY += 2;
        leftX++; leftFarX = leftX; leftFarY = leftY + 1;
        rightX--; rightFarX = rightX; rightFarY = rightY + 1;
    } else if (playerDir == 3) {
        frontX--; frontFarX -= 2;
        leftY++; leftFarY = leftY; leftFarX = leftX - 1;
        rightY--; rightFarY = rightY; rightFarX = rightX - 1;
    }

    if (!canMove(leftX, leftY)) display.drawBitmap(0, 0, epd_bitmap_closed_left, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    else display.drawBitmap(0, 0, epd_bitmap_open_left, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

    if (!canMove(rightX, rightY)) display.drawBitmap(0, 0, epd_bitmap_closed_right, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    else display.drawBitmap(0, 0, epd_bitmap_open_right, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

    if (!canMove(frontX, frontY)) {
        display.drawBitmap(0, 0, epd_bitmap_closed_front, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    } else {
        if (!canMove(frontFarX, frontFarY)) display.drawBitmap(0, 0, epd_bitmap_closed_front_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        if (!canMove(leftFarX, leftFarY)) display.drawBitmap(0, 0, epd_bitmap_closed_left_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        else display.drawBitmap(0, 0, epd_bitmap_open_left_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        if (!canMove(rightFarX, rightFarY)) display.drawBitmap(0, 0, epd_bitmap_closed_right_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        else display.drawBitmap(0, 0, epd_bitmap_open_right_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    }

    if (goalX == frontX && goalY == frontY) {
        display.drawBitmap(0, 0, epd_bitmap_hole, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    } else if (goalX == frontFarX && goalY == frontFarY) {
        display.drawBitmap(0, 0, epd_bitmap_hole_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    }

    display.display();
}

void moveForward() {
    int nextX = playerX, nextY = playerY;
    if (playerDir == 0) nextY--; else if (playerDir == 1) nextX++;
    else if (playerDir == 2) nextY++; else if (playerDir == 3) nextX--;
    
    if (canMove(nextX, nextY)) {
        playerX = nextX; 
        playerY = nextY;
    }
    
    if (playerX == goalX && playerY == goalY) {
        display.clearDisplay();
        display.display();
        digitalWrite(VICTORY_PIN, HIGH);
        while (true);
    }
}

void turnLeft() { playerDir = (playerDir + 3) % 4; }
void turnRight() { playerDir = (playerDir + 1) % 4; }

void setup() {
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_FORWARD, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(VICTORY_PIN, OUTPUT);
    digitalWrite(VICTORY_PIN, LOW);
    randomSeed(analogRead(0));
    placeGoal();
    placePlayer();

    Serial.begin(115200);
    Serial.print(playerX);
    Serial.print(" ");
    Serial.print(playerY);

    renderView();
}

void loop() {
    bool btnLeft = digitalRead(BTN_LEFT);
    bool btnForward = digitalRead(BTN_FORWARD);
    bool btnRight = digitalRead(BTN_RIGHT);

    if (!btnForward && lastBtnForward) { moveForward(); renderView(); delay(200); }
    if (!btnLeft && lastBtnLeft) { turnLeft(); renderView(); delay(200); }
    if (!btnRight && lastBtnRight) { turnRight(); renderView(); delay(200); }

    lastBtnLeft = btnLeft;
    lastBtnForward = btnForward;
    lastBtnRight = btnRight;
}