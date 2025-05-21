#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "game.h"
#include "graphics.h"
#include "input.h"
#include "font_types.h"
#include "game_utils.h"

// GLOBAL FONT VALUE
extern font_descriptor_t font_winFreeSystem14x16;

extern uint64_t get_time_ms();

void initEnemies(GameState* game);

bool initGame(GameState* game, MemoryMap* memMap) {
    if (!game) return false;
    // Initialize input system
    inputInit(memMap);

    // Load ship sprite
    game->shipSprite = read_ppm("sprites/harley_quinn.ppm");
    if (!game->shipSprite) {
        printf("Failed to load ship sprite\n");
        return false;
    }

    // Set ship initial position and parameters
    game->shipScale = 3.0f;
    game->shipWidth = game->shipSprite->width * game->shipScale;
    game->shipHeight = game->shipSprite->height * game->shipScale;

    // Position ship at bottom center of screen
    game->shipX = (LCD_WIDTH - game->shipWidth) / 2;
    game->shipY = GAME_BOUNDARY_Y - game->shipHeight;

    // Initialize bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        game->bullets[i].active = false;
    }
    game->lastShotTime = 0;

    // Load enemy sprites
    for (int i = 0; i < 3; i++) {
        char filename[32];
        sprintf(filename, "sprites/spiderman.ppm");
        game->enemySprites[i] = read_ppm(filename);
        if (!game->enemySprites[i]) {
            printf("Failed to load enemy sprite!\n");
            return false;
        }
    }

    // Load mystery ship sprite
    game->mysteryShipSprite = read_ppm("sprites/poison_ivy.ppm");
    if (!game->mysteryShipSprite) {
        printf("Failed to load mystery ship sprite!\n");
        return false;
    }

    // Initialize mystery ship and enemies
    game->mysteryShip.active = true; // Start with active mystery ship
    game->mysteryShip.direction = 1; // Move right initially
    game->mysteryShip.x = 0;         // Start at left edge
    initEnemies(game);
    game->lastEnemyMove = get_time_ms();

    // Game state
    game->gameOver = false;
    game->level = 1;
    game->score = 0;
    game->lives = 3;

    return true;
}

// Initialize the enemy formation
void initEnemies(GameState* game) {
    int startX = (LCD_WIDTH - ((MAX_ENEMY_COLS * ENEMY_WIDTH) +
                              ((MAX_ENEMY_COLS - 1) * ENEMY_SPACING_X))) / 2;
    int startY = 40;

    game->enemyCount = 0;
    game->enemyDirection = 1;

    for (int row = 0; row < MAX_ENEMY_ROWS; row++) {
        for (int col = 0; col < MAX_ENEMY_COLS; col++) {
            game->enemies[row][col].x = startX + (col * (ENEMY_WIDTH + ENEMY_SPACING_X));
            game->enemies[row][col].y = startY + (row * (ENEMY_HEIGHT + ENEMY_SPACING_Y));
            game->enemies[row][col].alive = true;
            game->enemies[row][col].type = row / 2; // Enemy type based on row
            game->enemyCount++;
        }
    }
}

void updateGame(GameState* game, MemoryMap* memMap) {
    if (!game) return;

    // Get knob rotation for horizontal movement
    int moveX = getKnobRotation(RED_KNOB) * SHIP_SPEED;

    // Update ship position
    game->shipX += moveX;

    // Keep ship within screen boundaries
    if (game->shipX < 0) {
        game->shipX = 0;
    }
    if (game->shipX > LCD_WIDTH - game->shipWidth) {
        game->shipX = LCD_WIDTH - game->shipWidth;
    }

    // Process shooting (RED_KNOB button)
    if (isButtonPressed(RED_KNOB)) {
        fireBullet(game);
    }

    // Update player bullets and bullet collisions
    updatePlayerBullets(game);

    // Update enemy bullets
    updateEnemyBullets(game);

    // Update enemy formation movement
    updateEnemyFormation(game);

    // Update mystery ship
    updateMysteryShip(game);

    // Check if all enemies are destroyed - level complete
    if (game->enemyCount <= 0) {
        game->level++;
        initEnemies(game);
    }
}

void renderGame(GameState* game, unsigned short* fb, unsigned char* parlcd_mem_base) {
    if (!game) return;

    // Clear the screen
    clearScreen(fb, 0x7010); // Dark purple background

    // Draw boundary line
    for (int x = 0; x < LCD_WIDTH; x++) {
        drawPixel(fb, x, GAME_BOUNDARY_Y, 0xFFFF); // White line
    }

    // Draw ship
    draw_sprite(fb, game->shipSprite, game->shipX, game->shipY,
                game->shipWidth, game->shipHeight, 0x0000);


    // Draw bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game->bullets[i].active) {
            for (int y = 0; y < BULLET_HEIGHT; y++) {
                for (int x = 0; x < BULLET_WIDTH; x++) {
                    drawPixel(fb, game->bullets[i].x + x, game->bullets[i].y + y, BULLET_COLOR);
                }
            }
        }
    }

    // Draw enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (game->enemyBullets[i].active) {
            for (int y = 0; y < BULLET_HEIGHT; y++) {
                for (int x = 0; x < BULLET_WIDTH; x++) {
                    drawPixel(fb, game->enemyBullets[i].x + x,
                             game->enemyBullets[i].y + y,
                             0xF800); // Red color for enemy bullets
                }
            }
        }
    }

    // Draw enemies
    for (int row = 0; row < MAX_ENEMY_ROWS; row++) {
        for (int col = 0; col < MAX_ENEMY_COLS; col++) {
            if (game->enemies[row][col].alive) {
                draw_sprite(fb, game->enemySprites[game->enemies[row][col].type],
                          game->enemies[row][col].x, game->enemies[row][col].y,
                          ENEMY_WIDTH, ENEMY_HEIGHT, 0x0000);
            }
        }
    }

    // Draw mystery ship if active
    if (game->mysteryShip.active) {
        draw_sprite(fb, game->mysteryShipSprite, game->mysteryShip.x, 5,
                  MYSTERY_SHIP_WIDTH, MYSTERY_SHIP_HEIGHT, 0x0000);
    }

    // Draw score/lives in bottom area
    char scoreText[32];
    char livesText[32];
    char levelText[32];
    sprintf(scoreText, "SCORE: %d", game->score);
    sprintf(livesText, "LIVES: %d", game->lives);
    sprintf(levelText, "LEVEL: %d", game->level);

    drawString(fb, 10, GAME_BOUNDARY_Y + 10, scoreText, &font_winFreeSystem14x16, 0xFFFF, 1);
    drawString(fb, 20 + stringWidth(scoreText, &font_winFreeSystem14x16, 1), GAME_BOUNDARY_Y + 10, livesText, &font_winFreeSystem14x16, 0xFFFF, 1);
    drawString(fb, 30 + stringWidth(scoreText, &font_winFreeSystem14x16, 1) + stringWidth(livesText, &font_winFreeSystem14x16, 1), GAME_BOUNDARY_Y + 10, levelText, &font_winFreeSystem14x16, 0xFFFF, 1);

    // Update display
    updateDisplay(parlcd_mem_base, fb);
}

void cleanupGame(GameState* game) {
    if (!game) return;

    // Free ship sprite
    if (game->shipSprite) {
        free_ppm(game->shipSprite);
        game->shipSprite = NULL;
    }

    // Free enemy sprites
    for (int i = 0; i < 3; i++) {
        if (game->enemySprites[i]) {
            free_ppm(game->enemySprites[i]);
            game->enemySprites[i] = NULL;
        }
    }

    // Free mystery ship sprite
    if (game->mysteryShipSprite) {
        free_ppm(game->mysteryShipSprite);
        game->mysteryShipSprite = NULL;
    }
}