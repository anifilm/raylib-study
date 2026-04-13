#ifndef PLAYER_H
#define PLAYER_H

#include "bullet.h"
#include "raylib.h"

#define PLAYER_WIDTH 50
#define PLAYER_HEIGHT 30
#define PLAYER_SPEED 5
#define PLAYER_SHOOT_COOLDOWN 0.7f

typedef struct
{
    float x;
    float y;
    int width;
    int height;
    int speed;
    float shootTimer;
} Player;

void PlayerInit(Player *player);
void PlayerUpdate(Player *player);
void PlayerDraw(const Player *player);
void PlayerShoot(Player *player, Bullet *bullets);

#endif
