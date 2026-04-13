#include "player.h"
#include "bullet.h"
#include "game.h"

void PlayerInit(Player *player) {
    player->width = PLAYER_WIDTH;
    player->height = PLAYER_HEIGHT;
    player->x = SCREEN_WIDTH / 2 - player->width / 2;
    player->y = SCREEN_HEIGHT - 50;
    player->speed = PLAYER_SPEED;
    player->shootTimer = 0;
}

void PlayerUpdate(Player *player) {
    // Update shoot timer
    if (player->shootTimer > 0) {
        player->shootTimer -= GetFrameTime();
    }

    if (IsKeyDown(KEY_LEFT) && player->x > 0) {
        player->x -= player->speed;
    }
    if (IsKeyDown(KEY_RIGHT) && player->x < SCREEN_WIDTH - player->width) {
        player->x += player->speed;
    }
}

void PlayerDraw(const Player *player) {
    DrawRectangle(player->x, player->y + 10, player->width, player->height - 10, GREEN);
    DrawTriangle(
        (Vector2){player->x + player->width / 2, player->y},
        (Vector2){player->x + 10, player->y + 15},
        (Vector2){player->x + player->width - 10, player->y + 15},
        GREEN
    );
}

void PlayerShoot(Player *player, Bullet *bullets) {
    // Check cooldown
    if (player->shootTimer > 0) {
        return;
    }

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].x = player->x + player->width / 2 - bullets[i].width / 2;
            bullets[i].y = player->y;
            bullets[i].speed = -8;
            player->shootTimer = PLAYER_SHOOT_COOLDOWN;
            break;
        }
    }
}