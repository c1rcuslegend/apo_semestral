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

void show_image_scale(unsigned char *parlcd_mem_base, PPMImage* image, float scale, int a, int b) {
    parlcd_write_cmd(parlcd_mem_base, 0x2c);

    // Calculate the boundaries of the scaled image
    int scaled_width = (int)(image->width * scale);
    int scaled_height = (int)(image->height * scale);

    // Calculate the area to update (clamp to screen boundaries)
    int start_x = (a < 0) ? 0 : a;
    int start_y = (b < 0) ? 0 : b;
    int end_x = (a + scaled_width > LCD_WIDTH) ? LCD_WIDTH : a + scaled_width;
    int end_y = (b + scaled_height > LCD_HEIGHT) ? LCD_HEIGHT : b + scaled_height;

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int img_x = (x - a) / scale;
            int img_y = (y - b) / scale;

            if (img_x >= 0 && img_x < image->width &&
                img_y >= 0 && img_y < image->height) {
                parlcd_write_data(parlcd_mem_base,
                    image->pixels[img_y * image->width + img_x]);
                }
        }
    }
}

void clear_screen(unsigned char *parlcd_mem_base, uint16_t color) {
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        parlcd_write_data(parlcd_mem_base, color);
    }
}