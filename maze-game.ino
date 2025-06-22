#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// --- Configuration ---
// Screen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1     
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// MPU6050 Sensor
Adafruit_MPU6050 mpu;

// Buzzer
#define BUZZER_PIN 23

// Game Settings
#define BALL_RADIUS 2
#define TILT_SENSITIVITY 0.3 // Higher value = faster ball movement
#define WALL_THICKNESS 8

// --- Game Objects & State ---
// Ball position
float ballX = WALL_THICKNESS + BALL_RADIUS + 2;
float ballY = WALL_THICKNESS + BALL_RADIUS + 2;

// Goal position
int goalX = 110;
int goalY = 50;
int goalSize = 10;

// Game State
enum GameState {
  STATE_PLAYING,
  STATE_WIN
};
GameState currentState = STATE_PLAYING;

// Define the maze layout (1 = wall, 0 = path)
// Each cell represents an 8x8 pixel block.
const int mazeCols = SCREEN_WIDTH / WALL_THICKNESS;
const int mazeRows = SCREEN_HEIGHT / WALL_THICKNESS;

byte maze[mazeRows][mazeCols] = {
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
  {1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1},
  {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1},
  {1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1},
  {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
  {1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); 
  }

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("MPU6050 Not Found");
    display.display();
    for (;;);
  }
  
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("Game Ready!");
  display.clearDisplay();
  playStartSound();
}


void loop() {
  switch (currentState) {
    case STATE_PLAYING:
      updateGame();
      drawGame();
      break;
    case STATE_WIN:
      drawWinScreen();
      delay(3000);
      resetGame();
      break;
  }
  
  // Control the frame rate
  delay(10); 
}

void updateGame() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Calculate ball's next potential position based on tilt
  float nextX = ballX + (a.acceleration.y * TILT_SENSITIVITY);
  float nextY = ballY + (a.acceleration.x * TILT_SENSITIVITY);

  // --- Collision Detection ---
  // Check screen bounds first
  if (nextX - BALL_RADIUS < 0 || nextX + BALL_RADIUS > SCREEN_WIDTH ||
      nextY - BALL_RADIUS < 0 || nextY + BALL_RADIUS > SCREEN_HEIGHT) {
    playBonkSound();
    return; // Don't update position
  }
  
  // Check maze walls
  int mazeX = nextX / WALL_THICKNESS;
  int mazeY = nextY / WALL_THICKNESS;

  if (maze[mazeY][mazeX] == 1) {
    playBonkSound();
    return; // It's a wall, don't move
  }

  // If no collision, update the ball's position
  ballX = nextX;
  ballY = nextY;

  // --- Win Condition ---
  if (ballX > goalX && ballX < goalX + goalSize &&
      ballY > goalY && ballY < goalY + goalSize) {
    currentState = STATE_WIN;
    playWinSound();
  }
}

void drawGame() {
  display.clearDisplay();
  drawMaze();
  drawGoal();
  drawBall();
  display.display();
}

void drawMaze() {
  display.setTextColor(SSD1306_WHITE);
  for (int y = 0; y < mazeRows; y++) {
    for (int x = 0; x < mazeCols; x++) {
      if (maze[y][x] == 1) {
        display.fillRect(x * WALL_THICKNESS, y * WALL_THICKNESS, WALL_THICKNESS, WALL_THICKNESS, SSD1306_WHITE);
      }
    }
  }
}

void drawBall() {
  display.fillCircle(ballX, ballY, BALL_RADIUS, SSD1306_WHITE);
}

void drawGoal() {
  display.drawRect(goalX, goalY, goalSize, goalSize, SSD1306_WHITE);
  display.drawRect(goalX+2, goalY+2, goalSize-4, goalSize-4, SSD1306_WHITE);
}

void drawWinScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(18, 25);
  display.println("YOU WIN!");
  display.display();
}

void resetGame() {
  ballX = WALL_THICKNESS + BALL_RADIUS + 2;
  ballY = WALL_THICKNESS + BALL_RADIUS + 2;
  currentState = STATE_PLAYING;
  playStartSound();
}

// --- Sound Effects ---
void playBonkSound() {
  tone(BUZZER_PIN, 150, 50); // tone(pin, frequency, duration)
}

void playWinSound() {
  tone(BUZZER_PIN, 600, 100);
  delay(120);
  tone(BUZZER_PIN, 800, 100);
  delay(120);
  tone(BUZZER_PIN, 1000, 150);
}

void playStartSound() {
  tone(BUZZER_PIN, 400, 80);
  delay(100);
  tone(BUZZER_PIN, 600, 80);
}