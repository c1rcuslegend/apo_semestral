#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include "game.h"

// Collision detection
bool checkCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

// Bullet management
void updatePlayerBullets(GameState* game, MemoryMap* memMap);
void fireBullet(GameState* game, int playerIndex);
void fireEnemyBullet(GameState* game, int enemyX, int enemyY);
void updateEnemyBullets(GameState* game);

// Enemy management
bool canEnemyShoot(GameState* game, int row, int col);
void updateEnemyFormation(GameState* game);
void updateMysteryShip(GameState* game);

// Scoring
void updateScore(GameState* game, int points, int player);

#endif // GAME_UTILS_H