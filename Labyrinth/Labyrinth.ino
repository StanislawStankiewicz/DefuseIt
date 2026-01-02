#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "bitmaps.h"          // Your external file containing all bitmaps
#include "ModuleComms.h"     // Module communications header

#define CMD_RESET 0x07

#define WHITE 1
#define BLACK 0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_CS    12  // Chip Select
#define OLED_DC    11  // Data/Command
#define OLED_RST   10  // Reset
#define OLED_CLK    9  // Clock
#define OLED_MOSI   8  // MOSI (Data In)

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI, OLED_CLK,
                         OLED_DC, OLED_RST, OLED_CS);

#define BTN_LEFT      2
#define BTN_FORWARD   3
#define BTN_RIGHT     4
#define VICTORY_PIN   5

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

// Game state flags
volatile bool gameRunning   = false; // Set to true when CMD_START_GAME is received
volatile bool gameCompleted = false; // Set to true when the player reaches the goal
volatile bool resetRequested  = false; // Set to true when CMD_RESET is received

#define SLAVE_ADDRESS 0x11
Slave gameSlave(SLAVE_ADDRESS);

void resetGameState() {
  gameRunning = false;
  gameCompleted = false;
  digitalWrite(VICTORY_PIN, LOW);
  display.clearDisplay();
  display.display();
  placeGoal();
  placePlayer();
}

void startGameCallback() {
  randomSeed(gameSlave.getVersion());
  resetGameState();
  gameRunning = true;
  renderView();
}

void endGameCallback() {
  digitalWrite(VICTORY_PIN, LOW);
}

void processResetCommand() {
  Serial.println("Reset command received.");
  resetRequested = false;
  resetGameState();
}

void completeGame() {
  Serial.println("Game completed!");
  gameCompleted = true;
  display.clearDisplay();
  display.display();
  digitalWrite(VICTORY_PIN, HIGH);
  gameSlave.setStatus(STATUS_PASSED);
}

void placeGoal() {
  do {
    goalX = random(1, LABYRINTH_WIDTH - 1);
    goalY = random(1, LABYRINTH_HEIGHT - 1);
  } while (labyrinth[goalY][goalX] == 1);
}

void placePlayer() {
  do {
    playerX = random(1, LABYRINTH_WIDTH - 1);
    playerY = random(1, LABYRINTH_HEIGHT - 1);
  } while (labyrinth[playerY][playerX] == 1);
  playerDir = random(0, 4);  // Random direction between 0 and 3
}

bool canMove(int x, int y) {
  return (x >= 0 && x < LABYRINTH_WIDTH &&
          y >= 0 && y < LABYRINTH_HEIGHT &&
          labyrinth[y][x] == 0);
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
    display.drawBitmap(0, 0, epd_bitmap_closed_left, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  } else {
    display.drawBitmap(0, 0, epd_bitmap_open_left, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  }
  // Right wall
  if (!canMove(rightX, rightY)) {
    display.drawBitmap(0, 0, epd_bitmap_closed_right, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  } else {
    display.drawBitmap(0, 0, epd_bitmap_open_right, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  }
  // Front wall(s)
  if (!canMove(frontX, frontY)) {
    display.drawBitmap(0, 0, epd_bitmap_closed_front, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  } else {
    if (!canMove(frontFarX, frontFarY)) {
      display.drawBitmap(0, 0, epd_bitmap_closed_front_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    }
    if (!canMove(leftFarX, leftFarY)) {
      display.drawBitmap(0, 0, epd_bitmap_closed_left_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    } else {
      display.drawBitmap(0, 0, epd_bitmap_open_left_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    }
    if (!canMove(rightFarX, rightFarY)) {
      display.drawBitmap(0, 0, epd_bitmap_closed_right_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    } else {
      display.drawBitmap(0, 0, epd_bitmap_open_right_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    }
  }
  if (goalX == frontX && goalY == frontY) {
    display.drawBitmap(0, 0, epd_bitmap_hole, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  } else if (goalX == frontFarX && goalY == frontFarY) {
    display.drawBitmap(0, 0, epd_bitmap_hole_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  }
  display.display();
}

void moveForward() {
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
    completeGame();
  }
}

void turnLeft()  { playerDir = (playerDir + 3) % 4; }
void turnRight() { playerDir = (playerDir + 1) % 4; }

void setup() {
  Serial.begin(9600);

  pinMode(BTN_LEFT,    INPUT_PULLUP);
  pinMode(BTN_FORWARD, INPUT_PULLUP);
  pinMode(BTN_RIGHT,   INPUT_PULLUP);
  pinMode(VICTORY_PIN, OUTPUT);
  digitalWrite(VICTORY_PIN, LOW);

  if (!display.begin(SPI_MODE0, 0x3C)) {
    Serial.println(F("Failed to initialize SH1106G display!"));
    while (true);
  }
  display.clearDisplay();
  display.display();
  display.setTextColor(WHITE);

  gameSlave.begin();
  gameSlave.onGameStart(startGameCallback);
  gameSlave.onGameEnd(endGameCallback);
  Serial.println("Init completed.");
}

void loop() {
  if (resetRequested) {
    processResetCommand();
  }

  if (gameRunning && !gameCompleted) {
    bool btnLeft    = digitalRead(BTN_LEFT);
    bool btnForward = digitalRead(BTN_FORWARD);
    bool btnRight   = digitalRead(BTN_RIGHT);

    if (!btnForward && lastBtnForward) {
      moveForward();
      if (gameCompleted) {
        return;
      }
      renderView();
      delay(200);
    }
    if (!btnLeft && lastBtnLeft) {
      turnLeft();
      if (gameCompleted) {
        return;
      }
      renderView();
      delay(200);
    }
    if (!btnRight && lastBtnRight) {
      turnRight();
      if (gameCompleted) {
        return;
      }
      renderView();
      delay(200);
    }

    lastBtnLeft    = btnLeft;
    lastBtnForward = btnForward;
    lastBtnRight   = btnRight;
  }
}
