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

void show_image_scale(unsigned char *parlcd_mem_base, PPMImage* image, float scale) {
    parlcd_write_cmd(parlcd_mem_base, 0x2c);  // Move command here

    int offset_x = (LCD_WIDTH - (image->width * scale)) / 2;
    int offset_y = (LCD_HEIGHT - (image->height * scale)) / 2;

    // Display scaled image
    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            // Calculate if current pixel is within image bounds
            int img_x = (x - offset_x) / scale;
            int img_y = (y - offset_y) / scale;

            if (img_x >= 0 && img_x < image->width &&
                img_y >= 0 && img_y < image->height) {
                // Display scaled image pixel
                parlcd_write_data(parlcd_mem_base,
                    image->pixels[img_y * image->width + img_x]);
                } else {
                    // Display black for areas outside image
                    parlcd_write_data(parlcd_mem_base, 0x0000);
                }
        }
    }
}