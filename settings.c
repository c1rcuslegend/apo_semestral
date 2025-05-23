#include "settings.h"

// Global variable to store game mode
GameMode current_game_mode = GAME_MODE_REGULAR;

void initSettings(void) {
    current_game_mode = GAME_MODE_REGULAR;
}

GameMode getGameMode(void) {
    return current_game_mode;
}

void setGameMode(GameMode mode) {
    current_game_mode = mode;
}