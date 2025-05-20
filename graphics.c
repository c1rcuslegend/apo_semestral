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

// Calculate string width
int stringWidth(const char *text, font_descriptor_t *font) {
    int width = 0;
    while (*text) {
        if (*text != '\n') {
            width += charWidth(font, *text) + 1;
        } else {
            break; // Only calculate width of first line
        }
        text++;
    }
    return width > 0 ? width - 1 : 0;
}

// Draw string centered horizontally on screen
void drawCenteredString(unsigned short *fb, int y, const char *text, font_descriptor_t *font, uint16_t color) {
    int text_width = stringWidth(text, font);
    int x = (LCD_WIDTH - text_width) / 2;
    drawString(fb, x, y, text, font, color);
}


// Update the LCD display with the frame buffer
void updateDisplay(unsigned char *parlcd_mem_base, unsigned short *fb) {
    // send the Memory Write command (0x2c) to the LCD controller (tell it we want to start writing data)
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (int i = 0; i < LCD_HEIGHT * LCD_WIDTH; i++) {
        // write the pixel data to the LCD controller
        // each pixel is a 16-bit color value
        // 5 red, 6 green, 5 blue
        parlcd_write_data(parlcd_mem_base, fb[i]);
    }
}