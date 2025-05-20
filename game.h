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

// Game state structure
typedef struct {
    PPMImage* shipSprite;   // Ship sprite
    int shipX;              // Ship X position
    int shipY;              // Ship Y position (fixed)
    int shipWidth;          // Width of ship sprite when rendered
    int shipHeight;         // Height of ship sprite when rendered
    float shipScale;        // Scale factor for ship
    bool gameOver;          // Game over flag
} GameState;

// Initialize the game
bool initGame(GameState* game, MemoryMap* memMap);
// Update game state based on input
void updateGame(GameState* game, MemoryMap* memMap);
// Render the game to the framebuffer
void renderGame(GameState* game, unsigned short* fb, unsigned char* parlcd_mem_base);
// Free resources when game is done
void cleanupGame(GameState* game);

#endif // GAME_H