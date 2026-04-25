#ifndef PLAYER_H
#define PLAYER_H

#include "maze.h"
#include "raylib.h"

#define TILE_SIZE 32
#define MOVE_DURATION 0.15f // seconds for one tile move

typedef enum
{
    PLAYER_IDLE,
    PLAYER_MOVING
} PlayerState;

typedef struct
{
    Vector2 gridPos;
    Vector2 renderPos;
    Vector2 targetPos;
    float moveProgress;
    PlayerState state;
    int treasuresCollected;
} Player;

void InitPlayer(Player *player, Vector2 startPos);
void UpdatePlayer(Player *player, Maze *maze, float deltaTime);
void DrawPlayer(Player *player);
bool CanMoveTo(Player *player, Maze *maze, Vector2 direction);

#endif // PLAYER_H
