#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "bitmaps.h"          // Your external file containing all bitmaps
#include "ModuleComms.h"     // Module communications header

#define WHITE 1
#define BLACK 0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_CLK    13  // SCK
#define OLED_RST    12  // RES
#define OLED_MOSI   11  // SDA (MOSI)
#define OLED_DC     10  // DC
#define OLED_CS     9   // CS

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI, OLED_CLK,
                         OLED_DC, OLED_RST, OLED_CS);

#define BTN_LEFT      8
#define BTN_FORWARD   7
#define BTN_RIGHT     6
#define VICTORY_PIN   2

#define LABYRINTH_WIDTH  8
#define LABYRINTH_HEIGHT 8

// Example 8x8 labyrinth map: 1 = wall, 0 = free space
uint8_t labyrinth[LABYRINTH_HEIGHT][LABYRINTH_WIDTH] = {
  {1,1,1,1,1,1,1,1},
  {1,0,0,0,1,0,0,1},
  {1,0,1,0,1,0,1,1},
  {1,0,1,0,0,0,0,1},
  {1,0,1,1,1,1,0,1},
  {1,0,0,0,0,1,0,1},
  {1,1,0,1,0,1,0,1},
  {1,1,1,1,1,1,1,1}
};

int playerX, playerY;   // Current cell of the player
int playerDir;          // 0: up, 1: right, 2: down, 3: left
int goalX, goalY;       // Goal cell coordinates

// For button debouncing
bool lastBtnLeft    = HIGH;
bool lastBtnForward = HIGH;
bool lastBtnRight   = HIGH;

unsigned long lastInputTime = 0;
const unsigned long INPUT_DELAY = 200;

// Forward declarations
void gameLoop();
void resetGameState();

#define SLAVE_ADDRESS 0x15
Slave slave(SLAVE_ADDRESS, gameLoop, resetGameState, VICTORY_PIN);

void resetGameState() {
  unsigned long seed = millis() + slave.getVersion();
  randomSeed(seed);
  Serial.print("[Labyrinth] RNG seed: ");
  Serial.println(seed);
  display.clearDisplay();
  display.display();
  placeGoal();
  placePlayer();
  renderView();
}


void placeGoal() {
  do {
    goalX = random(1, LABYRINTH_WIDTH - 1);
    goalY = random(1, LABYRINTH_HEIGHT - 1);
  } while (labyrinth[goalY][goalX] == 1);
  Serial.print("[Labyrinth] Goal placed at: ");
  Serial.print(goalX);
  Serial.print(",");
  Serial.println(goalY);
}

void placePlayer() {
  do {
    playerX = random(1, LABYRINTH_WIDTH - 1);
    playerY = random(1, LABYRINTH_HEIGHT - 1);
  } while (labyrinth[playerY][playerX] == 1 || (playerX == goalX && playerY == goalY));
  
  playerDir = random(0, 4);
  Serial.print("[Labyrinth] Player placed at: ");
  Serial.print(playerX);
  Serial.print(",");
  Serial.print(playerY);
  Serial.print(" dir=");
  Serial.println(playerDir);
}

bool canMove(int x, int y) {
  if (x < 0 || x >= LABYRINTH_WIDTH || y < 0 || y >= LABYRINTH_HEIGHT) {
    return false; 
  }
  return (labyrinth[y][x] == 0);
}

void drawBitmap(const uint8_t *bitmap) {
  display.drawBitmap(0, 0, bitmap, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
}

void renderView() {
  display.clearDisplay();

  // Calculate cell positions based on the player's direction.
  int frontX = playerX, frontY = playerY;
  int frontFarX = playerX, frontFarY = playerY;
  int leftX = playerX, leftY = playerY;
  int leftFarX = playerX, leftFarY = playerY;
  int rightX = playerX, rightY = playerY;
  int rightFarX = playerX, rightFarY = playerY;

  switch(playerDir) {
    case 0: // Up
      frontY--;    frontFarY -= 2;
      leftX--;     leftFarX = leftX;  leftFarY = leftY - 1;
      rightX++;    rightFarX = rightX; rightFarY = rightY - 1;
      break;
    case 1: // Right
      frontX++;    frontFarX += 2;
      leftY--;     leftFarY = leftY;  leftFarX = leftX + 1;
      rightY++;    rightFarY = rightY; rightFarX = rightX + 1;
      break;
    case 2: // Down
      frontY++;    frontFarY += 2;
      leftX++;     leftFarX = leftX;  leftFarY = leftY + 1;
      rightX--;    rightFarX = rightX; rightFarY = rightY + 1;
      break;
    case 3: // Left
      frontX--;    frontFarX -= 2;
      leftY++;     leftFarY = leftY;  leftFarX = leftX - 1;
      rightY--;    rightFarY = rightY; rightFarX = rightX - 1;
      break;
  }

  // Left wall
  if (!canMove(leftX, leftY)) {
    drawBitmap(epd_bitmap_closed_left);
  } else {
    drawBitmap(epd_bitmap_open_left);
  }
  // Right wall
  if (!canMove(rightX, rightY)) {
    drawBitmap(epd_bitmap_closed_right);
  } else {
    drawBitmap(epd_bitmap_open_right);
  }
  // Front wall(s)
  if (!canMove(frontX, frontY)) {
    drawBitmap(epd_bitmap_closed_front);
  } else {
    if (!canMove(frontFarX, frontFarY)) {
      drawBitmap(epd_bitmap_closed_front_far);
    }
    if (!canMove(leftFarX, leftFarY)) {
      drawBitmap(epd_bitmap_closed_left_far);
    } else {
      drawBitmap(epd_bitmap_open_left_far);
    }
    if (!canMove(rightFarX, rightFarY)) {
      drawBitmap(epd_bitmap_closed_right_far);
    } else {
      drawBitmap(epd_bitmap_open_right_far);
    }
  }
  if (goalX == frontX && goalY == frontY) {
    drawBitmap(epd_bitmap_hole);
  } else if (goalX == frontFarX && goalY == frontFarY) {
    drawBitmap(epd_bitmap_hole_far);
  }
  display.display();
}

bool moveForward() {
  int nextX = playerX;
  int nextY = playerY;

  switch(playerDir) {
    case 0: nextY--; break;
    case 1: nextX++; break;
    case 2: nextY++; break;
    case 3: nextX--; break;
  }

  if (canMove(nextX, nextY)) {
    playerX = nextX;
    playerY = nextY;
  }

  if (playerX == goalX && playerY == goalY) {
    slave.pass();
    return true;
  }
  return false;
}

void turnLeft()  { 
  playerDir = (playerDir + 3) % 4; 
}
void turnRight() { 
  playerDir = (playerDir + 1) % 4; 
}

void setup() {
  Serial.begin(9600);
  delay(2000);

  pinMode(BTN_LEFT,    INPUT_PULLUP);
  pinMode(BTN_FORWARD, INPUT_PULLUP);
  pinMode(BTN_RIGHT,   INPUT_PULLUP);

  if (!display.begin(SPI_MODE0, 0x3C)) {
    Serial.println(F("Failed to initialize SH1106G display!"));
    while (true);
  }
  Serial.println("Display initialized.");
  display.clearDisplay();
  // TODO: fix noise on startup
  slave.begin();
  Serial.println("Init completed.");
}

void gameLoop() {
  bool btnLeft    = digitalRead(BTN_LEFT);
  bool btnForward = digitalRead(BTN_FORWARD);
  bool btnRight   = digitalRead(BTN_RIGHT);
  
  unsigned long currentMillis = millis();

  if (currentMillis - lastInputTime >= INPUT_DELAY) {
    if (!btnForward && lastBtnForward) {
      lastInputTime = currentMillis;
      if (moveForward()) {
        display.clearDisplay();
        display.display();
        return;
      }
      renderView();
    }
    else if (!btnLeft && lastBtnLeft) {
      lastInputTime = currentMillis;
      turnLeft();
      renderView();
    }
    else if (!btnRight && lastBtnRight) {
      lastInputTime = currentMillis;
      turnRight();
      renderView();
    }
  }

  lastBtnLeft    = btnLeft;
  lastBtnForward = btnForward;
  lastBtnRight   = btnRight;
}

void loop() {
  slave.slaveLoop();
}
