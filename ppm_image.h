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
// Draw a sprite with scaling and transparency
void draw_sprite(
    unsigned short* fb,           // Framebuffer
    PPMImage* sprite,             // Sprite image
    int x, int y,                 // Position
    int width, int height,        // Desired dimensions
    uint16_t transparentColor     // Transparent color
);

#endif //PPM_IMAGE_H