#include "player.h"
#include "math.h"

// Lerp function
static float LerpFloat(float start, float end, float t)
{
    return start + (end - start) * t;
}

void InitPlayer(Player *player, Vector2 startPos)
{
    player->gridPos = startPos;
    player->renderPos = (Vector2){startPos.x * TILE_SIZE, startPos.y * TILE_SIZE};
    player->targetPos = startPos;
    player->moveProgress = 0.0f;
    player->state = PLAYER_IDLE;
    player->treasuresCollected = 0;
}

bool CanMoveTo(Player *player, Maze *maze, Vector2 direction)
{
    int newRow = (int)player->gridPos.y + (int)direction.y;
    int newCol = (int)player->gridPos.x + (int)direction.x;

    return IsValidPosition(maze, newRow, newCol) && !IsWall(maze, newRow, newCol);
}

void UpdatePlayer(Player *player, Maze *maze, float deltaTime)
{
    // Handle movement animation
    if (player->state == PLAYER_MOVING)
    {
        // Update movement progress
        player->moveProgress += deltaTime / MOVE_DURATION;

        if (player->moveProgress >= 1.0f)
        {
            // Movement complete
            player->renderPos.x = player->targetPos.x * TILE_SIZE;
            player->renderPos.y = player->targetPos.y * TILE_SIZE;
            player->gridPos = player->targetPos;
            player->state = PLAYER_IDLE;
            player->moveProgress = 0.0f;
        }
        else
        {
            // Ease-out interpolation
            float t = 1.0f - powf(1.0f - player->moveProgress, 3.0f);
            float startX = player->gridPos.x * TILE_SIZE;
            float startY = player->gridPos.y * TILE_SIZE;
            float targetX = player->targetPos.x * TILE_SIZE;
            float targetY = player->targetPos.y * TILE_SIZE;

            player->renderPos.x = LerpFloat(startX, targetX, t);
            player->renderPos.y = LerpFloat(startY, targetY, t);
        }
    }

    // Handle input - check keys every frame regardless of state
    {
        Vector2 direction = {0};

        // Check for any movement key - use IsKeyDown for continuous movement
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
            direction.y = -1;
        else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
            direction.y = 1;
        else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
            direction.x = -1;
        else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
            direction.x = 1;

        // Only start new movement if we're idle and direction is set
        if (player->state == PLAYER_IDLE && (direction.x != 0 || direction.y != 0))
        {
            if (CanMoveTo(player, maze, direction))
            {
                player->targetPos.x = player->gridPos.x + direction.x;
                player->targetPos.y = player->gridPos.y + direction.y;
                player->state = PLAYER_MOVING;
                player->moveProgress = 0.0f;
            }
        }
    }
}

void DrawPlayer(Player *player)
{
    int size = TILE_SIZE - 8;
    int offset = 4;

    // Draw player as a rounded square
    Vector2 pos = {
        player->renderPos.x + offset,
        player->renderPos.y + offset};

    DrawRectangle((int)pos.x + 2, (int)pos.y + 2, size - 4, size - 4, (Color){231, 76, 60, 255}); // COLOR_PLAYER

    // Draw eyes (direction indicator)
    DrawCircle((int)pos.x + size / 3, (int)pos.y + size / 3, 3, WHITE);
    DrawCircle((int)pos.x + 2 * size / 3, (int)pos.y + size / 3, 3, WHITE);
}
