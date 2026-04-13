#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "barrier.h"
#include "explosion.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

typedef struct {
    GameState state;
    int score;
    int lives;
} Game;

void GameInit(Game *game);
void GameUpdate(Game *game);
void GameDraw(const Game *game);

#endif