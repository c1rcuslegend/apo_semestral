#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "serialize_lock.h"
#include "font_types.h"

// GLOBAL FONT VALUE
extern font_descriptor_t font_winFreeSystem14x16;

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

// Function prototypes
void drawPixel(unsigned short *fb, int x, int y, uint16_t color);
void clearScreen(unsigned short *fb, uint16_t color);
void drawChar(unsigned short *fb, int x, int y, char ch, font_descriptor_t *font, uint16_t color);
void drawString(unsigned short *fb, int x, int y, const char *text, font_descriptor_t *font, uint16_t color);


int main(int argc, char *argv[])
{

    /* Serialize execution of applications */

    /* Try to acquire lock the first */
    if (serialize_lock(1) <= 0) {
        printf("System is occupied\n");

        if (1) {
              printf("Waitting\n");
              /* Wait till application holding lock releases it or exits */
              serialize_lock(0);
        }
    }

    printf("Game started!\n");

    // Initialize hardware
    unsigned char *parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
    /*
    * Setup memory mapping which provides access to the peripheral
    * registers region of RGB LEDs, knobs and line of yellow LEDs.
    */
    unsigned char *mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);

    /* If mapping fails exit with error code */
    if ((mem_base == NULL) || (parlcd_mem_base == NULL)) {
        printf("LCD or LED memory mapping failed\n");
        exit(1);
    }
    printf("Memory mapped successfully\n");

    // Initialize LCD
    parlcd_hx8357_init(parlcd_mem_base);
    printf("LCD initialized\n");

    // Frame buffer for LCD
    unsigned short *fb = (unsigned short *)malloc(LCD_WIDTH * LCD_HEIGHT * 2);
    if (fb == NULL) {
        printf("Memory allocation for framebuffer failed\n");
        exit(1);
    }
    printf("Framebuffer allocated\n");

    // Clear screen with black background
    clearScreen(fb, 0x0000);

    // Draw some text
    drawString(fb, 40, 80, "SPACE INVADERS", &font_winFreeSystem14x16, 0xFFFF);
    drawString(fb, 60, 120, "Micro Edition", &font_winFreeSystem14x16, 0x07E0);
    drawString(fb, 80, 160, "Press any button to start", &font_winFreeSystem14x16, 0xF800);

    // Update LCD display
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (int i = 0; i < LCD_HEIGHT * LCD_WIDTH; i++) {
        parlcd_write_data(parlcd_mem_base, ((uint16_t *)fb)[i]);
    }

    // Wait for 5 seconds
    sleep(5);

    printf("Game ended!\n");

    /* Release the lock and clean up*/
    free(fb);
    serialize_unlock();

    return 0;
}

// Draw a single pixel
void drawPixel(unsigned short *fb, int x, int y, uint16_t color) {
    if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < LCD_HEIGHT) {
        *((uint16_t *)fb + y * LCD_WIDTH + x) = color;
    }
}

// Clear the screen with a single color
void clearScreen(unsigned short *fb, uint16_t color) {
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        ((uint16_t *)fb)[i] = color;
    }
}

// Get character width for proportional fonts
int charWidth(font_descriptor_t* fdes, char ch) {
    int width = 0;
    if ((ch >= fdes->firstchar) && (ch - fdes->firstchar < fdes->size)) {
        int idx = ch - fdes->firstchar;
        if (!fdes->width) {
            width = fdes->maxwidth;
        } else {
            width = fdes->width[idx];
        }
    }
    return width;
}

// Draw a single character
void drawChar(unsigned short *fb, int x, int y, char ch, font_descriptor_t *font, uint16_t color) {
    // check if the character is within the fonts range
    if (ch < font->firstchar || ch >= font->firstchar + font->size) {
        return;
    }

    // the index of the character in the font data
    int idx = ch - font->firstchar;
    int width = charWidth(font, ch);
    int height = font->height;

    // pointer to the start of the font bitmap data
    const uint16_t *bits = font->bits;
    // Check if offset array exists before using it
    if (font->offset) {
        bits += font->offset[idx]; // offset table if available
    } else {
        bits += idx * height; // otherwise calculate position
    }


    for (int j = 0; j < height; j++) { // for each row
        // Start with a mask that has only the leftmost bit set (bit 15 in a 16-bit value)
        // 1000 0000 0000 0000
        uint16_t mask = 1 << 15;
        for (int i = 0; i < width; i++) { // for each column
            // If bit is set ( = 1), draw a pixel at that position
            if (bits[j] & mask) {
                drawPixel(fb, x + i, y + j, color);
            }
            // shift the mask right by 1 position
            // 1000 0000 0000 0000 --> 0100 0000 0000 0000 --> 0010 0000 0000 0000 --> ...
            mask = mask >> 1;
        }
    }
}

// Draw a string of text
void drawString(unsigned short *fb, int x, int y, const char *text, font_descriptor_t *font, uint16_t color) {
    int orig_x = x;

    while (*text) {
        if (*text == '\n') {
            x = orig_x;
            y += font->height;
        } else {
            drawChar(fb, x, y, *text, font, color);
            x += charWidth(font, *text) + 1; // a small gap between characters
        }
        text++;
    }
}
