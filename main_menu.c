#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "main_menu.h"
#include "input.h"
#include "graphics.h"
#include "mzapo_parlcd.h"
#include "mzapo_regs.h"
#include "font_types.h"

// External font reference
extern font_descriptor_t font_winFreeSystem14x16;

// Menu item text
static const char* menuItemLabels[MENU_OPTIONS_COUNT] = {
    "Start Game",
    "Multiplayer",
    "Settings"
};

// Colors
#define COLOR_BACKGROUND 0x2A0134   // Dark Purple
#define COLOR_TEXT       0xFFFF    // White
#define COLOR_SELECTED   0x001F    // Blue background
#define COLOR_HIGHLIGHT  0xF800    // Red

// Initialize menu state
void initMenuState(MenuState *menu, int itemCount) {
    if (menu == NULL) {
        return;
    }

    menu->selection = 0;
    menu->itemCount = itemCount;
    menu->itemSelected = false;
}

// Update menu selection based on knob rotation
bool updateMenuSelection(MenuState *menu, int knobId) {
    if (menu == NULL) {
        return false;
    }

    int rotation = getKnobRotation(knobId);

    if (rotation != 0) {
        // Update selection based on rotation
        menu->selection += rotation;
        // Wrap around if needed
        while (menu->selection < 0) {
            menu->selection += menu->itemCount;
        }
        menu->selection %= menu->itemCount;
        return true;  // Selection changed
    }

    return false;  // No change
}

// Process menu input - returns true if selection was made
bool processMenuInput(MenuState *menu, int knobId) {
    if (menu == NULL) {
        return false;
    }

    // Update menu selection based on knob rotation
    updateMenuSelection(menu, knobId);

    // Check if button is pressed
    if (isButtonPressed(knobId)) {
        menu->itemSelected = true;
        return true;  // Selection was made
    }

    return false;  // No selection
}

// Draw text with background highlight if selected
void drawMenuItem(unsigned short *fb, int x, int y, const char *text, bool isSelected) {
    font_descriptor_t *font = &font_winFreeSystem14x16;
    int scale = 2;
    int textWidth = stringWidth(text, font, scale);
    int textHeight = font->height * scale;

    // Draw background rectangle if selected
    if (isSelected) {
        // Draw background rectangle (slightly larger than text)
        int padding = 10;
        int bgX = x - padding;
        int bgY = y - padding/2;
        int bgWidth = textWidth + padding*2;
        int bgHeight = textHeight + padding;

        // Fill background rectangle
        for (int i = bgX; i < bgX + bgWidth; i++) {
            for (int j = bgY; j < bgY + bgHeight; j++) {
                drawPixel(fb, i, j, COLOR_SELECTED);
            }
        }

        // Draw text with highlight color
        drawString(fb, x, y, text, font, COLOR_HIGHLIGHT, scale);
    } else {
        // Draw regular text
        drawString(fb, x, y, text, font, COLOR_TEXT, scale);
    }
}

// Display and handle main menu
int showMainMenu(unsigned short *fb, unsigned char *parlcd_mem_base, MemoryMap *memMap) {
    // Initialize input system
    inputInit(memMap);

    MenuState menu;
    initMenuState(&menu, MENU_OPTIONS_COUNT);

    bool menuActive = true;
    bool redraw = true;

    // Clear screen initially
    clearScreen(fb, COLOR_BACKGROUND);

    while (menuActive) {
        // Check for knob rotation to update selection
        if (updateMenuSelection(&menu, RED_KNOB) ||
            updateMenuSelection(&menu, GREEN_KNOB) ||
            updateMenuSelection(&menu, BLUE_KNOB)) {
            redraw = true;
        }

        // Check for button press to make selection
        for (int i = 0; i < 3; i++) {
            if (isButtonPressed(i)) {
                // Button is pressed, mark selection
                menu.itemSelected = true;
                menuActive = false;
                break;
            }
        }

        // Redraw menu if needed
        if (redraw) {
            // Clear menu area (middle portion of screen)
            for (int y = 100; y < 220; y++) {
                for (int x = 100; x < 380; x++) {
                    drawPixel(fb, x, y, COLOR_BACKGROUND);
                }
            }

            // Draw title
            drawCenteredString(fb, 50, "SPACE INVADERS", &font_winFreeSystem14x16, COLOR_TEXT, 3);

            // Draw menu items
            int startY = 120;  // Vertical starting position
            int spacing = 40;  // Spacing between menu items
            int centerX = LCD_WIDTH / 2;

            for (int i = 0; i < MENU_OPTIONS_COUNT; i++) {
                int itemWidth = stringWidth(menuItemLabels[i], &font_winFreeSystem14x16, 2);
                int x = centerX - itemWidth/2;
                drawMenuItem(fb, x, startY + i * spacing,
                             menuItemLabels[i], (i == menu.selection));
            }

            // Update display
            updateDisplay(parlcd_mem_base, fb);
            redraw = false;
        }

        // Small delay to prevent CPU hogging
        usleep(10000);  // 10ms
    }

    // Return the selected menu option
    return menu.selection;
}