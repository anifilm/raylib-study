#ifndef BARRIER_H
#define BARRIER_H

#include "raylib.h"

#define BARRIER_COUNT 4
#define BARRIER_BLOCK_SIZE 8
#define BARRIER_WIDTH 11
#define BARRIER_HEIGHT 8

typedef struct {
    bool active;
} BarrierBlock;

typedef struct {
    float x;
    float y;
    BarrierBlock blocks[BARRIER_HEIGHT][BARRIER_WIDTH];
} Barrier;

void BarrierInitAll(Barrier barriers[BARRIER_COUNT]);
void BarrierDrawAll(Barrier barriers[BARRIER_COUNT]);
bool BarrierCheckCollision(Barrier barriers[BARRIER_COUNT], Rectangle rect);

#endif