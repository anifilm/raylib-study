#include "bullet.h"
#include "game.h"

void BulletInitAll(Bullet *bullets, int count) {
    for (int i = 0; i < count; i++) {
        bullets[i].x = 0;
        bullets[i].y = 0;
        bullets[i].width = BULLET_WIDTH;
        bullets[i].height = BULLET_HEIGHT;
        bullets[i].speed = 0;
        bullets[i].active = false;
    }
}

void BulletUpdateAll(Bullet *bullets, int count) {
    for (int i = 0; i < count; i++) {
        if (bullets[i].active) {
            bullets[i].y += bullets[i].speed;

            if (bullets[i].y < -BULLET_HEIGHT || bullets[i].y > SCREEN_HEIGHT) {
                bullets[i].active = false;
            }
        }
    }
}

void BulletDrawAll(Bullet *bullets, int count) {
    for (int i = 0; i < count; i++) {
        if (bullets[i].active) {
            Color color = (bullets[i].speed < 0) ? GREEN : RED;
            DrawRectangle(bullets[i].x, bullets[i].y, bullets[i].width, bullets[i].height, color);
        }
    }
}