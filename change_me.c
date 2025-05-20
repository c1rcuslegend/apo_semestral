#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "serialize_lock.h"
#include "ppm_read_image.h"

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

int main(int argc, char *argv[])
{
  // Serialize execution of applications
  if (serialize_lock(1) <= 0) {
    printf("System is occupied\n");
    if (1) {
      printf("Waiting\n");
      serialize_lock(0);
    }
  }

  // Initialize hardware
  unsigned char *parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
  if (parlcd_mem_base == NULL) {
    printf("LCD mapping failed\n");
    serialize_unlock();
    return 1;
  }

  // Initialize LCD
  parlcd_hx8357_init(parlcd_mem_base);

  // Read the PPM image
  PPMImage* image = read_ppm("batman.ppm");
  if (!image) {
    printf("Failed to load image\n");
    serialize_unlock();
    return 1;
  }

  // Display the image with scaling
  parlcd_write_cmd(parlcd_mem_base, 0x2c);

  // Calculate scaling factors
  float scale_x = (float)LCD_WIDTH / image->width;
  float scale_y = (float)LCD_HEIGHT / image->height;
  float scale = (scale_x < scale_y) ? scale_x : scale_y;  // Use smaller scale to fit screen

  // Calculate centered position
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

  // Clean up
  free_ppm(image);
  serialize_unlock();

  return 0;
}