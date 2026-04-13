#ifndef EXPLOSION_H
#define EXPLOSION_H

#include "raylib.h"

#define MAX_EXPLOSION_PARTICLES 20
#define EXPLOSION_DURATION 0.5f

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    Color color;
    float size;
} Particle;

typedef struct {
    bool active;
    float timer;
    Particle particles[MAX_EXPLOSION_PARTICLES];
} Explosion;

void ExplosionInit(Explosion *exp);
void ExplosionTrigger(Explosion *exp, float x, float y);
void ExplosionUpdate(Explosion *exp);
void ExplosionDraw(const Explosion *exp);

#endif