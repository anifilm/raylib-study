#ifndef BULLET_H
#define BULLET_H

#include "raylib.h"

#define MAX_PLAYER_BULLETS 5
#define MAX_ENEMY_BULLETS 10
#define BULLET_WIDTH 4
#define BULLET_HEIGHT 15

typedef struct {
    float x;
    float y;
    int width;
    int height;
    float speed;
    bool active;
} Bullet;

void BulletInitAll(Bullet *bullets, int count);
void BulletUpdateAll(Bullet *bullets, int count);
void BulletDrawAll(Bullet *bullets, int count);

#endif