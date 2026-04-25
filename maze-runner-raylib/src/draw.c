#include "draw.h"
#include "math.h"

static void DrawTile(int row, int col, Color color)
{
    DrawRectangle(col * CELL_SIZE, row * CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
}

void DrawMaze(GameContext *game)
{
    Maze *maze = &game->maze;
    bool exitUnlocked = game->exitUnlocked;

    for (int r = 0; r < maze->rows; r++)
    {
        for (int c = 0; c < maze->cols; c++)
        {
            int x = c * CELL_SIZE;
            int y = r * CELL_SIZE;

            if (maze->grid[r][c] == CELL_WALL)
            {
                // Wall
                DrawTile(r, c, COLOR_WALL);
                // Add some 3D effect
                DrawRectangle(x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4, (Color){52, 73, 94, 255});
            }
            else if (maze->grid[r][c] == CELL_EXIT)
            {
                // Exit
                DrawTile(r, c, exitUnlocked ? COLOR_EXIT_OPEN : COLOR_EXIT_LOCKED);
                // Draw door frame
                DrawRectangleLines(x + 4, y + 4, CELL_SIZE - 8, CELL_SIZE - 8, DARKGRAY);
            }
            else
            {
                // Floor
                DrawTile(r, c, COLOR_FLOOR);
            }
        }
    }

    // Draw all treasures that haven't been collected
    for (int i = 0; i < maze->treasureCount; i++)
    {
        if (maze->treasurePos[i].x >= 0 && maze->treasurePos[i].y >= 0)
        {
            int tx = (int)maze->treasurePos[i].x * CELL_SIZE;
            int ty = (int)maze->treasurePos[i].y * CELL_SIZE;

            // Draw treasure (diamond shape)
            Vector2 center = {tx + CELL_SIZE / 2, ty + CELL_SIZE / 2};
            DrawCircle((int)center.x, (int)center.y, CELL_SIZE / 3, COLOR_TREASURE);
            DrawCircle((int)center.x, (int)center.y, CELL_SIZE / 3 - 3, (Color){241, 196, 15, 200});
        }
    }
}

void DrawPlayerVisual(Player *player)
{
    int size = CELL_SIZE - 8;
    int offset = 4;
    Vector2 pos = {
        player->renderPos.x + offset,
        player->renderPos.y + offset};

    // Player body
    DrawRectangleRounded((Rectangle){pos.x, pos.y, size, size}, 0.3f, 10, COLOR_PLAYER);

    // Eyes
    int eyeY = (int)pos.y + size / 3;
    int eyeSize = 4;
    DrawCircle((int)pos.x + size / 3, eyeY, eyeSize, WHITE);
    DrawCircle((int)pos.x + 2 * size / 3, eyeY, eyeSize, WHITE);
}

void DrawUI(GameContext *game)
{
    int screenWidth = GetScreenWidth();

    // Score
    DrawText(TextFormat("Score: %d", game->score), 20, 20, 24, WHITE);

    // Time
    int minutes = (int)game->elapsedTime / 60;
    int seconds = (int)game->elapsedTime % 60;
    DrawText(TextFormat("Time: %02d:%02d", minutes, seconds), screenWidth - 150, 20, 24, WHITE);

    // Treasure status
    const char *treasureText = TextFormat("Treasures: %d/%d", game->maze.treasuresCollected, game->maze.treasureCount);
    DrawText(treasureText, 20, 50, 20, WHITE);
}

void DrawMenu(void)
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    DrawText("MAZE RUNNER", screenWidth / 2 - MeasureText("MAZE RUNNER", 48) / 2, screenHeight / 3, 48, WHITE);
    DrawText("Find treasures to unlock the exit!", screenWidth / 2 - MeasureText("Find treasures to unlock the exit!", 24) / 2, screenHeight / 3 + 60, 24, LIGHTGRAY);

    DrawText("Controls:", screenWidth / 2 - MeasureText("Controls:", 24) / 2, screenHeight / 2, 24, WHITE);
    DrawText("Arrow Keys / WASD - Move", screenWidth / 2 - MeasureText("Arrow Keys / WASD - Move", 20) / 2, screenHeight / 2 + 40, 20, LIGHTGRAY);
    DrawText("P / ESC - Pause", screenWidth / 2 - MeasureText("P / ESC - Pause", 20) / 2, screenHeight / 2 + 65, 20, LIGHTGRAY);

    DrawText("Press ENTER or SPACE to start", screenWidth / 2 - MeasureText("Press ENTER or SPACE to start", 24) / 2, screenHeight * 2 / 3 + 40, 24, COLOR_TREASURE);
}

void DrawWinScreen(int score, float time)
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    DrawText("YOU ESCAPED!", screenWidth / 2 - MeasureText("YOU ESCAPED!", 48) / 2, screenHeight / 3, 48, COLOR_EXIT_OPEN);

    DrawText(TextFormat("Final Score: %d", score), screenWidth / 2 - MeasureText(TextFormat("Final Score: %d", score), 28) / 2, screenHeight / 2, 28, WHITE);

    int minutes = (int)time / 60;
    int seconds = (int)time % 60;
    DrawText(TextFormat("Time: %02d:%02d", minutes, seconds), screenWidth / 2 - MeasureText(TextFormat("Time: %02d:%02d", minutes, seconds), 24) / 2, screenHeight / 2 + 40, 24, LIGHTGRAY);

    DrawText("Press ENTER to return to menu", screenWidth / 2 - MeasureText("Press ENTER to return to menu", 20) / 2, screenHeight * 2 / 3 + 40, 20, LIGHTGRAY);
}

void DrawPauseScreen(void)
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Semi-transparent overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 150});

    DrawText("PAUSED", screenWidth / 2 - MeasureText("PAUSED", 48) / 2, screenHeight / 2 - 30, 48, WHITE);
    DrawText("Press P or ESC to resume", screenWidth / 2 - MeasureText("Press P or ESC to resume", 20) / 2, screenHeight / 2 + 30, 20, LIGHTGRAY);
}

void DrawGame(GameContext *game)
{
    switch (game->state)
    {
    case GAME_MENU:
        ClearBackground(COLOR_BG);
        DrawMenu();
        break;

    case GAME_PLAYING:
    case GAME_PAUSED:
    case GAME_WIN:
    case GAME_LOSE:
        ClearBackground(COLOR_BG);

        // Calculate offset to center maze
        int mazePixelWidth = game->maze.cols * CELL_SIZE;
        int mazePixelHeight = game->maze.rows * CELL_SIZE;
        int offsetX = (GetScreenWidth() - mazePixelWidth) / 2;
        int offsetY = (GetScreenHeight() - mazePixelHeight) / 2;

        // Draw with offset
        for (int r = 0; r < game->maze.rows; r++)
        {
            for (int c = 0; c < game->maze.cols; c++)
            {
                int x = offsetX + c * CELL_SIZE;
                int y = offsetY + r * CELL_SIZE;

                Color color;
                if (game->maze.grid[r][c] == CELL_WALL)
                {
                    color = COLOR_WALL;
                }
                else if (game->maze.grid[r][c] == CELL_EXIT)
                {
                    color = game->exitUnlocked ? COLOR_EXIT_OPEN : COLOR_EXIT_LOCKED;
                }
                else
                {
                    color = COLOR_FLOOR;
                }

                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, color);
            }
        }

        // Draw all treasures that haven't been collected
        for (int i = 0; i < game->maze.treasureCount; i++)
        {
            if (game->maze.treasurePos[i].x >= 0 && game->maze.treasurePos[i].y >= 0)
            {
                int tx = offsetX + (int)game->maze.treasurePos[i].x * CELL_SIZE + CELL_SIZE / 2;
                int ty = offsetY + (int)game->maze.treasurePos[i].y * CELL_SIZE + CELL_SIZE / 2;
                DrawCircle(tx, ty, CELL_SIZE / 3, COLOR_TREASURE);
            }
        }

        // Draw player
        {
            int px = offsetX + (int)game->player.renderPos.x;
            int py = offsetY + (int)game->player.renderPos.y;
            int size = CELL_SIZE - 8;

            DrawRectangleRounded((Rectangle){px + 4, py + 4, size, size}, 0.3f, 10, COLOR_PLAYER);
            DrawCircle(px + 4 + size / 3, py + 4 + size / 3, 4, WHITE);
            DrawCircle(px + 4 + 2 * size / 3, py + 4 + size / 3, 4, WHITE);
        }

        // Draw UI
        DrawUI(game);

        if (game->state == GAME_PAUSED)
        {
            DrawPauseScreen();
        }
        else if (game->state == GAME_WIN)
        {
            DrawWinScreen(game->score, game->elapsedTime);
        }
        break;
    }
}
