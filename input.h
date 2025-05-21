#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>

// Knob IDs
#define RED_KNOB    2
#define GREEN_KNOB  1
#define BLUE_KNOB   0

// Button states
#define BUTTON_RELEASED 0
#define BUTTON_PRESSED  1

// Menu options
#define MENU_START_GAME   0
#define MENU_MULTIPLAYER  1
#define MENU_SETTINGS     2
#define MENU_OPTIONS_COUNT 3

// stores base addresses
typedef struct {
    unsigned char *mem_base;     // knobs/LED registers
    unsigned char *parlcd_base;  // LCD
} MemoryMap;

// Input initialization
void inputInit(MemoryMap *memMap);
// Read raw 32-bit value from knobs register
uint32_t readKnobsRegister();
// Get current value of specified knob (0-255)
uint8_t getKnobValue(int knobId);
// Get change in knob value since last read
int getKnobRotation(int knobId);
// Check if knob button is pressed
bool isButtonPressed(int knobId);
// Wait for any button press with timeout (in ms), return button pressed or -1 for timeout
int waitForAnyButtonPress(unsigned long timeoutMs);

#endif // INPUT_H