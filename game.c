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
#include "settings.h"

// Array of background colors - from light blue to deep purple (deeper space)
#define BACKGROUND_COLORS_COUNT 8
static const uint16_t backgroundColors[BACKGROUND_COLORS_COUNT] = {
    0xAEDF,  // Light blue
    0x5EDF,  // Sky blue
    0x3C9F,  // Medium blue
    0x1C5F,  // Deep blue
    0x18BF,  // Indigo
    0x88BF,  // Blue-purple
    0xA09F,  // Purple
    0x801F   // Dark purple
};

#define BIZARRE_SPRITES_COUNT 8
static const char* bizarreSprites[BIZARRE_SPRITES_COUNT] = {
    "sprites/captain_america.ppm",
    "sprites/flash.ppm",
    "sprites/hulk.ppm",
    "sprites/spiderman.ppm",
    "sprites/superman.ppm",
    "sprites/thor.ppm",
    "sprites/wolverine.ppm",
    "sprites/wonder_woman.ppm"
};


// GLOBAL FONT VALUE
extern font_descriptor_t font_winFreeSystem14x16;

extern uint64_t get_time_ms();

void initEnemies(GameState* game);

bool initGame(GameState* game, MemoryMap* memMap, bool multiplayer) {
    if (!game) return false;
    // Initialize input system
    inputInit(memMap);

    // Get current game mode
    GameMode mode = getGameMode();

    // Load ship sprite
    if (mode == GAME_MODE_BIZARRE) {
        game->shipSprite[0] = read_ppm("sprites/harley_quinn.ppm");
    } else {
        game->shipSprite[0] = read_ppm("sprites/player_01.ppm");
    }
    if (!game->shipSprite) {
        printf("Failed to load ship sprite\n");
        return false;
    }

    // Set ship initial position and parameters
    game->shipScale[0] = (mode == GAME_MODE_BIZARRE) ? 3.0f : 1.5f ;
    game->shipWidth[0] = game->shipSprite->width * game->shipScale;
    game->shipHeight[0] = game->shipSprite->height * game->shipScale;

    // Position ship at bottom center of screen
    game->shipX[0] = (LCD_WIDTH - game->shipWidth) / 2;
    game->shipY[0] = GAME_BOUNDARY_Y - game->shipHeight;

    // Initialize bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        game->bullets[0][i].active = false;
    }
    game->lastShotTime = 0;

    // Player 2 initialization (if multiplayer)
    game->isMultiplayer = multiplayer;
    if (multiplayer) {
        if (mode == GAME_MODE_BIZARRE) {
            game->shipSprite[1] = read_ppm("sprites/poison_ivy.ppm");
        } else {
            game->shipSprite[1] = read_ppm("sprites/player_02.ppm");
        }
        if (!game->shipSprite[1]) {
            // Fall back to player 1 sprite if player 2 sprite can't be loaded
            game->shipSprite[1] = game->shipSprite[0];
        }

        game->shipX[1] = (LCD_WIDTH - game->shipWidth) / 2 + 50;  // Offset from player 1
        game->shipY[1] = LCD_HEIGHT - game->shipHeight - 20;
        game->score[1] = 0;
        game->lives[1] = 3;

        // Initialize player 2 bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            game->bullets[1][i].active = false;
        }
    }

    // Load enemy sprites
    int usedIndices[3] = {-1, -1, -1}; // Array to track which bizarre sprites are used

    for (int i = 0; i < 3; i++) {
        char filename[32];
        if (mode == GAME_MODE_BIZARRE) {
            // Load bizarre enemy sprites - select a random unused sprite
            int randomIndex;
            bool isDuplicate;

            do {
                isDuplicate = false;
                randomIndex = rand() % BIZARRE_SPRITES_COUNT;

                // Check if this index was already used
                for (int j = 0; j < i; j++) {
                    if (randomIndex == usedIndices[j]) {
                        isDuplicate = true;
                        break;
                    }
                }
            } while (isDuplicate);

            // Store the selected index
            usedIndices[i] = randomIndex;

            // Get the filename from our array
            sprintf(filename, "%s", bizarreSprites[randomIndex]);
        } else {
            // Load regular enemy sprites
            sprintf(filename, "sprites/nemesis_0%d.ppm", i+1);
        }
        game->enemySprites[i] = read_ppm(filename);
        if (!game->enemySprites[i]) {
            printf("Failed to load enemy sprite!\n");
            return false;
        }
    }

    // Load mystery ship sprite
    if (mode == GAME_MODE_BIZARRE) {
        game->mysteryShipSprite = read_ppm("sprites/batman.ppm");
    } else {
        game->mysteryShipSprite = read_ppm("sprites/space_ship.ppm");
    }
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
    game->score[0] = 0;
    game->lives[0] = 3;

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

    // Update player 1 (RED_KNOB)
    // Get knob rotation for horizontal movement
    int moveX1 = getKnobRotation(RED_KNOB) * SHIP_SPEED;
    // Update ship position
    game->shipX[0] += moveX1;

    // Keep ship within screen boundaries
    if (game->shipX[0] < 0) {
        game->shipX[0] = 0;
    }
    if (game->shipX[0] > LCD_WIDTH - game->shipWidth) {
        game->shipX[0] = LCD_WIDTH - game->shipWidth;
    }

    // Process shooting (RED_KNOB button)
    if (isButtonPressed(RED_KNOB)) {
        fireBullet(game, 0); // 0 = player 1
    }

    // Handle player 2 if in multiplayer mode
    if (game->isMultiplayer) {
        // Update player 2 (BLUE_KNOB)
        int moveX2 = getKnobRotation(BLUE_KNOB) * SHIP_SPEED;
        game->shipX[1] += moveX2;

        // Keep player 2 within screen boundaries
        if (game->shipX[1] < 0) {
            game->shipX[1] = 0;
        }
        if (game->shipX[1] > LCD_WIDTH - game->shipWidth) {
            game->shipX[1] = LCD_WIDTH - game->shipWidth;
        }

        // Process player 2 shooting
        if (isButtonPressed(BLUE_KNOB)) {
            fireBullet(game, 1);  // 1 = player 2
        }
    }

    // Update player bullets and bullet collisions
    updatePlayerBullets(game, memMap);

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

    // Check game over condition
    if (game->isMultiplayer) {
        game->gameOver = (game->lives[0] <= 0 && game->lives[1] <= 0);
    } else {
        game->gameOver = (game->lives[0] <= 0);
    }
}

void renderGame(GameState* game, unsigned short* fb, unsigned char* parlcd_mem_base) {
    if (!game) return;

    // Clear the screen with level-appropriate background color (deeper in space)
    int colorIndex = game->level - 1;  // Level starts at 1
    if (colorIndex >= BACKGROUND_COLORS_COUNT) {
        colorIndex = BACKGROUND_COLORS_COUNT - 1;  // Stop and use darkest blue for high levels
    }
    clearScreen(fb, backgroundColors[colorIndex]);

    // Draw boundary line
    for (int x = 0; x < LCD_WIDTH; x++) {
        drawPixel(fb, x, GAME_BOUNDARY_Y, 0xFFFF); // White line
    }

    // Draw ships
    if (game->lives[0] > 0) {
        draw_sprite(fb, game->shipSprite[0], game->shipX[0], game->shipY[0],
                    game->shipWidth, game->shipHeight, 0x0000);
    }

    // Draw player 2 ship if in multiplayer mode
    if (game->isMultiplayer && game->lives[1] > 0) {
        draw_sprite(fb, game->shipSprite[1], game->shipX[1], game->shipY[1],
                   game->shipWidth, game->shipHeight, 0x0000);
    }

    // Draw bullets
    for (int player = 0; player < (game->isMultiplayer ? 2 : 1); player++) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (game->bullets[player][i].active) {
                // Draw player's bullet (different colors for each player)
                unsigned short bulletColor = (player == 0) ? BULLET_COLOR : 0x07FF; // Cyan for P2
                for (int y = 0; y < BULLET_HEIGHT; y++) {
                    for (int x = 0; x < BULLET_WIDTH; x++) {
                        drawPixel(fb, game->bullets[player][i].x + x, game->bullets[player][i].y + y, BULLET_COLOR);
                    }
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

    // Draw score/lives/level in bottom area
    char scoreText1[32], livesText1[32];
    sprintf(scoreText1, "SCORE1: %d", game->score[0]);
    sprintf(livesText1, "LIVES1: %d", game->lives[0]);

    // Calculate positions to display text
    int xPos = 10;

    // Draw player 1 info
    drawString(fb, xPos, GAME_BOUNDARY_Y + 10, scoreText1, &font_winFreeSystem14x16, 0xFFFF, 1);
    xPos += stringWidth(scoreText1, &font_winFreeSystem14x16, 1) + 10;

    drawString(fb, xPos, GAME_BOUNDARY_Y + 10, livesText1, &font_winFreeSystem14x16, 0xFFFF, 1);
    xPos += stringWidth(livesText1, &font_winFreeSystem14x16, 1) + 10;

    // Draw level info in the middle
    char levelText[32];
    sprintf(levelText, "LEVEL: %d", game->level);

    // Center the level text
    int levelX;
    if (game->isMultiplayer) {
        levelX = (LCD_WIDTH - stringWidth(levelText, &font_winFreeSystem14x16, 1)) / 2;
    } else {
        levelX = xPos;
    }

    drawString(fb, levelX, GAME_BOUNDARY_Y + 10, levelText, &font_winFreeSystem14x16, 0xFFFF, 1);

    // Draw player 2 info if in multiplayer mode
    if (game->isMultiplayer) {
        char scoreText2[32], livesText2[32];
        sprintf(scoreText2, "SCORE: %d", game->score[1]);
        sprintf(livesText2, "LIVES: %d", game->lives[1]);

        // Place player 2 info on the right side
        int p2X = LCD_WIDTH - stringWidth(scoreText2, &font_winFreeSystem14x16, 1) -
                  stringWidth(livesText2, &font_winFreeSystem14x16, 1) - 20;

        drawString(fb, p2X, GAME_BOUNDARY_Y + 10, scoreText2, &font_winFreeSystem14x16, 0xFFFF, 1);
        p2X += stringWidth(scoreText2, &font_winFreeSystem14x16, 1) + 10;

        drawString(fb, p2X, GAME_BOUNDARY_Y + 10, livesText2, &font_winFreeSystem14x16, 0xFFFF, 1);
    }
    // Update display
    updateDisplay(parlcd_mem_base, fb);
}

void cleanupGame(GameState* game) {
    if (!game) return;

    // Free player 1 sprite
    if (game->shipSprite[0]) {
        free_ppm(game->shipSprite[0]);
        game->shipSprite[0] = NULL;
    }

    // Free player 2 sprite if different from player 1
    if (game->isMultiplayer && game->shipSprite[1] && game->shipSprite[1] != game->shipSprite[0]) {
        free_ppm(game->shipSprite[1]);
        game->shipSprite[1] = NULL;
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