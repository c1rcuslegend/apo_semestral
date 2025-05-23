#include "ppm_image.h"
#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "serialize_lock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

// Convert RGB888 to RGB565
static uint16_t rgb888_to_rgb565(int r, int g, int b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

PPMImage* read_ppm(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    PPMImage* img = malloc(sizeof(PPMImage));
    if (!img) {
        fclose(file);
        return NULL;
    }

    char line[128];

    // Read magic number
    if (!fgets(line, sizeof(line), file) || strncmp(line, "P6", 2) != 0) {
        free(img);
        fclose(file);
        return NULL;
    }

    // Skip comments
    do {
        if (!fgets(line, sizeof(line), file)) {
            free(img);
            fclose(file);
            return NULL;
        }
    } while (line[0] == '#');

    // Read width and height
    if (sscanf(line, "%u %u", &img->width, &img->height) != 2) {
        free(img);
        fclose(file);
        return NULL;
    }

    // Read maximum color value
    if (!fgets(line, sizeof(line), file)) {
        free(img);
        fclose(file);
        return NULL;
    }
    img->max_color = atoi(line);

    // Allocate memory for pixels
    img->pixels = malloc(img->width * img->height * sizeof(uint16_t));
    if (!img->pixels) {
        free(img);
        fclose(file);
        return NULL;
    }

    // Read pixel data and convert to RGB565
    unsigned char pixel[3];
    for (unsigned int i = 0; i < img->width * img->height; i++) {
        if (fread(pixel, 1, 3, file) != 3) {
            free(img->pixels);
            free(img);
            fclose(file);
            return NULL;
        }
        img->pixels[i] = rgb888_to_rgb565(pixel[0], pixel[1], pixel[2]);
    }

    fclose(file);
    return img;
}

void free_ppm(PPMImage* img) {
    if (img) {
        free(img->pixels);
        free(img);
    }
}

void draw_sprite( unsigned short* fb, PPMImage* sprite, int x, int y, int width, int height, uint16_t transparentColor) {
    if (!fb || !sprite) return;

    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            // Calculate source coordinates with scaling
            int srcX = dx * sprite->width / width;
            int srcY = dy * sprite->height / height;

            if (srcX < sprite->width && srcY < sprite->height) {
                uint16_t color = sprite->pixels[srcY * sprite->width + srcX];

                // Skip transparent pixels
                if (color != transparentColor) {
                    // Calculate destination coordinates
                    int destX = x + dx;
                    int destY = y + dy;

                    // Check if destination is within screen bounds
                    if (destX >= 0 && destX < LCD_WIDTH && destY >= 0 && destY < LCD_HEIGHT) {
                        fb[destY * LCD_WIDTH + destX] = color;
                    }
                }
            }
        }
    }
}