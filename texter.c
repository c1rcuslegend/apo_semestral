#include <stdio.h>
#include <stdlib.h>
#include "texter.h"

#define HIGH_SCORE_FILE "score.txt"

int readHighScore() {
    FILE *file = fopen(HIGH_SCORE_FILE, "r");
    if (!file) {
        printf("No high score file exists yet!\n");
        return 0;
    }

    int score;
    if (fscanf(file, "%d", &score) != 1) {
        printf("Failed to read score!\n");
        score = 0;
    }

    fclose(file);
    return score;
}

bool writeHighScore(int score) {
    FILE *file = fopen(HIGH_SCORE_FILE, "w");
    if (!file) {
        return false; // Couldn't open file for writing
    }

    fprintf(file, "%d", score);
    fclose(file);
    return true;
}