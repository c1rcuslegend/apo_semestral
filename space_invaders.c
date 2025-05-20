#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "serialize_lock.h"
#include "font_types.h"
#include "graphics.h"
#include "gui.h"

// GLOBAL FONT VALUE
extern font_descriptor_t font_winFreeSystem14x16;

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

// Returns time in milliseconds
uint64_t get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL);
}

int main(int argc, char *argv[])
{
    /* Serialize execution of applications */

    /* Try to acquire lock the first */
    if (serialize_lock(1) <= 0) {
        printf("System is occupied\n");

        if (1) {
              printf("Waitting\n");
              /* Wait till application holding lock releases it or exits */
              serialize_lock(0);
        }
    }

    printf("Game started!\n");

    // Initialize hardware
    unsigned char *parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
    /*
    * Setup memory mapping which provides access to the peripheral
    * registers region of RGB LEDs, knobs and line of yellow LEDs.
    */
    unsigned char *mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);

    /* If mapping fails exit with error code */
    if ((mem_base == NULL) || (parlcd_mem_base == NULL)) {
        printf("LCD or LED memory mapping failed\n");
        exit(1);
    }
    printf("Memory mapped successfully\n");

    // Initialize LCD
    parlcd_hx8357_init(parlcd_mem_base);
    printf("LCD initialized\n");

    // Frame buffer for LCD
    unsigned short *fb = (unsigned short *)malloc(LCD_WIDTH * LCD_HEIGHT * 2);
    if (fb == NULL) {
        printf("Memory allocation for framebuffer failed\n");
        exit(1);
    }
    printf("Framebuffer allocated\n");

    // Display start menu and wait for user input
    if (displayStartMenu(fb, parlcd_mem_base, mem_base)) {
        // Start the game
        printf("Game playing...\n");

        // PLACEHOLDER
    }

    printf("Game ended!\n");

    // Clear screen with black background
    clearScreen(fb, 0x0000);
    /* Release the lock and clean up*/
    free(fb);
    serialize_unlock();

    return 0;
}
