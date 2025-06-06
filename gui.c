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
#include "settings.h"
#include "texter.h"

// GLOBAL FONT VALUE
extern font_descriptor_t font_winFreeSystem14x16;
extern font_descriptor_t font_rom8x16;

// get time in milliseconds
static uint64_t get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL);
}

bool displayStartMenu(unsigned short *fb, unsigned char *parlcd_mem_base, unsigned char *mem_base, MemoryMap *memMap) {
    inputInit(memMap);
    // Clear screen with black background
    clearScreen(fb, 0x7010);

    // Draw starting text
    drawCenteredString(fb, 120, "SPACE INVADERS", &font_rom8x16, 0xFFE0, 3);
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
                    drawPixel(fb, x, y, 0x7010);
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

bool displayGameOverScreen(unsigned short *fb, unsigned char *parlcd_mem_base,
                         MemoryMap *memMap, int score[2], bool isMultiplayer) {
    // Clear screen with dark background
    clearScreen(fb, 0x0000);

    // Read high score
    int highScore = readHighScore();
    bool isNewHighScore = false;

    int playerScore = 0;

    // Check if multiplayer mode
    if (isMultiplayer) {
        playerScore = (score[0] > score[1]) ? score[0] : score[1];
    } else {
        playerScore = score[0];
    }

    // Check if new high score achieved
    if (playerScore > highScore) {
        writeHighScore(playerScore);
        isNewHighScore = true;
        highScore = playerScore;
    }

    // Game Over text
    char gameOver[] = "GAME OVER";
    drawCenteredString(fb, 100, gameOver, &font_rom8x16, 0xFF00, 2);

    if (isMultiplayer) {
        char winnerText[32];
        sprintf(winnerText, "Player %d WINS!", (score[0] > score[1]) ? 1 : 2);
        drawCenteredString(fb, 130, winnerText, &font_winFreeSystem14x16, 0xFFFF, 1);
    } else {
        char singlePlayerText[] = "Well done!";
        drawCenteredString(fb, 130, singlePlayerText, &font_winFreeSystem14x16, 0xFFFF, 1);
    }
    // Score display
    char scoreText[32];
    sprintf(scoreText, "Score: %d", playerScore);
    drawCenteredString(fb, 160, scoreText, &font_winFreeSystem14x16, 0xFFFF, 1);

    // High score display
    char highScoreText[32];
    sprintf(highScoreText, "High Score: %d", highScore);
    drawCenteredString(fb, 190, highScoreText, &font_winFreeSystem14x16, 0xFFFF, 1);

    // New high score message if applicable
    if (isNewHighScore) {
        char newHighScoreText[] = "NEW HIGH SCORE!";
        drawCenteredString(fb, 220, newHighScoreText, &font_winFreeSystem14x16, 0xFFE0, 1); // Yellow color
    }

    // Press any button to continue
    char continueText[] = "Press any button to continue";
    drawCenteredString(fb, 270, continueText, &font_winFreeSystem14x16, 0xFFFF, 1);

    // Update display
    updateDisplay(parlcd_mem_base, fb);

    // Check for any button press
    while (1) {
        for (int i = 0; i < 3; i++) {
            if (isButtonPressed(i)) {
                clearScreen(fb, 0x0000);
                updateDisplay(parlcd_mem_base, fb);
                return true;  // Button was pressed
            }
        }
    }

    return false;
}

bool displaySettingsMenu(unsigned short *fb, unsigned char *parlcd_mem_base, MemoryMap *memMap) {
    GameMode currentMode = getGameMode();

    while (1) {
        // Clear screen with dark blue background
        clearScreen(fb, 0x7010);

        // Draw title
        drawCenteredString(fb, 80, "SETTINGS", &font_rom8x16, 0x07E0, 2);

        // Draw game mode option
        drawCenteredString(fb, 140, "GAME MODE:", &font_winFreeSystem14x16, 0xFFFF, 1);

        // Draw current mode with highlighted color
        const char* modeText = (currentMode == GAME_MODE_REGULAR) ? "REGULAR" : "BIZARRE";
        drawCenteredString(fb, 170, modeText, &font_winFreeSystem14x16, 0xF800, 1); // Red color

        // Draw instructions
        drawCenteredString(fb, 220, "Press ANY BUTTON to toggle mode", &font_winFreeSystem14x16, 0xFFFF, 1);
        drawCenteredString(fb, 250, "Press BLUE to exit", &font_winFreeSystem14x16, 0xFFFF, 1);

        // Update display
        updateDisplay(parlcd_mem_base, fb);

        // Wait for button press
        int buttonPressed = waitForAnyButtonPress(1000); // 1 second timeout

        if (buttonPressed == BLUE_KNOB) {
            // Exit settings
            usleep(500000);
            return true;
        } else if (buttonPressed != -1) { // Any button press (except timeout)
            // Toggle game mode
            currentMode = (currentMode == GAME_MODE_REGULAR) ?
                          GAME_MODE_BIZARRE : GAME_MODE_REGULAR;

            // Save the new game mode
            setGameMode(currentMode);

             // Add debounce delay
             usleep(500000); // 500ms delay
        }
    }
}