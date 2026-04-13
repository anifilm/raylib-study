#include "game.h"
#include "player.h"
#include "enemy.h"
#include "bullet.h"

static Player player;
static Enemy enemies[ENEMY_ROWS][ENEMY_COLS];
static Bullet playerBullets[MAX_PLAYER_BULLETS];
static Bullet enemyBullets[MAX_ENEMY_BULLETS];
static Barrier barriers[BARRIER_COUNT];
static Explosion playerExplosion;

static void BulletEnemyCollision(Bullet *bullets, Enemy enemies[ENEMY_ROWS][ENEMY_COLS], int *score) {
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!bullets[i].active) continue;

        for (int row = 0; row < ENEMY_ROWS; row++) {
            for (int col = 0; col < ENEMY_COLS; col++) {
                if (!enemies[row][col].alive) continue;

                Rectangle bulletRect = {bullets[i].x, bullets[i].y, bullets[i].width, bullets[i].height};
                Rectangle enemyRect = {enemies[row][col].x, enemies[row][col].y,
                                       enemies[row][col].width, enemies[row][col].height};

                if (CheckCollisionRecs(bulletRect, enemyRect)) {
                    bullets[i].active = false;
                    enemies[row][col].alive = false;

                    switch (enemies[row][col].type) {
                        case 0: *score += 30; break;
                        case 1: *score += 20; break;
                        case 2: *score += 10; break;
                    }
                    break;
                }
            }
        }
    }
}

static bool BulletPlayerCollision(Bullet *bullets, Player *player, Explosion *exp) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!bullets[i].active) continue;

        Rectangle bulletRect = {bullets[i].x, bullets[i].y, bullets[i].width, bullets[i].height};
        Rectangle playerRect = {player->x, player->y, player->width, player->height};

        if (CheckCollisionRecs(bulletRect, playerRect)) {
            bullets[i].active = false;
            // Trigger explosion at player center
            ExplosionTrigger(exp, player->x + player->width / 2, player->y + player->height / 2);
            return true;
        }
    }
    return false;
}

static void BulletBarrierCollision(Bullet *bullets, int count, Barrier barriers[BARRIER_COUNT]) {
    for (int i = 0; i < count; i++) {
        if (!bullets[i].active) continue;

        Rectangle bulletRect = {bullets[i].x, bullets[i].y, bullets[i].width, bullets[i].height};
        if (BarrierCheckCollision(barriers, bulletRect)) {
            bullets[i].active = false;
        }
    }
}

void GameInit(Game *game) {
    game->state = STATE_TITLE;
    game->score = 0;
    game->lives = 3;

    PlayerInit(&player);
    EnemyInitAll(enemies);
    BulletInitAll(playerBullets, MAX_PLAYER_BULLETS);
    BulletInitAll(enemyBullets, MAX_ENEMY_BULLETS);
    BarrierInitAll(barriers);
    ExplosionInit(&playerExplosion);
}

static void ResetGame(Game *game) {
    game->score = 0;
    game->lives = 3;

    PlayerInit(&player);
    EnemyInitAll(enemies);
    BulletInitAll(playerBullets, MAX_PLAYER_BULLETS);
    BulletInitAll(enemyBullets, MAX_ENEMY_BULLETS);
    BarrierInitAll(barriers);
    ExplosionInit(&playerExplosion);
}

void GameUpdate(Game *game) {
    // Update explosion effect
    ExplosionUpdate(&playerExplosion);

    switch (game->state) {
        case STATE_TITLE:
            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame(game);
                game->state = STATE_PLAYING;
            }
            break;

        case STATE_PLAYING: {
            PlayerUpdate(&player);
            EnemyUpdateAll(enemies);
            BulletUpdateAll(playerBullets, MAX_PLAYER_BULLETS);
            BulletUpdateAll(enemyBullets, MAX_ENEMY_BULLETS);

            if (IsKeyPressed(KEY_SPACE)) {
                PlayerShoot(&player, playerBullets);
            }

            EnemyShoot(enemies, enemyBullets);

            // Collision: player bullets vs barriers
            BulletBarrierCollision(playerBullets, MAX_PLAYER_BULLETS, barriers);
            // Collision: enemy bullets vs barriers
            BulletBarrierCollision(enemyBullets, MAX_ENEMY_BULLETS, barriers);

            // Collision: player bullets vs enemies
            BulletEnemyCollision(playerBullets, enemies, &game->score);

            // Collision: enemy bullets vs player
            if (BulletPlayerCollision(enemyBullets, &player, &playerExplosion)) {
                game->lives--;
                if (game->lives <= 0) {
                    game->state = STATE_GAME_OVER;
                } else {
                    // Respawn after explosion
                    PlayerInit(&player);
                    BulletInitAll(enemyBullets, MAX_ENEMY_BULLETS);
                }
            }

            if (EnemyAllDestroyed(enemies)) {
                game->state = STATE_GAME_OVER;
            }

            if (EnemyReachedBottom(enemies)) {
                game->state = STATE_GAME_OVER;
            }
            break;
        }

        case STATE_GAME_OVER:
            if (IsKeyPressed(KEY_ENTER)) {
                game->state = STATE_TITLE;
            }
            break;
    }
}

void GameDraw(const Game *game) {
    BeginDrawing();
    ClearBackground(BLACK);

    switch (game->state) {
        case STATE_TITLE:
            DrawText("SPACE INVADERS", SCREEN_WIDTH / 2 - MeasureText("SPACE INVADERS", 40) / 2, 200, 40, GREEN);
            DrawText("Press ENTER to Start", SCREEN_WIDTH / 2 - MeasureText("Press ENTER to Start", 20) / 2, 300, 20, WHITE);
            break;

        case STATE_PLAYING:
            if (!playerExplosion.active) {
                PlayerDraw(&player);
            }
            EnemyDrawAll(enemies);
            BarrierDrawAll(barriers);
            BulletDrawAll(playerBullets, MAX_PLAYER_BULLETS);
            BulletDrawAll(enemyBullets, MAX_ENEMY_BULLETS);
            ExplosionDraw(&playerExplosion);

            DrawText(TextFormat("Score: %d", game->score), 10, 10, 20, WHITE);
            DrawText(TextFormat("Lives: %d", game->lives), SCREEN_WIDTH - 100, 10, 20, WHITE);
            break;

        case STATE_GAME_OVER:
            ExplosionDraw(&playerExplosion);
            DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, 200, 40, RED);
            DrawText(TextFormat("Final Score: %d", game->score), SCREEN_WIDTH / 2 - MeasureText(TextFormat("Final Score: %d", game->score), 20) / 2, 260, 20, WHITE);
            DrawText("Press ENTER to Continue", SCREEN_WIDTH / 2 - MeasureText("Press ENTER to Continue", 20) / 2, 320, 20, WHITE);
            break;
    }

    EndDrawing();
}