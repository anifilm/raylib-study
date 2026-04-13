#!/bin/bash

gcc ./src/main.c -o hello $(pkg-config --cflags --libs raylib)

if [ $? -eq 0 ]; then
    echo "Build successful: ./hello"
else
    echo "Build failed"
fi
