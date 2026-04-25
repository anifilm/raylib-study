#include "game.h"
#include "collision.h"
#include "stdlib.h"
#include "time.h"

void ChangeState(GameContext *game, GameState newState)
{
    game->prevState = game->state;
    game->state = newState;
    game->stateTimer = 0.0f;
}

void InitGame(GameContext *game)
{
    srand((unsigned int)time(NULL));

    game->state = GAME_MENU;
    game->prevState = GAME_MENU;
    game->stateTimer = 0.0f;
    game->score = 0;
    game->elapsedTime = 0.0f;
    game->exitUnlocked = false;

    // Generate initial maze
    GenerateMaze(&game->maze, MAX_ROWS, MAX_COLS);
    InitPlayer(&game->player, game->maze.startPos);
}

void UpdateGame(GameContext *game, float deltaTime)
{
    game->stateTimer += deltaTime;

    switch (game->state)
    {
    case GAME_MENU:
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
        {
            ChangeState(game, GAME_PLAYING);
        }
        break;

    case GAME_PLAYING:
        game->elapsedTime += deltaTime;
        UpdatePlayer(&game->player, &game->maze, deltaTime);

        // Check treasure collection
        if (CheckTreasure(&game->player, &game->maze))
        {
            game->player.treasuresCollected++;
            game->score += 100;
            // Unlock exit when at least one treasure is collected
            if (!game->exitUnlocked)
            {
                game->exitUnlocked = true;
            }
        }

        // Check exit reached
        if (CheckExit(&game->player, game))
        {
            ChangeState(game, GAME_WIN);
        }

        // Pause handling
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
        {
            ChangeState(game, GAME_PAUSED);
        }
        break;

    case GAME_PAUSED:
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
        {
            ChangeState(game, GAME_PLAYING);
        }
        break;

    case GAME_WIN:
    case GAME_LOSE:
        if (game->stateTimer > 2.0f && IsKeyPressed(KEY_ENTER))
        {
            // Reset game
            GenerateMaze(&game->maze, MAX_ROWS, MAX_COLS);
            InitPlayer(&game->player, game->maze.startPos);
            game->score = 0;
            game->elapsedTime = 0.0f;
            game->exitUnlocked = false;
            ChangeState(game, GAME_MENU);
        }
        break;
    }
}
