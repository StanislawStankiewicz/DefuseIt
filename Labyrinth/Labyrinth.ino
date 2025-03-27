#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "bitmaps.h"  // Your external file containing all bitmaps

// Define color constants if desired:
#define WHITE 1
#define BLACK 0

// Define display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// SPI Pin definitions for the SH1106G
#define OLED_CS    10  // Chip Select
#define OLED_DC     9  // Data/Command
#define OLED_RST    8  // Reset
#define OLED_CLK   13  // Clock
#define OLED_MOSI  11  // MOSI (Data In)

// Create an instance of the display with pin-based SPI
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI, OLED_CLK, 
                         OLED_DC, OLED_RST, OLED_CS);

// Button and victory pins
#define BTN_LEFT      2
#define BTN_FORWARD   3
#define BTN_RIGHT     4
#define VICTORY_PIN   5

// Labyrinth map: 1 = wall, 0 = free space
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

int playerX, playerY;
int playerDir; 
int goalX, goalY;

bool lastBtnLeft    = HIGH;
bool lastBtnForward = HIGH;
bool lastBtnRight   = HIGH;

void setup() {
  Serial.begin(115200);

  pinMode(BTN_LEFT,    INPUT_PULLUP);
  pinMode(BTN_FORWARD, INPUT_PULLUP);
  pinMode(BTN_RIGHT,   INPUT_PULLUP);
  pinMode(VICTORY_PIN, OUTPUT);
  digitalWrite(VICTORY_PIN, LOW);

  // Initialize the SH1106G display using SPI
  if (!display.begin(SPI_MODE0, 0x3C)) {
    Serial.println(F("Failed to initialize SH1106G display!"));
    while (true) {}
  }

  display.clearDisplay();
  display.display();
  // Use text color = 1 (WHITE)
  display.setTextColor(1);

  // Seed random from an analog pin
  randomSeed(analogRead(0));

  placeGoal();
  placePlayer();

  Serial.print("Player start: ");
  Serial.print(playerX);
  Serial.print(", ");
  Serial.println(playerY);

  renderView();
}

// Choose a random free cell for the goal
void placeGoal() {
  do {
    goalX = random(1, 15);
    goalY = random(1, 15);
  } while (labyrinth[goalY][goalX] == 1);
}

// Choose a random free cell for the player
void placePlayer() {
  do {
    playerX = random(1, 15);
    playerY = random(1, 15);
  } while (labyrinth[playerY][playerX] == 1);
  playerDir = random(0, 4); // 0..3
}

// Check if a cell is free
bool canMove(int x, int y) {
  return (x >= 0 && x < 16 && y >= 0 && y < 16 && labyrinth[y][x] == 0);
}

// Render the forward/left/right "3D" view with bitmaps
void renderView() {
  display.clearDisplay();

  // Calculate front, left, right based on playerDir
  int frontX = playerX, frontY = playerY;
  int frontFarX = playerX, frontFarY = playerY;
  int leftX = playerX, leftY = playerY;
  int leftFarX = playerX, leftFarY = playerY;
  int rightX = playerX, rightY = playerY;
  int rightFarX = playerX, rightFarY = playerY;

  if (playerDir == 0) { 
    // facing up
    frontY--;      frontFarY -= 2;
    leftX--;       leftFarX = leftX;   leftFarY = leftY - 1;
    rightX++;      rightFarX = rightX; rightFarY = rightY - 1;
  } 
  else if (playerDir == 1) { 
    // facing right
    frontX++;      frontFarX += 2;
    leftY--;       leftFarY = leftY;   leftFarX = leftX + 1;
    rightY++;      rightFarY = rightY; rightFarX = rightX + 1;
  } 
  else if (playerDir == 2) {
    // facing down
    frontY++;      frontFarY += 2;
    leftX++;       leftFarX = leftX;   leftFarY = leftY + 1;
    rightX--;      rightFarX = rightX; rightFarY = rightY + 1;
  } 
  else if (playerDir == 3) {
    // facing left
    frontX--;      frontFarX -= 2;
    leftY++;       leftFarY = leftY;   leftFarX = leftX - 1;
    rightY--;      rightFarY = rightY; rightFarX = rightX - 1;
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
    // front cell is open, check the second cell ahead
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

  // If the goal is immediately ahead
  if (goalX == frontX && goalY == frontY) {
    display.drawBitmap(0, 0, epd_bitmap_hole, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  }
  // Or if it is two cells ahead
  else if (goalX == frontFarX && goalY == frontFarY) {
    display.drawBitmap(0, 0, epd_bitmap_hole_far, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  }

  display.display();
}

void moveForward() {
  int nextX = playerX;
  int nextY = playerY;

  switch (playerDir) {
    case 0: nextY--; break;  // up
    case 1: nextX++; break;  // right
    case 2: nextY++; break;  // down
    case 3: nextX--; break;  // left
  }

  if (canMove(nextX, nextY)) {
    playerX = nextX;
    playerY = nextY;
  }

  // Check win condition
  if (playerX == goalX && playerY == goalY) {
    display.clearDisplay();
    display.display();
    digitalWrite(VICTORY_PIN, HIGH); // Activate victory pin (LED or buzzer)
    while (true) { /* stop here */ }
  }
}

void turnLeft()  { playerDir = (playerDir + 3) % 4; }
void turnRight() { playerDir = (playerDir + 1) % 4; }

void loop() {
  bool btnLeft    = digitalRead(BTN_LEFT);
  bool btnForward = digitalRead(BTN_FORWARD);
  bool btnRight   = digitalRead(BTN_RIGHT);

  // Button press detected on falling edge (HIGH -> LOW)
  if (!btnForward && lastBtnForward) {
    moveForward();
    renderView();
    delay(200);
  }
  if (!btnLeft && lastBtnLeft) {
    turnLeft();
    renderView();
    delay(200);
  }
  if (!btnRight && lastBtnRight) {
    turnRight();
    renderView();
    delay(200);
  }

  lastBtnLeft    = btnLeft;
  lastBtnForward = btnForward;
  lastBtnRight   = btnRight;
}
