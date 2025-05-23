#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "ppm_image.h"
#include "input.h"

#define SHIP_SPEED 3        // Pixels per knob rotation unit
#define BOTTOM_PADDING 30   // Padding at bottom of screen
#define GAME_BOUNDARY_Y (LCD_HEIGHT - BOTTOM_PADDING)
#define LCD_WIDTH 480
#define LCD_HEIGHT 320

// Bullet
#define MAX_BULLETS 1      // Maximum number of bullets
#define BULLET_SPEED 5      // Pixels per frame
#define BULLET_WIDTH 2      // Width of bullet
#define BULLET_HEIGHT 10    // Height of bullet
#define BULLET_COLOR 0xFFE0 // Yellow

// Enemy bullets
#define MAX_ENEMY_BULLETS 5
#define ENEMY_BULLET_SPEED 3
#define ENEMY_BULLET_WIDTH 2
#define ENEMY_BULLET_HEIGHT 10
#define ENEMY_BULLET_COLOR 0xF800  // Red

// Enemy
#define MAX_ENEMY_ROWS 5
#define MAX_ENEMY_COLS 10
#define ENEMY_WIDTH 32
#define ENEMY_HEIGHT 32
#define ENEMY_SPACING_X 10
#define ENEMY_SPACING_Y 0
#define ENEMY_MOVE_SPEED 1
#define ENEMY_MOVE_INTERVAL 500 // ms between movements

// Mystery ship
#define MYSTERY_SHIP_WIDTH 50
#define MYSTERY_SHIP_HEIGHT 30
#define MYSTERY_SHIP_SPEED 3
#define MYSTERY_SHIP_POINTS 100

// Bullet structure
typedef struct {
    int x;          // X position
    int y;          // Y position
    bool active;    // Whether bullet is active
} Bullet;

// Enemy structure
typedef struct {
    int x;        // X position
    int y;        // Y position
    bool alive;   // Whether enemy is alive
    int type;     // Enemy type (0-2). As in classical game, depending on the rows
} Enemy;

// Mystery ship structure
typedef struct {
    int x;
    int y;
    bool active;
    int direction; // 1 = right, -1 = left
} MysteryShip;

// Game state structure
typedef struct {
    PPMImage* shipSprite[2];   // Ship sprite
    int shipX[2];              // Ship X position
    int shipY[2];              // Ship Y position (fixed)
    int shipWidth;          // Width of ship sprite when rendered
    int shipHeight;         // Height of ship sprite when rendered
    float shipScale;        // Scale factor for ship

    Bullet bullets[2][MAX_BULLETS]; // Array of bullets
    int lastShotTime[2];            // Time of last shot (for rate limiting)

    // Enemy bullets
    Bullet enemyBullets[MAX_ENEMY_BULLETS];
    int lastEnemyShot;           // Time of last enemy shot

    // Enemy data
    Enemy enemies[MAX_ENEMY_ROWS][MAX_ENEMY_COLS];
    PPMImage* enemySprites[3];   // 3 different enemy sprites
    int enemyDirection;          // Current direction (1=right, -1=left)
    int enemyCount;              // Number of enemies alive
    uint64_t lastEnemyMove;      // Time of last enemy movement

    // Mystery ship
    MysteryShip mysteryShip;
    PPMImage* mysteryShipSprite;

    // Game progression
    bool gameOver;          // Game over flag
    int level;
    int lives[2];
    int score[2];
    bool isMultiplayer; // Multiplayer mode
} GameState;

// Initialize the game
bool initGame(GameState* game, MemoryMap* memMap, bool multiplayer);
// Update game state based on input
void updateGame(GameState* game, MemoryMap* memMap);
// Render the game to the framebuffer
void renderGame(GameState* game, unsigned short* fb, unsigned char* parlcd_mem_base);
// Free resources when game is done
void cleanupGame(GameState* game);
// Check if enemies should change direction
bool shouldChangeDirection(GameState* game);
// Move all enemies down one row
void moveEnemiesDown(GameState* game);
// Fire player bullet
void fireBullet(GameState* game, int playerIndex);

#endif // GAME_H