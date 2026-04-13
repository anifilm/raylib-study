#include "raylib.h"
#include "game.h"

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Invaders - Raylib");
    SetTargetFPS(60);

    Game game;
    GameInit(&game);

    while (!WindowShouldClose()) {
        GameUpdate(&game);
        GameDraw(&game);
    }

    CloseWindow();
    return 0;
}