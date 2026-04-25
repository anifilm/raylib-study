#ifndef COLLISION_H
#define COLLISION_H

#include "game.h"
#include "maze.h"
#include "player.h"

static inline bool CheckTreasure(Player *player, Maze *maze)
{
    int row = (int)player->gridPos.y;
    int col = (int)player->gridPos.x;

    for (int i = 0; i < maze->treasureCount; i++)
    {
        if ((int)maze->treasurePos[i].y == row && (int)maze->treasurePos[i].x == col)
        {
            // Mark this treasure as collected by moving it off-grid
            maze->treasurePos[i].x = -1;
            maze->treasurePos[i].y = -1;
            maze->treasuresCollected++;
            return true;
        }
    }
    return false;
}

static inline bool CheckExit(Player *player, GameContext *game)
{
    if (!game->exitUnlocked)
        return false;

    int row = (int)player->gridPos.y;
    int col = (int)player->gridPos.x;

    return (row == (int)game->maze.exitPos.y && col == (int)game->maze.exitPos.x);
}

#endif // COLLISION_H
