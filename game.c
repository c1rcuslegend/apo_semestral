#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "game.h"
#include "graphics.h"
#include "input.h"
#include "font_types.h"

// GLOBAL FONT VALUE
extern font_descriptor_t font_winFreeSystem14x16;

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

    game->gameOver = false;

    return true;
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
        // TODO: Implement shooting
    }
}

void renderGame(GameState* game, unsigned short* fb, unsigned char* parlcd_mem_base) {
    if (!game) return;

    // Clear the screen
    clearScreen(fb, 0x0000); // Black background

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

                // Skip transparent pixels (assuming 0x0000 is transparent)
                if (color != 0x0000) {
                    drawPixel(fb, game->shipX + x, game->shipY + y, color);
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