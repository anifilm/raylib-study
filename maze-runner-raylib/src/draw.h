#ifndef DRAW_H
#define DRAW_H

#include "game.h"
#include "raylib.h"

#define CELL_SIZE 32

// Colors
static const Color COLOR_WALL = (Color){44, 62, 80, 255};           // Dark blue-gray
static const Color COLOR_FLOOR = (Color){236, 240, 241, 255};       // Light gray
static const Color COLOR_PLAYER = (Color){231, 76, 60, 255};        // Red
static const Color COLOR_TREASURE = (Color){241, 196, 15, 255};     // Gold
static const Color COLOR_EXIT_LOCKED = (Color){149, 165, 166, 255}; // Gray
static const Color COLOR_EXIT_OPEN = (Color){39, 174, 96, 255};     // Green
static const Color COLOR_BG = (Color){52, 73, 94, 255};             // Darker bg

void DrawMaze(GameContext *game);
void DrawPlayerVisual(Player *player);
void DrawUI(GameContext *game);
void DrawMenu(void);
void DrawWinScreen(int score, float time);
void DrawPauseScreen(void);
void DrawGame(GameContext *game);

#endif // DRAW_H
