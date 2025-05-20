#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>

#include "gui.h"
#include "graphics.h"
#include "font_types.h"
#include "main_menu.h"
#include "input.h"

// GLOBAL FONT VALUE
extern font_descriptor_t font_winFreeSystem14x16;

// get time in milliseconds
static uint64_t get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL);
}

bool displayStartMenu(unsigned short *fb, unsigned char *parlcd_mem_base, unsigned char *mem_base, MemoryMap *memMap) {
    inputInit(memMap);
    // Clear screen with black background
    clearScreen(fb, 0x0000);

    // Draw starting text
    drawCenteredString(fb, 120, "SPACE INVADERS", &font_winFreeSystem14x16, 0xFFFF, 3);
    drawCenteredString(fb, 180, "Micro Edition", &font_winFreeSystem14x16, 0x07E0, 2);

    // Blinking text implementation
    bool text_visible = true;
    uint64_t last_toggle = get_time_ms();
    uint64_t toggle_interval = 500; // Toggle every 500ms

    // Text position parameters
    int text_y = 220;
    int text_height = font_winFreeSystem14x16.height * 1; // scale factor is 1

    // Update display with initial content
    updateDisplay(parlcd_mem_base, fb);

    // Wait for input (60 sec max)
    uint64_t start_time = get_time_ms();
    while (get_time_ms() - start_time < 60000) {
        uint64_t current_time = get_time_ms();

        // visibility every interval
        if (current_time - last_toggle >= toggle_interval) {
            text_visible = !text_visible;
            last_toggle = current_time;

            // clear text area
            for (int y = text_y; y < text_y + text_height; y++) {
                for (int x = 0; x < LCD_WIDTH; x++) {
                    drawPixel(fb, x, y, 0x0000);
                }
            }

            // draw text again
            if (text_visible) {
                drawCenteredString(fb, text_y, "Press any button to start", &font_winFreeSystem14x16, 0xF800, 1);
            }

            // Update display
            updateDisplay(parlcd_mem_base, fb);
        }

        // Check for any button press
        for (int i = 0; i < 3; i++) {
            if (isButtonPressed(i)) {
                usleep(300000); // Debounce delay
                return true;  // Button was pressed
            }
        }

        // Small delay to prevent CPU hogging
        usleep(10000); // 10ms
    }

    return false;
}