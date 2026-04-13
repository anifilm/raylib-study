#include "enemy.h"
#include "bullet.h"
#include "game.h"
#include <stdlib.h>
#include <time.h>

static int moveDirection = 1;
static float moveTimer = 0;
static float moveInterval = 0.5f;
static int currentFrame = 0;

void EnemyInitAll(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]) {
    int startX = 50;
    int startY = 80;
    int spacingX = 55;
    int spacingY = 45;

    for (int row = 0; row < ENEMY_ROWS; row++) {
        for (int col = 0; col < ENEMY_COLS; col++) {
            enemies[row][col].x = startX + col * spacingX;
            enemies[row][col].y = startY + row * spacingY;
            enemies[row][col].width = ENEMY_WIDTH;
            enemies[row][col].height = ENEMY_HEIGHT;
            enemies[row][col].alive = true;
            enemies[row][col].type = row < 1 ? 0 : (row < 3 ? 1 : 2);
            enemies[row][col].frame = 0;
        }
    }

    moveDirection = 1;
    moveTimer = 0;
    moveInterval = 0.5f;
    currentFrame = 0;

    srand((unsigned int)time(NULL));
}

void EnemyUpdateAll(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]) {
    moveTimer += GetFrameTime();

    if (moveTimer >= moveInterval) {
        moveTimer = 0;

        // Toggle animation frame
        currentFrame = !currentFrame;

        bool shouldDrop = false;
        float rightMost = 0;
        float leftMost = SCREEN_WIDTH;

        for (int row = 0; row < ENEMY_ROWS; row++) {
            for (int col = 0; col < ENEMY_COLS; col++) {
                if (enemies[row][col].alive) {
                    if (enemies[row][col].x + enemies[row][col].width > rightMost) {
                        rightMost = enemies[row][col].x + enemies[row][col].width;
                    }
                    if (enemies[row][col].x < leftMost) {
                        leftMost = enemies[row][col].x;
                    }
                }
            }
        }

        if (moveDirection > 0 && rightMost >= SCREEN_WIDTH - 10) {
            moveDirection = -1;
            shouldDrop = true;
        } else if (moveDirection < 0 && leftMost <= 10) {
            moveDirection = 1;
            shouldDrop = true;
        }

        for (int row = 0; row < ENEMY_ROWS; row++) {
            for (int col = 0; col < ENEMY_COLS; col++) {
                if (enemies[row][col].alive) {
                    enemies[row][col].frame = currentFrame;
                    if (shouldDrop) {
                        enemies[row][col].y += ENEMY_DROP;
                    } else {
                        enemies[row][col].x += moveDirection * 10;
                    }
                }
            }
        }

        int aliveCount = 0;
        for (int row = 0; row < ENEMY_ROWS; row++) {
            for (int col = 0; col < ENEMY_COLS; col++) {
                if (enemies[row][col].alive) aliveCount++;
            }
        }
        moveInterval = 0.5f - (55 - aliveCount) * 0.008f;
        if (moveInterval < 0.1f) moveInterval = 0.1f;
    }
}

static Color GetEnemyColor(int type) {
    switch (type) {
        case 0: return RED;
        case 1: return YELLOW;
        case 2: return PURPLE;
        default: return WHITE;
    }
}

// Draw enemy type 0 (top row - small alien)
static void DrawEnemyType0(Enemy *e, Color color, int frame) {
    if (frame == 0) {
        // Frame 1: arms down
        DrawRectangle(e->x + 5, e->y, e->width - 10, e->height - 10, color);
        DrawRectangle(e->x, e->y + 5, e->width, 10, color);
        DrawTriangle(
            (Vector2){e->x + e->width / 2, e->y - 5},
            (Vector2){e->x + 8, e->y + 5},
            (Vector2){e->x + e->width - 8, e->y + 5},
            color
        );
    } else {
        // Frame 2: arms up
        DrawRectangle(e->x + 8, e->y, e->width - 16, e->height - 10, color);
        DrawRectangle(e->x + 3, e->y + 8, 10, e->height - 12, color);
        DrawRectangle(e->x + e->width - 13, e->y + 8, 10, e->height - 12, color);
        DrawTriangle(
            (Vector2){e->x + e->width / 2, e->y - 3},
            (Vector2){e->x + 10, e->y + 5},
            (Vector2){e->x + e->width - 10, e->y + 5},
            color
        );
    }
}

// Draw enemy type 1 (middle rows - medium alien)
static void DrawEnemyType1(Enemy *e, Color color, int frame) {
    if (frame == 0) {
        // Frame 1: closed
        DrawRectangle(e->x + 5, e->y, e->width - 10, 10, color);
        DrawRectangle(e->x, e->y + 10, e->width, e->height - 15, color);
        DrawRectangle(e->x + 5, e->y + e->height - 8, 8, 8, color);
        DrawRectangle(e->x + e->width - 13, e->y + e->height - 8, 8, 8, color);
    } else {
        // Frame 2: open
        DrawRectangle(e->x + 8, e->y, e->width - 16, 10, color);
        DrawRectangle(e->x + 3, e->y + 10, e->width - 6, e->height - 15, color);
        DrawRectangle(e->x, e->y + e->height - 8, 10, 8, color);
        DrawRectangle(e->x + e->width - 10, e->y + e->height - 8, 10, 8, color);
    }
}

// Draw enemy type 2 (bottom rows - large alien)
static void DrawEnemyType2(Enemy *e, Color color, int frame) {
    if (frame == 0) {
        // Frame 1: legs together
        DrawRectangle(e->x, e->y + 5, e->width, e->height - 15, color);
        DrawCircle(e->x + 10, e->y + 8, 6, color);
        DrawCircle(e->x + e->width - 10, e->y + 8, 6, color);
        DrawRectangle(e->x + 5, e->y + e->height - 10, 10, 10, color);
        DrawRectangle(e->x + e->width - 15, e->y + e->height - 10, 10, 10, color);
    } else {
        // Frame 2: legs spread
        DrawRectangle(e->x + 2, e->y + 5, e->width - 4, e->height - 15, color);
        DrawCircle(e->x + 8, e->y + 8, 5, color);
        DrawCircle(e->x + e->width - 8, e->y + 8, 5, color);
        DrawRectangle(e->x - 2, e->y + e->height - 10, 8, 10, color);
        DrawRectangle(e->x + e->width - 6, e->y + e->height - 10, 8, 10, color);
    }
}

void EnemyDrawAll(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]) {
    for (int row = 0; row < ENEMY_ROWS; row++) {
        for (int col = 0; col < ENEMY_COLS; col++) {
            if (enemies[row][col].alive) {
                Enemy *e = &enemies[row][col];
                Color color = GetEnemyColor(e->type);

                switch (e->type) {
                    case 0:
                        DrawEnemyType0(e, color, e->frame);
                        break;
                    case 1:
                        DrawEnemyType1(e, color, e->frame);
                        break;
                    case 2:
                        DrawEnemyType2(e, color, e->frame);
                        break;
                }
            }
        }
    }
}

void EnemyShoot(Enemy enemies[ENEMY_ROWS][ENEMY_COLS], Bullet *bullets) {
    static float shootTimer = 0;
    shootTimer += GetFrameTime();

    if (shootTimer >= 0.8f) {
        shootTimer = 0;

        int shootCols[ENEMY_COLS];
        int shootCount = 0;

        for (int col = 0; col < ENEMY_COLS; col++) {
            for (int row = ENEMY_ROWS - 1; row >= 0; row--) {
                if (enemies[row][col].alive) {
                    shootCols[shootCount++] = col;
                    break;
                }
            }
        }

        if (shootCount > 0 && rand() % 100 < 30) {
            int col = shootCols[rand() % shootCount];
            for (int row = ENEMY_ROWS - 1; row >= 0; row--) {
                if (enemies[row][col].alive) {
                    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
                        if (!bullets[i].active) {
                            bullets[i].active = true;
                            bullets[i].x = enemies[row][col].x + enemies[row][col].width / 2;
                            bullets[i].y = enemies[row][col].y + enemies[row][col].height;
                            bullets[i].speed = 5;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
}

bool EnemyAllDestroyed(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]) {
    for (int row = 0; row < ENEMY_ROWS; row++) {
        for (int col = 0; col < ENEMY_COLS; col++) {
            if (enemies[row][col].alive) return false;
        }
    }
    return true;
}

bool EnemyReachedBottom(Enemy enemies[ENEMY_ROWS][ENEMY_COLS]) {
    for (int row = 0; row < ENEMY_ROWS; row++) {
        for (int col = 0; col < ENEMY_COLS; col++) {
            if (enemies[row][col].alive && enemies[row][col].y + enemies[row][col].height >= SCREEN_HEIGHT - 60) {
                return true;
            }
        }
    }
    return false;
}