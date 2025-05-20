#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <stdbool.h>
#include "input.h"

// Menu options
#define MENU_START_GAME   0
#define MENU_MULTIPLAYER  1
#define MENU_SETTINGS     2
#define MENU_OPTIONS_COUNT 3

// Menu navigation structure
typedef struct {
    int selection;       // Current selected
    int itemCount;       // Total number of menu items
    bool itemSelected;   // True if current item has been selected
} MenuState;

// Initialize menu state
void initMenuState(MenuState *menu, int itemCount);

// Update menu selection based on knob rotation
bool updateMenuSelection(MenuState *menu, int knobId);

// Process menu input - returns true if selection was made
bool processMenuInput(MenuState *menu, int knobId);

// Display and handle main menu
int showMainMenu(unsigned short *fb, unsigned char *parlcd_mem_base, MemoryMap *memMap);

#endif // MAIN_MENU_H