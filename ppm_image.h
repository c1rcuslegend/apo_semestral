#ifndef PPM_IMAGE_H
#define PPM_IMAGE_H

#include <stdint.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "serialize_lock.h"

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int max_color;
    uint16_t* pixels;  // Store as RGB565 format for LCD
} PPMImage;

// Read PPM image from file
PPMImage* read_ppm(const char* filename);

// Free the image structure
void free_ppm(PPMImage* img);

// Show the image on the LCD with scaling
void show_image_scale(unsigned char *parlcd_mem_base, PPMImage* image, float scale, int a, int b);

// Clear the screen with a specific color
void clear_screen(unsigned char *parlcd_mem_base, uint16_t color);

#endif //PPM_IMAGE_H