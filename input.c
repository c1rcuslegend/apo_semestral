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