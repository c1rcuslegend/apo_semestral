#ifndef PPM_IMAGE_H
#define PPM_IMAGE_H

#include <stdint.h>

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
void show_image_scale(PPMImage* image, float scale);

#endif //PPM_IMAGE_H