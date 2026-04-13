#!/bin/bash

gcc ./src/main.c ./src/game.c ./src/player.c ./src/enemy.c ./src/bullet.c ./src/barrier.c ./src/explosion.c \
    -o space-invaders \
    $(pkg-config --cflags --libs raylib) \
    -lm

if [ $? -eq 0 ]; then
    echo "Build successful: ./space-invaders"
else
    echo "Build failed"
fi