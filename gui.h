#ifndef GUI_H
#define GUI_H

#include <stdbool.h>
#include "main_menu.h"
#include "input.h"

/**
 * Displays the start menu and waits for user input
 *
 * @param fb Pointer to the framebuffer
 * @param parlcd_mem_base Pointer to the LCD memory base
 * @param mem_base Pointer to the LED/button memory base
 * @return true when user presses a button
 */
bool displayStartMenu(unsigned short *fb, unsigned char *parlcd_mem_base, unsigned char *mem_base,  MemoryMap *memMap);

#endif /* GUI_H */