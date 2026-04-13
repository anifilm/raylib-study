#include "explosion.h"
#include <stdlib.h>
#include <math.h>

void ExplosionInit(Explosion *exp) {
    exp->active = false;
    exp->timer = 0;
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
        exp->particles[i].x = 0;
        exp->particles[i].y = 0;
        exp->particles[i].vx = 0;
        exp->particles[i].vy = 0;
        exp->particles[i].color = WHITE;
        exp->particles[i].size = 0;
    }
}

void ExplosionTrigger(Explosion *exp, float x, float y) {
    exp->active = true;
    exp->timer = EXPLOSION_DURATION;

    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
        exp->particles[i].x = x;
        exp->particles[i].y = y;

        // Random velocity in all directions
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float speed = 50 + rand() % 100;
        exp->particles[i].vx = cosf(angle) * speed;
        exp->particles[i].vy = sinf(angle) * speed;

        // Random color (warm colors for explosion)
        int colorType = rand() % 4;
        switch (colorType) {
            case 0: exp->particles[i].color = ORANGE; break;
            case 1: exp->particles[i].color = YELLOW; break;
            case 2: exp->particles[i].color = RED; break;
            case 3: exp->particles[i].color = WHITE; break;
        }

        exp->particles[i].size = 3 + rand() % 5;
    }
}

void ExplosionUpdate(Explosion *exp) {
    if (!exp->active) return;

    exp->timer -= GetFrameTime();
    if (exp->timer <= 0) {
        exp->active = false;
        return;
    }

    float delta = GetFrameTime();
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
        exp->particles[i].x += exp->particles[i].vx * delta;
        exp->particles[i].y += exp->particles[i].vy * delta;
        exp->particles[i].size -= delta * 5;
        if (exp->particles[i].size < 0) exp->particles[i].size = 0;
    }
}

void ExplosionDraw(const Explosion *exp) {
    if (!exp->active) return;

    float alpha = exp->timer / EXPLOSION_DURATION;

    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
        if (exp->particles[i].size > 0) {
            Color c = exp->particles[i].color;
            c.a = (unsigned char)(alpha * 255);
            DrawRectangle(
                exp->particles[i].x - exp->particles[i].size / 2,
                exp->particles[i].y - exp->particles[i].size / 2,
                exp->particles[i].size,
                exp->particles[i].size,
                c
            );
        }
    }
}