#include "raylib.h"

int main(void)
{
    InitWindow(800, 450, "Hello, Raylib!");

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Hello, Raylib!", 350, 200, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}