#ifndef GAME_H
#define GAME_H

#include "maze.h"
#include "player.h"
#include "raylib.h"

typedef enum
{
    GAME_MENU = 0,
    GAME_PLAYING = 1,
    GAME_WIN = 2,
    GAME_LOSE = 3,
    GAME_PAUSED = 4
} GameState;

typedef struct
{
    GameState state;
    GameState prevState;
    Maze maze;
    Player player;
    int score;
    float elapsedTime;
    bool exitUnlocked;
    float stateTimer;
} GameContext;

void InitGame(GameContext *game);
void UpdateGame(GameContext *game, float deltaTime);
void DrawGame(GameContext *game);
void ChangeState(GameContext *game, GameState newState);

#endif // GAME_H
