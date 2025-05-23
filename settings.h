#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

typedef enum {
    GAME_MODE_REGULAR,
    GAME_MODE_BIZARRE
} GameMode;

extern GameMode current_game_mode;

// Initialize settings with default values
void initSettings(void);

// Get and set game mode
GameMode getGameMode(void);
void setGameMode(GameMode mode);

#endif /* SETTINGS_H */