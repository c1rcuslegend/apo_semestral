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
#include "ppm_image.h"

#define LCD_WIDTH 480
#define LCD_HEIGHT 320
#define NUM_CHARACTERS 3

const char* image_files[NUM_CHARACTERS] = {
  "sprites/batman.ppm",
  "sprites/harley_quinn.ppm",
  "sprites/poison_ivy.ppm"
};

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

  // Create array of PPMImage pointers
  PPMImage* images[NUM_CHARACTERS];

  // Load all images
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    images[i] = read_ppm(image_files[i]);
    if (!images[i]) {
      printf("Failed to load image: %s\n", image_files[i]);
      // Clean up previously loaded images
      for (int j = 0; j < i; j++) {
        free_ppm(images[j]);
      }
      serialize_unlock();
      return 1;
    }
  }

  clear_screen(parlcd_mem_base, 0x0000);

  show_image_scale(parlcd_mem_base, images[0], 10.0f, 0, 0);
  show_image_scale(parlcd_mem_base, images[1], 10.0f, 170, 0);
  show_image_scale(parlcd_mem_base, image[2], 10.0f, 340, 0);

  // Clean up all images
  for (int i = 0; i < NUM_CHARACTERS; i++) {
    free_ppm(images[i]);
  }
  serialize_unlock();

  return 0;
}