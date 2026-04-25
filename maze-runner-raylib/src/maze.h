#ifndef MAZE_H
#define MAZE_H

#include "raylib.h"

#define MAX_ROWS 25
#define MAX_COLS 25
#define MAX_TREASURES 18 // 3% of 25x25 = 18.75

typedef enum
{
    CELL_EMPTY = 0,
    CELL_WALL = 1,
    CELL_EXIT = 2
} CellType;

typedef struct
{
    int rows;
    int cols;
    int grid[MAX_ROWS][MAX_COLS];
    Vector2 startPos;
    Vector2 exitPos;
    Vector2 treasurePos[MAX_TREASURES];
    int treasureCount;
    int treasuresCollected;
} Maze;

void GenerateMaze(Maze *maze, int rows, int cols);
void ResetMaze(Maze *maze);
bool IsWall(Maze *maze, int row, int col);
bool IsValidPosition(Maze *maze, int row, int col);

#endif // MAZE_H
