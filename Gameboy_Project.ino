#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Display pins
#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9

// Joystick pins
#define JOY_UP   2
#define JOY_DWN  3
#define JOY_LFT  4
#define JOY_RHT  5
#define JOY_MID  6
#define JOY_SET  7
#define JOY_RST  12

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Game states
enum GameState {
  STATE_MENU,
  STATE_SNAKE,
  STATE_PACMAN,
  STATE_BREAKOUT,
  STATE_INVADERS,
  STATE_POKEYMON,
  STATE_MEGAMINI
};

// Game variables
GameState currentState = STATE_MENU;
int selectedGame = 0;
const int gameCount = 6;
unsigned long lastInputTime = 0;
unsigned long lastFrameTime = 0;
const int inputDelay = 200;
const int frameDelay = 50; // 20 FPS

// Custom colors
#define ST77XX_PINK    0xFC1F
#define ST77XX_PURPLE  0x8010
#define ST77XX_LIME    0x07E0

const char* gameNames[] = {
  "Snake",
  "Pac-Man",
  "Breakout",
  "Invaders",
  "Pokeymon",
  "MegaMini"
};

// Game icons (16x16 representations)
const uint16_t gameIcons[6][16] = {
  { // Snake
    0x0000, 0x0000, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x0000, 0x0000,
    0x0000, 0x07E0, 0x07E0, 0x07E0, 0x0000, 0x0000, 0x0000, 0x0000
  },
  { // Pac-Man
    0x0000, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0,
    0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0x0000, 0x0000
  },
  { // Breakout
    0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
    0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000
  },
  { // Invaders
    0x0000, 0x0C30, 0x1C38, 0x3E7C, 0x3FFC, 0x7FFE, 0x7FFE, 0x5FFA,
    0x7FFE, 0x3FFC, 0x1C38, 0x0C30, 0x0000, 0x0000, 0x0000, 0x0000
  },
  { // Pokeymon
    0x0000, 0x0000, 0x0FF0, 0x1FF8, 0x3FFC, 0x3FFC, 0x3FFC, 0x1FF8,
    0x1FF8, 0x0FF0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
  },
  { // MegaMini
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
  }
};

// ======================
// COMMON FUNCTIONS
// ======================

void showGameOver(const char* gameName, int score) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(20, 40);
  tft.print("GAME OVER");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, 60);
  tft.print(gameName);
  tft.print(": ");
  tft.print(score);
  delay(2000);
  currentState = STATE_MENU;
  tft.fillScreen(ST77XX_BLACK);
}

bool shouldReturnToHome() {
  if (digitalRead(JOY_RST) == LOW) {
    delay(50); // Debounce
    if (digitalRead(JOY_RST) == LOW) {
      tft.fillScreen(ST77XX_BLACK);
      currentState = STATE_MENU;
      return true;
    }
  }
  return false;
}

// ======================
// IMPROVED MENU SYSTEM
// ======================

void drawMenu() {
  static int lastSelected = -1;
  
  // Only redraw if selection changed
  if (selectedGame != lastSelected) {
    tft.fillScreen(ST77XX_BLACK);
    
    // Draw title
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(40, 5);
    tft.print("GAME HUB");
    
    // Draw grid (2 rows × 3 columns)
    for (int i = 0; i < gameCount; i++) {
      int col = i % 3;
      int row = i / 3;
      int x = 20 + col * 45;
      int y = 30 + row * 60;
      
      // Selection border
      if (i == selectedGame) {
        tft.drawRoundRect(x-3, y-3, 36, 56, 5, ST77XX_GREEN);
      }
      
      // Game icon
      for (int dy = 0; dy < 4; dy++) {
        for (int dx = 0; dx < 4; dx++) {
          if (gameIcons[i][dy*4 + dx]) {
            tft.fillRect(x + dx*4, y + dy*4, 4, 4, getGameColor(i));
          }
        }
      }
      
      // Game name
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(x, y + 20);
      tft.print(gameNames[i]);
    }
    lastSelected = selectedGame;
  }
}

uint16_t getGameColor(int gameIndex) {
  switch(gameIndex) {
    case 0: return ST77XX_GREEN;
    case 1: return ST77XX_YELLOW;
    case 2: return ST77XX_CYAN;
    case 3: return ST77XX_LIME;
    case 4: return ST77XX_PINK;
    case 5: return ST77XX_BLUE;
    default: return ST77XX_WHITE;
  }
}

void handleMenuInput() {
  if (millis() - lastInputTime < inputDelay) return;
  
  if (digitalRead(JOY_DWN) == LOW) {
    selectedGame = (selectedGame + 3) % gameCount;
    lastInputTime = millis();
  } 
  else if (digitalRead(JOY_UP) == LOW) {
    selectedGame = (selectedGame - 3 + gameCount) % gameCount;
    lastInputTime = millis();
  }
  else if (digitalRead(JOY_RHT) == LOW) {
    selectedGame = (selectedGame + 1) % gameCount;
    lastInputTime = millis();
  }
  else if (digitalRead(JOY_LFT) == LOW) {
    selectedGame = (selectedGame - 1 + gameCount) % gameCount;
    lastInputTime = millis();
  }
  else if (digitalRead(JOY_MID) == LOW || digitalRead(JOY_SET) == LOW) {
    currentState = static_cast<GameState>(selectedGame + 1);
    tft.fillScreen(ST77XX_BLACK);
    lastInputTime = millis();
    delay(300);
  }
}

// ======================
// SNAKE GAME (Complete)
// ======================

struct SnakeSegment {
  int x, y;
  SnakeSegment* next;
};

SnakeSegment* snakeHead;
int snakeDirX = 1, snakeDirY = 0;
int foodX, foodY;
int snakeLength = 3;
bool snakeGameOver = false;
int snakeScore = 0;

void initSnake() {
  snakeHead = new SnakeSegment{10, 10, nullptr};
  SnakeSegment* tail = snakeHead;
  for (int i = 1; i < snakeLength; i++) {
    tail->next = new SnakeSegment{10 - i, 10, nullptr};
    tail = tail->next;
  }
  snakeScore = 0;
  spawnFood();
}

void spawnFood() {
  foodX = random(2, tft.width() / 8 - 2);
  foodY = random(2, tft.height() / 8 - 2);
}

void handleSnakeInput() {
  if (digitalRead(JOY_UP) == LOW && snakeDirY != 1) {
    snakeDirX = 0; snakeDirY = -1;
  } else if (digitalRead(JOY_DWN) == LOW && snakeDirY != -1) {
    snakeDirX = 0; snakeDirY = 1;
  } else if (digitalRead(JOY_LFT) == LOW && snakeDirX != 1) {
    snakeDirX = -1; snakeDirY = 0;
  } else if (digitalRead(JOY_RHT) == LOW && snakeDirX != -1) {
    snakeDirX = 1; snakeDirY = 0;
  }
}

void updateSnake() {
  SnakeSegment* newHead = new SnakeSegment{snakeHead->x + snakeDirX, snakeHead->y + snakeDirY, snakeHead};
  snakeHead = newHead;

  if (snakeHead->x == foodX && snakeHead->y == foodY) {
    spawnFood();
    snakeLength++;
    snakeScore += 10;
  } else {
    SnakeSegment* current = snakeHead;
    while (current->next->next != nullptr) {
      current = current->next;
    }
    delete current->next;
    current->next = nullptr;
  }
}

void checkSnakeCollision() {
  // Wall collision
  if (snakeHead->x < 0 || snakeHead->x >= tft.width() / 8 || 
      snakeHead->y < 0 || snakeHead->y >= tft.height() / 8) {
    snakeGameOver = true;
  }

  // Self collision
  SnakeSegment* current = snakeHead->next;
  while (current != nullptr) {
    if (snakeHead->x == current->x && snakeHead->y == current->y) {
      snakeGameOver = true;
      break;
    }
    current = current->next;
  }
}

void drawSnakeGame() {
  tft.fillScreen(ST77XX_BLACK);
  
  // Draw food
  tft.fillRect(foodX * 8, foodY * 8, 8, 8, ST77XX_RED);
  
  // Draw snake
  SnakeSegment* current = snakeHead;
  while (current != nullptr) {
    tft.fillRect(current->x * 8, current->y * 8, 8, 8, ST77XX_GREEN);
    current = current->next;
  }

  // Draw score
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(2, 2);
  tft.print("Score: ");
  tft.print(snakeScore);
}

void cleanupSnake() {
  while (snakeHead != nullptr) {
    SnakeSegment* next = snakeHead->next;
    delete snakeHead;
    snakeHead = next;
  }
  snakeLength = 3;
  snakeGameOver = false;
}

void playSnake() {
  initSnake();
  
  while (!snakeGameOver) {
    if (shouldReturnToHome()) return;
    
    handleSnakeInput();
    updateSnake();
    checkSnakeCollision();
    drawSnakeGame();
    delay(150);
  }

  showGameOver("Snake", snakeScore);
  cleanupSnake();
}

// ======================
// PAC-MAN GAME (Complete)
// ======================

int pacmanX = 64, pacmanY = 64;
int pacmanDirX = 1, pacmanDirY = 0;
int ghostX[4], ghostY[4], ghostDirX[4], ghostDirY[4];
bool dots[16][16];
int pacmanScore = 0;
bool pacmanGameOver = false;

void initPacman() {
  pacmanX = 64; pacmanY = 64;
  pacmanDirX = 1; pacmanDirY = 0;
  pacmanScore = 0;
  pacmanGameOver = false;
  
  // Initialize dots
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      dots[x][y] = true;
    }
  }
  
  // Initialize ghosts
  ghostX[0] = 16; ghostY[0] = 16;
  ghostX[1] = 112; ghostY[1] = 16;
  ghostX[2] = 16; ghostY[2] = 112;
  ghostX[3] = 112; ghostY[3] = 112;
  
  for (int i = 0; i < 4; i++) {
    ghostDirX[i] = random(0, 2) ? 1 : -1;
    ghostDirY[i] = random(0, 2) ? 1 : -1;
  }
}

void handlePacmanInput() {
  if (digitalRead(JOY_UP) == LOW) {
    pacmanDirX = 0; pacmanDirY = -1;
  } else if (digitalRead(JOY_DWN) == LOW) {
    pacmanDirX = 0; pacmanDirY = 1;
  } else if (digitalRead(JOY_LFT) == LOW) {
    pacmanDirX = -1; pacmanDirY = 0;
  } else if (digitalRead(JOY_RHT) == LOW) {
    pacmanDirX = 1; pacmanDirY = 0;
  }
}

void updatePacman() {
  int newX = pacmanX + pacmanDirX * 4;
  int newY = pacmanY + pacmanDirY * 4;
  
  // Wrap around screen
  if (newX < 0) newX = tft.width() - 8;
  if (newX >= tft.width()) newX = 0;
  if (newY < 0) newY = tft.height() - 8;
  if (newY >= tft.height()) newY = 0;
  
  pacmanX = newX;
  pacmanY = newY;
  
  // Check if eating dot
  int gridX = pacmanX / 8;
  int gridY = pacmanY / 8;
  if (dots[gridX][gridY]) {
    dots[gridX][gridY] = false;
    pacmanScore += 10;
  }
}

void updateGhosts() {
  for (int i = 0; i < 4; i++) {
    ghostX[i] += ghostDirX[i] * 2;
    ghostY[i] += ghostDirY[i] * 2;
    
    // Bounce off walls
    if (ghostX[i] <= 0 || ghostX[i] >= tft.width() - 8) ghostDirX[i] *= -1;
    if (ghostY[i] <= 0 || ghostY[i] >= tft.height() - 8) ghostDirY[i] *= -1;
    
    // Random direction changes
    if (random(0, 20) == 0) {
      ghostDirX[i] = random(0, 2) ? 1 : -1;
      ghostDirY[i] = random(0, 2) ? 1 : -1;
    }
  }
}

void checkPacmanCollision() {
  for (int i = 0; i < 4; i++) {
    if (abs(pacmanX - ghostX[i]) < 8 && abs(pacmanY - ghostY[i]) < 8) {
      pacmanGameOver = true;
      return;
    }
  }
}

void drawPacmanGame() {
  tft.fillScreen(ST77XX_BLACK);
  
  // Draw dots
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      if (dots[x][y]) {
        tft.fillCircle(x * 8 + 4, y * 8 + 4, 2, ST77XX_WHITE);
      }
    }
  }
  
  // Draw Pac-Man
  tft.fillCircle(pacmanX, pacmanY, 4, ST77XX_YELLOW);
  
  // Draw ghosts
  tft.fillRect(ghostX[0], ghostY[0], 8, 8, ST77XX_RED);
  tft.fillRect(ghostX[1], ghostY[1], 8, 8, 0xF81F); // Pink
  tft.fillRect(ghostX[2], ghostY[2], 8, 8, ST77XX_CYAN);
  tft.fillRect(ghostX[3], ghostY[3], 8, 8, ST77XX_ORANGE);
  
  // Draw score
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(2, 2);
  tft.print("Score: ");
  tft.print(pacmanScore);
}

void playPacman() {
  initPacman();
  
  while (!pacmanGameOver) {
    if (shouldReturnToHome()) return;
    
    handlePacmanInput();
    updatePacman();
    updateGhosts();
    checkPacmanCollision();
    drawPacmanGame();
    delay(150);
  }
  
  showGameOver("Pac-Man", pacmanScore);
}

// ======================
// BREAKOUT GAME (Complete)
// ======================

int paddleX = 64;
int ballX = 64, ballY = 64;
int ballDirX = 1, ballDirY = 1;
bool bricks[8][5];
int breakoutScore = 0;
bool breakoutGameOver = false;

void initBreakout() {
  paddleX = 64;
  ballX = 64; ballY = 64;
  ballDirX = 1; ballDirY = 1;
  breakoutScore = 0;
  breakoutGameOver = false;
  
  // Initialize bricks
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 5; y++) {
      bricks[x][y] = true;
    }
  }
}

void handleBreakoutInput() {
  if (digitalRead(JOY_LFT) == LOW && paddleX > 0) {
    paddleX -= 4;
  }
  if (digitalRead(JOY_RHT) == LOW && paddleX < tft.width() - 16) {
    paddleX += 4;
  }
}

void updateBreakout() {
  // Move ball
  ballX += ballDirX * 2;
  ballY += ballDirY * 2;
  
  // Wall collision
  if (ballX <= 0 || ballX >= tft.width() - 4) ballDirX *= -1;
  if (ballY <= 0) ballDirY *= -1;
  
  // Paddle collision
  if (ballY >= tft.height() - 8 && 
      ballX >= paddleX && ballX <= paddleX + 16) {
    ballDirY = -1;
    ballDirX = (ballX - (paddleX + 8)) / 4;
  }
  
  // Bottom collision (game over)
  if (ballY >= tft.height()) {
    breakoutGameOver = true;
    return;
  }
  
  // Brick collision
  int brickX = ballX / 16;
  int brickY = ballY / 8;
  
  if (brickX >= 0 && brickX < 8 && brickY >= 0 && brickY < 5 && bricks[brickX][brickY]) {
    bricks[brickX][brickY] = false;
    ballDirY *= -1;
    breakoutScore += 10;
    
    // Check if all bricks cleared
    bool allCleared = true;
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 5; y++) {
        if (bricks[x][y]) {
          allCleared = false;
          break;
        }
      }
    }
    if (allCleared) {
      breakoutGameOver = true;
    }
  }
}

void drawBreakoutGame() {
  tft.fillScreen(ST77XX_BLACK);
  
  // Draw bricks
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 5; y++) {
      if (bricks[x][y]) {
        tft.fillRect(x * 16, y * 8, 16, 4, ST77XX_RED);
      }
    }
  }
  
  // Draw paddle
  tft.fillRect(paddleX, tft.height() - 4, 16, 4, ST77XX_WHITE);
  
  // Draw ball
  tft.fillRect(ballX, ballY, 4, 4, ST77XX_WHITE);
  
  // Draw score
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(2, 2);
  tft.print("Score: ");
  tft.print(breakoutScore);
}

void playBreakout() {
  initBreakout();
  
  while (!breakoutGameOver) {
    if (shouldReturnToHome()) return;
    
    handleBreakoutInput();
    updateBreakout();
    drawBreakoutGame();
    delay(50);
  }
  
  showGameOver("Breakout", breakoutScore);
}

// ======================
// SPACE INVADERS (Complete)
// ======================

int invaderX[5][10], invaderY[5][10];
bool invaderActive[5][10];
int playerX = 64;
int bulletX = -1, bulletY = -1;
int invaderBulletX = -1, invaderBulletY = -1;
int invadersScore = 0;
bool invadersGameOver = false;
int invaderDirection = 1;
unsigned long lastInvaderMove = 0;
const int invaderMoveDelay = 500;

void initInvaders() {
  playerX = 64;
  invadersScore = 0;
  invadersGameOver = false;
  
  // Initialize invaders grid
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 10; col++) {
      invaderX[row][col] = col * 12 + 2;
      invaderY[row][col] = row * 10 + 10;
      invaderActive[row][col] = true;
    }
  }
}

void handleInvadersInput() {
  if (digitalRead(JOY_LFT) == LOW && playerX > 0) {
    playerX -= 2;
  }
  if (digitalRead(JOY_RHT) == LOW && playerX < tft.width() - 8) {
    playerX += 2;
  }
  if (digitalRead(JOY_MID) == LOW && bulletY == -1) {
    bulletX = playerX + 4;
    bulletY = tft.height() - 16;
  }
}

void updateInvaders() {
  // Move invaders
  if (millis() - lastInvaderMove > invaderMoveDelay) {
    lastInvaderMove = millis();
    
    bool moveDown = false;
    for (int row = 0; row < 5; row++) {
      for (int col = 0; col < 10; col++) {
        if (invaderActive[row][col]) {
          invaderX[row][col] += invaderDirection;
          
          // Check if any invader hit edge
          if (invaderX[row][col] <= 0 || invaderX[row][col] >= tft.width() - 8) {
            moveDown = true;
          }
        }
      }
    }
    
    if (moveDown) {
      invaderDirection *= -1;
      for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 10; col++) {
          if (invaderActive[row][col]) {
            invaderY[row][col] += 5;
            
            // Check if invaders reached bottom
            if (invaderY[row][col] >= tft.height() - 16) {
              invadersGameOver = true;
              return;
            }
          }
        }
      }
    }
  }
  
  // Move player bullet
  if (bulletY != -1) {
    bulletY -= 4;
    
    // Check if bullet hit top
    if (bulletY < 0) {
      bulletY = -1;
    } else {
      // Check for collision with invaders
      for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 10; col++) {
          if (invaderActive[row][col] && 
              bulletX >= invaderX[row][col] && 
              bulletX <= invaderX[row][col] + 8 &&
              bulletY >= invaderY[row][col] && 
              bulletY <= invaderY[row][col] + 8) {
            invaderActive[row][col] = false;
            bulletY = -1;
            invadersScore += 10;
            
            // Check if all invaders destroyed
            bool allDestroyed = true;
            for (int r = 0; r < 5; r++) {
              for (int c = 0; c < 10; c++) {
                if (invaderActive[r][c]) {
                  allDestroyed = false;
                  break;
                }
              }
            }
            if (allDestroyed) {
              invadersGameOver = true;
            }
            return;
          }
        }
      }
    }
  }
  
  // Invader shooting
  if (invaderBulletY == -1 && random(0, 50) == 0) {
    // Find a random active invader to shoot from
    for (int attempt = 0; attempt < 10; attempt++) {
      int row = random(0, 5);
      int col = random(0, 10);
      if (invaderActive[row][col]) {
        invaderBulletX = invaderX[row][col] + 4;
        invaderBulletY = invaderY[row][col] + 8;
        break;
      }
    }
  }
  
  // Move invader bullet
  if (invaderBulletY != -1) {
    invaderBulletY += 3;
    
    // Check if bullet hit bottom
    if (invaderBulletY >= tft.height()) {
      invaderBulletY = -1;
    } else if (invaderBulletY >= tft.height() - 8 && 
               invaderBulletX >= playerX && 
               invaderBulletX <= playerX + 8) {
      // Hit player
      invadersGameOver = true;
    }
  }
}

void drawInvadersGame() {
  tft.fillScreen(ST77XX_BLACK);
  
  // Draw invaders
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 10; col++) {
      if (invaderActive[row][col]) {
        tft.fillRect(invaderX[row][col], invaderY[row][col], 8, 8, ST77XX_GREEN);
      }
    }
  }
  
  // Draw player
  tft.fillRect(playerX, tft.height() - 8, 8, 8, ST77XX_WHITE);
  
  // Draw bullets
  if (bulletY != -1) {
    tft.fillRect(bulletX, bulletY, 2, 4, ST77XX_WHITE);
  }
  if (invaderBulletY != -1) {
    tft.fillRect(invaderBulletX, invaderBulletY, 2, 4, ST77XX_RED);
  }
  
  // Draw score
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(2, 2);
  tft.print("Score: ");
  tft.print(invadersScore);
}

void playInvaders() {
  initInvaders();
  
  while (!invadersGameOver) {
    if (shouldReturnToHome()) return;
    
    handleInvadersInput();
    updateInvaders();
    drawInvadersGame();
    delay(50);
  }
  
  showGameOver("Invaders", invadersScore);
}

// ======================
// POKEYMON GAME (Complete)
// ======================

int pokemonX = 64, pokemonY = 64;
int pokemonDirX = 0, pokemonDirY = 0;
int enemyX[3], enemyY[3];
int pokemonScore = 0;
bool pokemonGameOver = false;

void initPokeymon() {
  pokemonX = 64; pokemonY = 64;
  pokemonScore = 0;
  pokemonGameOver = false;
  
  // Initialize enemies
  for (int i = 0; i < 3; i++) {
    enemyX[i] = random(8, tft.width() - 8);
    enemyY[i] = random(8, tft.height() - 8);
  }
}

void handlePokeymonInput() {
  pokemonDirX = 0;
  pokemonDirY = 0;
  
  if (digitalRead(JOY_UP) == LOW) pokemonDirY = -1;
  if (digitalRead(JOY_DWN) == LOW) pokemonDirY = 1;
  if (digitalRead(JOY_LFT) == LOW) pokemonDirX = -1;
  if (digitalRead(JOY_RHT) == LOW) pokemonDirX = 1;
}

void updatePokeymon() {
  // Move player
  pokemonX += pokemonDirX * 2;
  pokemonY += pokemonDirY * 2;
  
  // Keep player on screen
  if (pokemonX < 0) pokemonX = 0;
  if (pokemonX >= tft.width()) pokemonX = tft.width() - 1;
  if (pokemonY < 0) pokemonY = 0;
  if (pokemonY >= tft.height()) pokemonY = tft.height() - 1;
  
  // Move enemies
  for (int i = 0; i < 3; i++) {
    // Simple AI: move toward player
    if (enemyX[i] < pokemonX) enemyX[i]++;
    if (enemyX[i] > pokemonX) enemyX[i]--;
    if (enemyY[i] < pokemonY) enemyY[i]++;
    if (enemyY[i] > pokemonY) enemyY[i]--;
    
    // Check collision with player
    if (abs(pokemonX - enemyX[i]) < 8 && abs(pokemonY - enemyY[i]) < 8) {
      pokemonGameOver = true;
      return;
    }
  }
  
  // Spawn new enemies occasionally
  if (random(0, 100) == 0 && pokemonScore > 0) {
    pokemonScore--;
    for (int i = 0; i < 3; i++) {
      if (random(0, 2) == 0) {
        enemyX[i] = random(8, tft.width() - 8);
        enemyY[i] = random(8, tft.height() - 8);
      }
    }
  }
  
  // Check for "collecting" enemies (with MID button)
  if (digitalRead(JOY_MID) == LOW) {
    for (int i = 0; i < 3; i++) {
      if (abs(pokemonX - enemyX[i]) < 16 && abs(pokemonY - enemyY[i]) < 16) {
        enemyX[i] = random(8, tft.width() - 8);
        enemyY[i] = random(8, tft.height() - 8);
        pokemonScore += 5;
      }
    }
  }
}

void drawPokeymonGame() {
  tft.fillScreen(ST77XX_BLACK);
  
  // Draw player
  tft.fillCircle(pokemonX, pokemonY, 4, ST77XX_YELLOW);
  
  // Draw enemies
  tft.fillCircle(enemyX[0], enemyY[0], 4, ST77XX_RED);
  tft.fillCircle(enemyX[1], enemyY[1], 4, ST77XX_BLUE);
  tft.fillCircle(enemyX[2], enemyY[2], 4, ST77XX_GREEN);
  
  // Draw score
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(2, 2);
  tft.print("Score: ");
  tft.print(pokemonScore);
}

void playPokeymon() {
  initPokeymon();
  
  while (!pokemonGameOver) {
    if (shouldReturnToHome()) return;
    
    handlePokeymonInput();
    updatePokeymon();
    drawPokeymonGame();
    delay(100);
  }
  
  showGameOver("Pokeymon", pokemonScore);
}

// ======================
// MEGAMINI GAME (Complete)
// ======================

int megaX = 16, megaY = 64;
int megaDirX = 0, megaDirY = 0;
bool megaJumping = false;
int megaScore = 0;
bool megaGameOver = false;
int platforms[5][4]; // x, y, width, active

void initMegaMini() {
  megaX = 16; megaY = 64;
  megaScore = 0;
  megaGameOver = false;
  
  // Initialize platforms
  platforms[0][0] = 0; platforms[0][1] = 80; platforms[0][2] = 40; platforms[0][3] = true;
  platforms[1][0] = 40; platforms[1][1] = 60; platforms[1][2] = 30; platforms[1][3] = true;
  platforms[2][0] = 80; platforms[2][1] = 70; platforms[2][2] = 30; platforms[2][3] = true;
  platforms[3][0] = 120; platforms[3][1] = 50; platforms[3][2] = 30; platforms[3][3] = true;
  platforms[4][0] = 160; platforms[4][1] = 90; platforms[4][2] = 40; platforms[4][3] = true;
}

void handleMegaMiniInput() {
  megaDirX = 0;
  
  if (digitalRead(JOY_LFT) == LOW) megaDirX = -1;
  if (digitalRead(JOY_RHT) == LOW) megaDirX = 1;
  
  // Jump (only if not already jumping)
  if (digitalRead(JOY_UP) == LOW && !megaJumping) {
    megaJumping = true;
    megaDirY = -3;
  }
}

void updateMegaMini() {
  // Move horizontally
  megaX += megaDirX * 2;
  
  // Apply gravity
  if (megaJumping) {
    megaY += megaDirY;
    megaDirY += 0.5; // Gravity
    
    // Check for platform landing
    for (int i = 0; i < 5; i++) {
      if (platforms[i][3] && 
          megaX + 8 >= platforms[i][0] && 
          megaX <= platforms[i][0] + platforms[i][2] &&
          megaY + 8 >= platforms[i][1] && 
          megaY + 8 <= platforms[i][1] + 4) {
        megaJumping = false;
        megaY = platforms[i][1] - 8;
        megaScore += 10;
        break;
      }
    }
    
    // Check if fell off bottom
    if (megaY > tft.height()) {
      megaGameOver = true;
    }
  }
  
  // Scroll platforms if player reaches right side
  if (megaX > tft.width() - 16) {
    megaX = tft.width() - 16;
    for (int i = 0; i < 5; i++) {
      platforms[i][0] -= 2;
      // If platform goes off left side, recycle it to the right
      if (platforms[i][0] + platforms[i][2] < 0) {
        platforms[i][0] = tft.width();
        platforms[i][1] = random(40, tft.height() - 20);
        platforms[i][2] = random(20, 50);
        platforms[i][3] = true;
      }
    }
  }
  
  // Don't let player go off left side
  if (megaX < 0) megaX = 0;
}

void drawMegaMiniGame() {
  tft.fillScreen(ST77XX_BLACK);
  
  // Draw platforms
  for (int i = 0; i < 5; i++) {
    if (platforms[i][3]) {
      tft.fillRect(platforms[i][0], platforms[i][1], platforms[i][2], 4, ST77XX_GREEN);
    }
  }
  
  // Draw player
  tft.fillRect(megaX, megaY, 8, 8, ST77XX_BLUE);
  
  // Draw score
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(2, 2);
  tft.print("Score: ");
  tft.print(megaScore);
}

void playMegaMini() {
  initMegaMini();
  
  while (!megaGameOver) {
    if (shouldReturnToHome()) return;
    
    handleMegaMiniInput();
    updateMegaMini();
    drawMegaMiniGame();
    delay(50);
  }
  
  showGameOver("MegaMini", megaScore);
}

// ======================
// SETUP AND LOOP
// ======================

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);

  pinMode(JOY_UP, INPUT_PULLUP);
  pinMode(JOY_DWN, INPUT_PULLUP);
  pinMode(JOY_LFT, INPUT_PULLUP);
  pinMode(JOY_RHT, INPUT_PULLUP);
  pinMode(JOY_MID, INPUT_PULLUP);
  pinMode(JOY_SET, INPUT_PULLUP);
  pinMode(JOY_RST, INPUT_PULLUP);

  randomSeed(analogRead(0));
}

void loop() {
  switch (currentState) {
    case STATE_MENU:
      drawMenu();
      handleMenuInput();
      break;
    case STATE_SNAKE:
      playSnake();
      break;
    case STATE_PACMAN:
      playPacman();
      break;
    case STATE_BREAKOUT:
      playBreakout();
      break;
    case STATE_INVADERS:
      playInvaders();
      break;
    case STATE_POKEYMON:
      playPokeymon();
      break;
    case STATE_MEGAMINI:
      playMegaMini();
      break;
  }
}