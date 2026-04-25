#include "maze.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"

static void shuffle(int *array, int size)
{
    for (int i = size - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

static bool is_in_bounds(int row, int col, int rows, int cols)
{
    return row >= 0 && row < rows && col >= 0 && col < cols;
}

void GenerateMaze(Maze *maze, int rows, int cols)
{
    maze->rows = rows;
    maze->cols = cols;

    // Initialize all cells as walls
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            maze->grid[r][c] = CELL_WALL;
        }
    }

    // Use Prim's algorithm
    // 1. Start with a random cell, mark as passage
    // 2. Add its walls to wall list
    // 3. While wall list not empty:
    //    - Pick random wall
    //    - If exactly one side is a passage, make passage
    //    - Remove wall from list

    // Start position (odd coordinates for proper maze)
    int startRow = 1;
    int startCol = 1;
    maze->grid[startRow][startCol] = CELL_EMPTY;
    maze->startPos = (Vector2){(float)startCol, (float)startRow};

    // Wall list: store pairs of (row, col)
    // We'll use a simple array-based list
    typedef struct
    {
        int row;
        int col;
        int dir; // 0=up, 1=right, 2=down, 3=left (the wall direction from passage)
    } Wall;

    Wall walls[1000];
    int wallCount = 0;

    // Add neighboring walls of starting cell
    int dRow[] = {-1, 0, 1, 0};
    int dCol[] = {0, 1, 0, -1};

    for (int i = 0; i < 4; i++)
    {
        int nr = startRow + dRow[i];
        int nc = startCol + dCol[i];
        if (is_in_bounds(nr, nc, rows, cols) && maze->grid[nr][nc] == CELL_WALL)
        {
            walls[wallCount].row = nr;
            walls[wallCount].col = nc;
            walls[wallCount].dir = i;
            wallCount++;
        }
    }

    // Process walls
    while (wallCount > 0)
    {
        // Pick random wall
        int idx = rand() % wallCount;
        Wall wall = walls[idx];

        // Remove wall from list (swap with last)
        walls[idx] = walls[wallCount - 1];
        wallCount--;

        int row = wall.row;
        int col = wall.col;
        int dir = wall.dir;

        // Get the cell on the other side of this wall
        int cellRow = row + dRow[dir];
        int cellCol = col + dCol[dir];

        // Check if the other cell is out of bounds
        if (!is_in_bounds(cellRow, cellCol, rows, cols))
        {
            continue;
        }

        // If the cell on the other side is not a passage yet
        if (maze->grid[cellRow][cellCol] == CELL_WALL)
        {
            // Make the wall a passage
            maze->grid[row][col] = CELL_EMPTY;
            // Make the other cell a passage too
            maze->grid[cellRow][cellCol] = CELL_EMPTY;

            // Add neighboring walls of the new cell
            for (int i = 0; i < 4; i++)
            {
                int nr = cellRow + dRow[i];
                int nc = cellCol + dCol[i];
                if (is_in_bounds(nr, nc, rows, cols) && maze->grid[nr][nc] == CELL_WALL)
                {
                    walls[wallCount].row = nr;
                    walls[wallCount].col = nc;
                    walls[wallCount].dir = i;
                    wallCount++;
                }
            }
        }
    }

    // Place exit at bottom-right area (find a passage near the corner)
    bool exitPlaced = false;
    for (int r = rows - 2; r > 0 && !exitPlaced; r--)
    {
        for (int c = cols - 2; c > 0 && !exitPlaced; c--)
        {
            if (maze->grid[r][c] == CELL_EMPTY)
            {
                maze->grid[r][c] = CELL_EXIT;
                maze->exitPos = (Vector2){(float)c, (float)r};
                exitPlaced = true;
            }
        }
    }

    // Place multiple treasures (3% of total cells)
    maze->treasureCount = 0;
    int targetTreasures = (rows * cols) / 33; // 3% = ~18 for 25x25
    int treasureAttempts = 0;
    while (maze->treasureCount < targetTreasures && treasureAttempts < 10000)
    {
        int r = rand() % (rows - 2) + 1;
        int c = rand() % (cols - 2) + 1;
        if (maze->grid[r][c] == CELL_EMPTY &&
            !(r == (int)maze->startPos.y && c == (int)maze->startPos.x) &&
            !(r == (int)maze->exitPos.y && c == (int)maze->exitPos.x))
        {
            // Check if this cell already has a treasure
            bool alreadyHasTreasure = false;
            for (int i = 0; i < maze->treasureCount; i++)
            {
                if ((int)maze->treasurePos[i].y == r && (int)maze->treasurePos[i].x == c)
                {
                    alreadyHasTreasure = true;
                    break;
                }
            }
            if (!alreadyHasTreasure)
            {
                maze->treasurePos[maze->treasureCount] = (Vector2){(float)c, (float)r};
                maze->treasureCount++;
            }
        }
        treasureAttempts++;
    }
    maze->treasuresCollected = 0;
}

void ResetMaze(Maze *maze)
{
    for (int r = 0; r < maze->rows; r++)
    {
        for (int c = 0; c < maze->cols; c++)
        {
            maze->grid[r][c] = CELL_WALL;
        }
    }
}

bool IsWall(Maze *maze, int row, int col)
{
    if (!IsValidPosition(maze, row, col))
        return true;
    return maze->grid[row][col] == CELL_WALL;
}

bool IsValidPosition(Maze *maze, int row, int col)
{
    return row >= 0 && row < maze->rows && col >= 0 && col < maze->cols;
}
