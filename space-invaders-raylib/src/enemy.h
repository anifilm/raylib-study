#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "bullet.h"

#define ENEMY_WIDTH 40
#define ENEMY_HEIGHT 30
#define ENEMY_ROWS 5
#define ENEMY_COLS 11
#define ENEMY_SPEED 1
#define ENEMY_DROP 20

typedef struct {
    float x;
    float y;
    int width;
    int height;
    bool alive;
    int type;
    int frame;
} Enemy;

void EnemyInitAll(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]);
void EnemyUpdateAll(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]);
void EnemyDrawAll(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]);
void EnemyShoot(Enemy enemies[ENEMY_ROWS][ENEMY_COLS], Bullet *bullets);
bool EnemyAllDestroyed(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]);
bool EnemyReachedBottom(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]);

#endif