#include "barrier.h"
#include "game.h"

// Barrier shape pattern (1 = block exists)
static const int barrierPattern[BARRIER_HEIGHT][BARRIER_WIDTH] = {
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1}
};

void BarrierInitAll(Barrier barriers[BARRIER_COUNT]) {
    int spacing = SCREEN_WIDTH / (BARRIER_COUNT + 1);

    for (int i = 0; i < BARRIER_COUNT; i++) {
        barriers[i].x = spacing * (i + 1) - (BARRIER_WIDTH * BARRIER_BLOCK_SIZE) / 2;
        barriers[i].y = SCREEN_HEIGHT - 120;

        for (int row = 0; row < BARRIER_HEIGHT; row++) {
            for (int col = 0; col < BARRIER_WIDTH; col++) {
                barriers[i].blocks[row][col].active = (barrierPattern[row][col] == 1);
            }
        }
    }
}

void BarrierDrawAll(Barrier barriers[BARRIER_COUNT]) {
    for (int i = 0; i < BARRIER_COUNT; i++) {
        for (int row = 0; row < BARRIER_HEIGHT; row++) {
            for (int col = 0; col < BARRIER_WIDTH; col++) {
                if (barriers[i].blocks[row][col].active) {
                    float bx = barriers[i].x + col * BARRIER_BLOCK_SIZE;
                    float by = barriers[i].y + row * BARRIER_BLOCK_SIZE;
                    DrawRectangle(bx, by, BARRIER_BLOCK_SIZE, BARRIER_BLOCK_SIZE, GREEN);
                }
            }
        }
    }
}

bool BarrierCheckCollision(Barrier barriers[BARRIER_COUNT], Rectangle rect) {
    for (int i = 0; i < BARRIER_COUNT; i++) {
        for (int row = 0; row < BARRIER_HEIGHT; row++) {
            for (int col = 0; col < BARRIER_WIDTH; col++) {
                if (!barriers[i].blocks[row][col].active) continue;

                float bx = barriers[i].x + col * BARRIER_BLOCK_SIZE;
                float by = barriers[i].y + row * BARRIER_BLOCK_SIZE;

                Rectangle blockRect = {bx, by, BARRIER_BLOCK_SIZE, BARRIER_BLOCK_SIZE};

                if (CheckCollisionRecs(rect, blockRect)) {
                    // Destroy the block
                    barriers[i].blocks[row][col].active = false;
                    return true;
                }
            }
        }
    }
    return false;
}