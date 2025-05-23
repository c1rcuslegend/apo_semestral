#ifndef GUI_H
#define GUI_H

#include <stdbool.h>
#include "main_menu.h"
#include "input.h"

// Displays the start menu and waits for user input
bool displayStartMenu(unsigned short *fb, unsigned char *parlcd_mem_base, unsigned char *mem_base,  MemoryMap *memMap);

// Displays the game over screen
bool displayGameOverScreen(unsigned short *fb, unsigned char *parlcd_mem_base, MemoryMap *memMap, int score[2], bool isMultiplayer);

// Displays the settings menu and handles user input
bool displaySettingsMenu(unsigned short *fb, unsigned char *parlcd_mem_base, MemoryMap *memMap);

#endif /* GUI_H */