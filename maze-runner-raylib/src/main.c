#include "game.h"
#include "raylib.h"

int main(void)
{
    // Maze is 25x25 cells at 32px each = 800x800
    // Add padding for UI at top (80px) and some margin
    const int screenWidth = 800;
    const int screenHeight = 880;

    InitWindow(screenWidth, screenHeight, "Maze Runner");
    SetTargetFPS(60);

    GameContext game;
    InitGame(&game);

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        UpdateGame(&game, deltaTime);

        BeginDrawing();
        DrawGame(&game);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
