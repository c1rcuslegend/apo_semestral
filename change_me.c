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

  // Display the image
  parlcd_write_cmd(parlcd_mem_base, 0x2c);
  for (int y = 0; y < LCD_HEIGHT; y++) {
    for (int x = 0; x < LCD_WIDTH; x++) {
      // If pixel is within image bounds, display it; otherwise display black
      if (x < image->width && y < image->height) {
        parlcd_write_data(parlcd_mem_base, image->pixels[y * image->width + x]);
      } else {
        parlcd_write_data(parlcd_mem_base, 0x0000);  // Black color
      }
    }
  }

  // Clean up
  free_ppm(image);
  serialize_unlock();

  return 0;
}