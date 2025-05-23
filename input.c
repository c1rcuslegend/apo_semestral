#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "input.h"
#include "mzapo_regs.h"

// Global memory map
static MemoryMap memoryMap;

// Store previous knob values to detect rotation
static uint8_t prevKnobValues[3] = {0, 0, 0};

// Read raw 32-bit value from knobs register
uint32_t readKnobsRegister() {
    if (memoryMap.mem_base == NULL) {
        return 0;
    }
    return *(volatile uint32_t*)(memoryMap.mem_base + SPILED_REG_KNOBS_8BIT_o);
}

// Initialize input handling
void inputInit(MemoryMap *memMap) {
    memoryMap = *memMap;

    // Initialize previous values by reading current state
    uint32_t knobsValue = readKnobsRegister();
    prevKnobValues[RED_KNOB] = (knobsValue >> 16) & 0xFF;
    prevKnobValues[GREEN_KNOB] = (knobsValue >> 8) & 0xFF;
    prevKnobValues[BLUE_KNOB] = knobsValue & 0xFF;
}

// Get current value of specified knob (0-255)
uint8_t getKnobValue(int knobId) {
    uint32_t knobsValue = readKnobsRegister();

    switch (knobId) {
        case RED_KNOB:
            return (knobsValue >> 16) & 0xFF;
        case GREEN_KNOB:
            return (knobsValue >> 8) & 0xFF;
        case BLUE_KNOB:
            return knobsValue & 0xFF;
        default:
            return 0;
    }
}

// Get change in knob value since last read
int getKnobRotation(int knobId) {
    uint8_t currentValue = getKnobValue(knobId);
    int rotation = 0;

    // Calculate rotation based on difference
    // also handle wrap-around cases
    if (currentValue > prevKnobValues[knobId]) {
        if (currentValue - prevKnobValues[knobId] < 128) {
            rotation = currentValue - prevKnobValues[knobId];
        } else {
            rotation = (currentValue - 256) - prevKnobValues[knobId];
        }
    } else if (currentValue < prevKnobValues[knobId]) {
        if (prevKnobValues[knobId] - currentValue < 128) {
            rotation = currentValue - prevKnobValues[knobId];
        } else {
            rotation = (currentValue + 256) - prevKnobValues[knobId];
        }
    }

    // Update previous value
    prevKnobValues[knobId] = currentValue;

    return rotation;
}

// Check if knob button is pressed
bool isButtonPressed(int knobId) {
    uint32_t knobsValue = readKnobsRegister();
    uint32_t buttonMask = 0;

    switch (knobId) {
        case RED_KNOB:
            buttonMask = 1 << 26;
            break;
        case GREEN_KNOB:
            buttonMask = 1 << 25;
            break;
        case BLUE_KNOB:
            buttonMask = 1 << 24;
            break;
        default:
            return false;
    }

    return (knobsValue & buttonMask) != 0;
}

// Get milliseconds
static uint64_t getTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL);
}

// Wait for any button press with timeout (in ms), return button pressed or -1 for timeout
int waitForAnyButtonPress(unsigned long timeoutMs) {
    uint64_t startTime = getTimeMs();
    uint64_t currentTime;

    while (1) {
        currentTime = getTimeMs();
        if (currentTime - startTime >= timeoutMs) {
            return -1; // Timeout
        }

        if (isButtonPressed(RED_KNOB)) return RED_KNOB;
        if (isButtonPressed(GREEN_KNOB)) return GREEN_KNOB;
        if (isButtonPressed(BLUE_KNOB)) return BLUE_KNOB;

        // Small delay to prevent CPU hogging
        usleep(10000); // 10ms
    }
}

// Set the color of an RGB LED
void setRGBLed(int ledIndex, uint32_t color) {
    if (memoryMap.mem_base == NULL) {
        return;
    }

    // Select register based on LED index
    unsigned int offset;
    switch (ledIndex) {
        case 0:
            offset = SPILED_REG_LED_RGB1_o;
            break;
        case 1:
            offset = SPILED_REG_LED_RGB2_o;
            break;
        default:
            return; // Invalid LED index
    }

    // Write color value to the register
    *(volatile uint32_t*)(memoryMap.mem_base + offset) = color;
}

// Flash RGB LEDs red when an enemy is killed
void flashEnemyKillLED(MemoryMap *memMap, uint32_t color) {
    static uint64_t flashStartTime = 0;
    static bool flashing = false;
    static uint32_t led1Original = 0;
    static uint32_t led2Original = 0;

    uint64_t currentTime = getTimeMs();

    // Start flashing
    if (!flashing) {
        // Store original LED colors
        if (memoryMap.mem_base != NULL) {
            led1Original = *(volatile uint32_t*)(memoryMap.mem_base + SPILED_REG_LED_RGB1_o);
            led2Original = *(volatile uint32_t*)(memoryMap.mem_base + SPILED_REG_LED_RGB2_o);
        }

        // Set LEDs to color
        setRGBLed(0, color);
        setRGBLed(1, color);

        flashing = true;
        flashStartTime = currentTime;
    }

    // Check if flash duration has passed
    if (flashing && (currentTime - flashStartTime >= 1000)) {
        // Restore original LED colors
        turnOffAllLEDs(&memMap);

        flashing = false;
    }
}

// Turn off all LEDs
void turnOffAllLEDs(MemoryMap *memMap) {
    if (memMap->mem_base == NULL) {
        return;
    }

    // Turn off RGB LED 1
    *(volatile uint32_t*)(memMap->mem_base + SPILED_REG_LED_RGB1_o) = 0x0;

    // Turn off RGB LED 2
    *(volatile uint32_t*)(memMap->mem_base + SPILED_REG_LED_RGB2_o) = 0x0;

    // Turn off LED line
    *(volatile uint32_t*)(memMap->mem_base + SPILED_REG_LED_LINE_o) = 0x0;
}