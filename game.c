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

    game->gameOver = false;

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

    // Process shooting (BLUE_KNOB button)
    if (isButtonPressed(BLUE_KNOB)) {
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
}