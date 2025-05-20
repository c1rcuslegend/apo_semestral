#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include "font_types.h"

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

 // Draw a single pixel
void drawPixel(unsigned short *fb, int x, int y, uint16_t color);
// Clear the screen with a single color
void clearScreen(unsigned short *fb, uint16_t color);
// Get character width for proportional fonts
int charWidth(font_descriptor_t* fdes, char ch);
// Draw a single character
void drawChar(unsigned short *fb, int x, int y, char ch, font_descriptor_t *font, uint16_t color);
// Draw a string of characters
void drawString(unsigned short *fb, int x, int y, const char *text, font_descriptor_t *font, uint16_t color);
// Calculate string width
int stringWidth(const char *text, font_descriptor_t *font);
// Draw string centered horizontally on screen
void drawCenteredString(unsigned short *fb, int y, const char *text, font_descriptor_t *font, uint16_t color);
// Update the LCD display with the frame buffer
void updateDisplay(unsigned char *parlcd_mem_base, unsigned short *fb);

#endif /* GRAPHICS_H */