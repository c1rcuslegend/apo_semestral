#include <stdlib.h>
#include <stdbool.h>
#include "game_utils.h"
#include "graphics.h"
#include "game.h"

// External time function
extern uint64_t get_time_ms();

// Check if two rectangles collide
bool checkCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

// Update player bullets (movement and collision)
void updatePlayerBullets(GameState* game, MemoryMap* memMap) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game->bullets[i].active) {
            // Move bullet upward
            game->bullets[i].y -= BULLET_SPEED;

            // Check collisions with enemies
            for (int row = 0; row < MAX_ENEMY_ROWS; row++) {
                for (int col = 0; col < MAX_ENEMY_COLS; col++) {
                    if (game->enemies[row][col].alive) {
                        if (checkCollision(
                                game->bullets[i].x, game->bullets[i].y,
                                BULLET_WIDTH, BULLET_HEIGHT,
                                game->enemies[row][col].x, game->enemies[row][col].y,
                                ENEMY_WIDTH, ENEMY_HEIGHT)) {

                            // Kill enemy
                            game->enemies[row][col].alive = false;
                            game->enemyCount--;

                            // Award points based on row
                            int points;
                            if (row == 0) {
                                points = 30;  // Top row
                            } else if (row < 3) {
                                points = 20;  // Middle rows
                            } else {
                                points = 10;  // Bottom rows
                            }
                            updateScore(game, points);

                            flashEnemyKillLED(&memMap, 0xFF00);

                            // Deactivate bullet
                            game->bullets[i].active = false;
                            break;
                        }
                    }
                }
                if (!game->bullets[i].active) break;
            }

            // Check collision with mystery ship
            if (game->bullets[i].active && game->mysteryShip.active) {
                if (checkCollision(
                        game->bullets[i].x, game->bullets[i].y,
                        BULLET_WIDTH, BULLET_HEIGHT,
                        game->mysteryShip.x, game->mysteryShip.y,
                        MYSTERY_SHIP_WIDTH, MYSTERY_SHIP_HEIGHT)) {

                    // Kill mystery ship
                    game->mysteryShip.active = false;

                    // Award bonus points
                    updateScore(game, MYSTERY_SHIP_POINTS);

                    flashEnemyKillLED(&memMap, 0xFFE0);

                    // Deactivate bullet
                    game->bullets[i].active = false;
                }
            }

            // Deactivate bullet if it goes off screen
            if (game->bullets[i].active && game->bullets[i].y + BULLET_HEIGHT < 0) {
                game->bullets[i].active = false;
            }
        }
    }
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

// Fire an enemy bullet
void fireEnemyBullet(GameState* game, int enemyX, int enemyY) {
    uint64_t currentTime = get_time_ms();

    // Rate limit enemy shots (300ms between shots)
    if (currentTime - game->lastEnemyShot < 300) {
        return;
    }

    // Find an inactive enemy bullet
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!game->enemyBullets[i].active) {
            // Position bullet at bottom center of enemy
            game->enemyBullets[i].x = enemyX + (ENEMY_WIDTH / 2) - (ENEMY_BULLET_WIDTH / 2);
            game->enemyBullets[i].y = enemyY + ENEMY_HEIGHT;
            game->enemyBullets[i].active = true;

            game->lastEnemyShot = currentTime;
            return;
        }
    }
}

// Update enemy bullets (movement and collision)
void updateEnemyBullets(GameState* game) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (game->enemyBullets[i].active) {
            // Move bullet downward
            game->enemyBullets[i].y += ENEMY_BULLET_SPEED;

            // Check if bullet reached bottom boundary (not the screen bottom)
            if (game->enemyBullets[i].y >= GAME_BOUNDARY_Y) {
                game->enemyBullets[i].active = false;
                continue;
            }

            // Check collision with player
            if (checkCollision(
                    game->enemyBullets[i].x, game->enemyBullets[i].y,
                    ENEMY_BULLET_WIDTH, ENEMY_BULLET_HEIGHT,
                    game->shipX, game->shipY,
                    game->shipWidth, game->shipHeight)) {

                // Reduce player lives
                game->lives--;

                // Check game over
                if (game->lives <= 0) {
                    game->gameOver = true;
                }

                // Deactivate bullet
                game->enemyBullets[i].active = false;
            }

            // Deactivate bullet if it goes off screen
            if (game->enemyBullets[i].y > GAME_BOUNDARY_Y) {
                game->enemyBullets[i].active = false;
            }
        }
    }
}

// Check if an enemy can shoot (no other enemies in front)
bool canEnemyShoot(GameState* game, int row, int col) {
    // Check if there's any enemy below this one
    for (int r = row + 1; r < MAX_ENEMY_ROWS; r++) {
        if (game->enemies[r][col].alive) {
            return false;
        }
    }
    return true;
}

// Update enemy formation movement
void updateEnemyFormation(GameState* game) {
    uint64_t currentTime = get_time_ms();

    // Move enemies at regular intervals
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

        // Try enemy shooting (only bottom enemies in column can shoot)
        for (int col = 0; col < MAX_ENEMY_COLS; col++) {
            for (int row = MAX_ENEMY_ROWS - 1; row >= 0; row--) {
                if (game->enemies[row][col].alive && canEnemyShoot(game, row, col)) {
                    // Random chance to fire (5%)
                    if (rand() % 100 < 5) {
                        fireEnemyBullet(game,
                                       game->enemies[row][col].x,
                                       game->enemies[row][col].y);
                    }
                    break; // Only check the bottom enemy in each column
                }
            }
        }

        bool gameOverCondition = false;

        // Check if any enemy has reached player level
        for (int row = 0; row < MAX_ENEMY_ROWS; row++) {
            for (int col = 0; col < MAX_ENEMY_COLS; col++) {
                if (game->enemies[row][col].alive) {
                    if (game->enemies[row][col].y + ENEMY_HEIGHT >= GAME_BOUNDARY_Y - game->shipHeight) {
                        gameOverCondition = true;
                        break;
                    }
                }
            }
            if (gameOverCondition) break;
        }

        if (gameOverCondition) {
            game->lives = 0;  // Remove all lives
            game->gameOver = true;
        }

    }
}

// Update mystery ship
void updateMysteryShip(GameState* game) {
    if (game->mysteryShip.active) {
        game->mysteryShip.x += game->mysteryShip.direction * MYSTERY_SHIP_SPEED;

        // If mystery ship goes off screen, deactivate it
        if (game->mysteryShip.x > LCD_WIDTH ||
            game->mysteryShip.x + MYSTERY_SHIP_WIDTH < 0) {
            game->mysteryShip.active = false;
        }
    }
    // Randomly activate mystery ship
    else if (rand() % 500 == 0) {
        game->mysteryShip.active = true;
        game->mysteryShip.y = 5; // Position at top

        // Start from left or right
        if (rand() % 2 == 0) {
            game->mysteryShip.x = -MYSTERY_SHIP_WIDTH;
            game->mysteryShip.direction = 1;
        } else {
            game->mysteryShip.x = LCD_WIDTH;
            game->mysteryShip.direction = -1;
        }
    }
}

// Update player score
void updateScore(GameState* game, int points) {
    game->score += points;
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