#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "game.h"
#include "graphics.h"
#include "input.h"
#include "font_types.h"

// GLOBAL FONT VALUE
extern font_descriptor_t font_winFreeSystem14x16;

extern uint64_t get_time_ms();

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
        sprintf(filename, "sprites/batman.ppm", i);
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

    return true;
}

// Create a new bullet at the ship's position
void fireBullet(GameState* game) {
    // Limit fire rate (250ms between shots, no spamming)
    uint64_t currentTime = get_time_ms();
    if (currentTime - game->lastShotTime < 250) {
        return;
    }

    // Find an inactive bullet
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!game->bullets[i].active) {
            // Position bullet at top center of ship
            game->bullets[i].x = game->shipX + (game->shipWidth / 2) - (BULLET_WIDTH / 2);
            game->bullets[i].y = game->shipY - BULLET_HEIGHT;
            game->bullets[i].active = true;

            game->lastShotTime = currentTime;
            return;
        }
    }
}

// Initialize the enemy formation
void initEnemies(GameState* game) {
    int startX = (LCD_WIDTH - ((MAX_ENEMY_COLS * ENEMY_WIDTH) +
                              ((MAX_ENEMY_COLS - 1) * ENEMY_SPACING_X))) / 2;
    int startY = 60;

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

// Check if enemies need to change direction
bool shouldChangeDirection(GameState* game) {
    for (int row = 0; row < MAX_ENEMY_ROWS; row++) {
        for (int col = 0; col < MAX_ENEMY_COLS; col++) {
            if (game->enemies[row][col].alive) {
                // Check right edge
                if (game->enemyDirection > 0 &&
                    game->enemies[row][col].x + ENEMY_WIDTH >= LCD_WIDTH) {
                    return true;
                }
                // Check left edge
                if (game->enemyDirection < 0 &&
                    game->enemies[row][col].x <= 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Move all enemies down one row
void moveEnemiesDown(GameState* game) {
    for (int row = 0; row < MAX_ENEMY_ROWS; row++) {
        for (int col = 0; col < MAX_ENEMY_COLS; col++) {
            if (game->enemies[row][col].alive) {
                game->enemies[row][col].y += ENEMY_HEIGHT / 2;

                // Check if enemies reached the boundary
                if (game->enemies[row][col].y + ENEMY_HEIGHT >= GAME_BOUNDARY_Y) {
                    game->gameOver = true;
                }
            }
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

    // Update bullet positions
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game->bullets[i].active) {
            // Move bullet upward
            game->bullets[i].y -= BULLET_SPEED;

            // Deactivate bullet if it goes off screen
            if (game->bullets[i].y + BULLET_HEIGHT < 0) {
                game->bullets[i].active = false;
            }
        }
    }

    // Update enemy movement
    uint64_t currentTime = get_time_ms();
    if (currentTime - game->lastEnemyMove > ENEMY_MOVE_INTERVAL) {
        // Check if enemies need to change direction
        if (shouldChangeDirection(game)) {
            game->enemyDirection *= -1; // Reverse direction
            moveEnemiesDown(game);      // Move down
        } else {
            // Move enemies horizontally
            for (int row = 0; row < MAX_ENEMY_ROWS; row++) {
                for (int col = 0; col < MAX_ENEMY_COLS; col++) {
                    if (game->enemies[row][col].alive) {
                        game->enemies[row][col].x += game->enemyDirection * ENEMY_MOVE_SPEED;
                    }
                }
            }
        }
        game->lastEnemyMove = currentTime;
    }

    // Update mystery ship
    if (game->mysteryShip.active) {
        game->mysteryShip.x += game->mysteryShip.direction * MYSTERY_SHIP_SPEED;

        // If mystery ship goes off screen, reset to opposite side
        if (game->mysteryShip.x > LCD_WIDTH) {
            game->mysteryShip.x = -MYSTERY_SHIP_WIDTH;
        } else if (game->mysteryShip.x + MYSTERY_SHIP_WIDTH < 0) {
            game->mysteryShip.x = LCD_WIDTH;
        }

        // Randomly change direction or deactivate
        if (rand() % 500 == 0) {
            game->mysteryShip.direction *= -1;
        }

        // Randomly deactivate mystery ship
        if (rand() % 1000 == 0) {
            game->mysteryShip.active = false;
        }
    } else if (rand() % 500 == 0) { // Randomly activate inactive mystery ship
        game->mysteryShip.active = true;
        // Start from either left or right side
        if (rand() % 2 == 0) {
            game->mysteryShip.x = -MYSTERY_SHIP_WIDTH;
            game->mysteryShip.direction = 1;
        } else {
            game->mysteryShip.x = LCD_WIDTH;
            game->mysteryShip.direction = -1;
        }
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

    // Draw ship directly to framebuffer
    for (int y = 0; y < game->shipHeight; y++) {
        for (int x = 0; x < game->shipWidth; x++) {
            int srcX = x / game->shipScale;
            int srcY = y / game->shipScale;

            if (srcX < game->shipSprite->width && srcY < game->shipSprite->height) {
                uint16_t color = game->shipSprite->pixels[srcY * game->shipSprite->width + srcX];

                // Skip transparent pixels
                if (color != 0x0000) {
                    drawPixel(fb, game->shipX + x, game->shipY + y, color);
                }
            }
        }
    }

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

    // Draw enemies
    for (int row = 0; row < MAX_ENEMY_ROWS; row++) {
        for (int col = 0; col < MAX_ENEMY_COLS; col++) {
            if (game->enemies[row][col].alive) {
                // Get enemy sprite based on type
                PPMImage* sprite = game->enemySprites[game->enemies[row][col].type];

                // Draw enemy sprite
                for (int y = 0; y < ENEMY_HEIGHT; y++) {
                    for (int x = 0; x < ENEMY_WIDTH; x++) {
                        int srcX = x * sprite->width / ENEMY_WIDTH;
                        int srcY = y * sprite->height / ENEMY_HEIGHT;

                        if (srcX < sprite->width && srcY < sprite->height) {
                            uint16_t color = sprite->pixels[srcY * sprite->width + srcX];

                            // Skip transparent pixels
                            if (color != 0x0000) {
                                drawPixel(fb,
                                          game->enemies[row][col].x + x,
                                          game->enemies[row][col].y + y,
                                          color);
                            }
                        }
                    }
                }
            }
        }
    }

    // Draw mystery ship if active
    if (game->mysteryShip.active) {
        for (int y = 0; y < ENEMY_HEIGHT; y++) {
            for (int x = 0; x < ENEMY_WIDTH * 2; x++) {
                int srcX = x * game->mysteryShipSprite->width / (ENEMY_WIDTH * 2);
                int srcY = y * game->mysteryShipSprite->height / ENEMY_HEIGHT;

                if (srcX < game->mysteryShipSprite->width && srcY < game->mysteryShipSprite->height) {
                    uint16_t color = game->mysteryShipSprite->pixels[srcY * game->mysteryShipSprite->width + srcX];

                    // Skip transparent pixels
                    if (color != 0x0000) {
                        drawPixel(fb, game->mysteryShip.x + x, 40 + y, color);
                    }
                }
            }
        }
    }

    // Draw score/lives in bottom area
    drawString(fb, 10, GAME_BOUNDARY_Y + 10, "SCORE: 0", &font_winFreeSystem14x16, 0xFFFF, 1);
    drawString(fb, LCD_WIDTH - 100, GAME_BOUNDARY_Y + 10, "LIVES: 3", &font_winFreeSystem14x16, 0xFFFF, 1);

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