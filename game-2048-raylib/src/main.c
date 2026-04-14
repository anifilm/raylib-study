#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

// ── Layout constants ──
#define GRID_SIZE         4
#define WINDOW_WIDTH      500
#define WINDOW_HEIGHT     648
#define HEADER_HEIGHT     144
#define GRID_PADDING      12
#define CELL_GAP          12
#define CELL_SIZE         104
#define ANIM_SLIDE_FRAMES 8
#define ANIM_POP_FRAMES   8
#define FONT_PATH         "assets/Jua-Regular.ttf"

// ── Game state ──
typedef enum { STATE_PLAYING, STATE_WON, STATE_OVER } GameState;

// ── Animation: one moving tile ──
typedef struct {
    int from_r, from_c;
    int to_r, to_c;
    int value;
} TileAnim;

// ── Overall animation state ──
typedef struct {
    TileAnim tiles[16];
    int tile_count;
    int slide_frame;

    int merge_cells[8][2];
    int merge_count;
    int pop_frame;

    int spawn_r, spawn_c;
    bool has_spawn;

    bool active;
    bool sliding;
} AnimState;

typedef struct {
    int grid[GRID_SIZE][GRID_SIZE];
    int score;
    int bestScore;
    GameState state;
    bool keepPlaying;
    AnimState anim;
} Game;

// ── Colors (classic 2048 palette) ──
static Color tileColors[] = {
    {205, 193, 180, 255}, // 0  empty
    {238, 228, 218, 255}, // 2
    {237, 224, 200, 255}, // 4
    {242, 177, 121, 255}, // 8
    {245, 149,  99, 255}, // 16
    {246, 124,  95, 255}, // 32
    {246,  94,  59, 255}, // 64
    {237, 207, 114, 255}, // 128
    {237, 204,  97, 255}, // 256
    {237, 200,  80, 255}, // 512
    {237, 197,  63, 255}, // 1024
    {237, 194,  46, 255}, // 2048
};
static Color darkText  = {119, 110, 101, 255};
static Color lightText = {249, 246, 242, 255};

// ── Global font & sounds ──
static Font font;
static Sound sndMove;
static Sound sndMerge;
static Sound sndGameOver;

// ── Generate a short sine-wave sound with decay envelope ──
static Sound MakeSound(float freq, float duration, float volume) {
    unsigned int sampleRate = 44100;
    unsigned int frameCount = (unsigned int)(sampleRate * duration);
    short *data = (short *)malloc(frameCount * sizeof(short));

    for (unsigned int i = 0; i < frameCount; i++) {
        float t = (float)i / (float)sampleRate;
        float env = 1.0f - (t / duration);            // linear decay
        float sample = sinf(2.0f * PI * freq * t) * env * volume;
        data[i] = (short)(sample * 32767.0f);
    }

    Wave wave = {0};
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound sound = LoadSoundFromWave(wave);
    free(data);
    return sound;
}

// ── Helpers ──
static int GetColorIndex(int value) {
    if (value <= 0) return 0;
    int idx = 1;
    while (value > 2) { value /= 2; idx++; }
    return (idx > 11) ? 11 : idx;
}

static float EaseOut(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

// ── Pixel position of grid cell ──
static float CellX(int c) {
    return GRID_PADDING + CELL_GAP + c * (CELL_SIZE + CELL_GAP);
}
static float CellY(int r) {
    return HEADER_HEIGHT + CELL_GAP + r * (CELL_SIZE + CELL_GAP);
}

// ── Font helpers (with spacing = 0) ──
static void DrawTextF(const char *text, float x, float y, float size, Color color) {
    DrawTextEx(font, text, (Vector2){x, y}, size, 0.0f, color);
}

static float MeasureTextF(const char *text, float size) {
    return MeasureTextEx(font, text, size, 0.0f).x;
}

static void DrawTextCentered(const char *text, float cx, float cy, float size, Color color) {
    float w = MeasureTextF(text, size);
    DrawTextEx(font, text, (Vector2){cx - w / 2.0f, cy - size / 2.0f}, size, 0.0f, color);
}

// ── Spawn a random tile (90% -> 2, 10% -> 4) ──
static void SpawnTile(Game *g) {
    int empty = 0;
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            if (g->grid[r][c] == 0) empty++;
    if (empty == 0) return;

    int target = GetRandomValue(0, empty - 1);
    int count  = 0;
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            if (g->grid[r][c] == 0) {
                if (count == target) {
                    g->grid[r][c] = (GetRandomValue(1, 10) <= 9) ? 2 : 4;
                    g->anim.spawn_r = r;
                    g->anim.spawn_c = c;
                    g->anim.has_spawn = true;
                    return;
                }
                count++;
            }
        }
    }
}

// ── Init / restart ──
static void InitGame(Game *g) {
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            g->grid[r][c] = 0;
    g->score       = 0;
    g->state       = STATE_PLAYING;
    g->keepPlaying = false;
    g->anim.active = false;
    g->anim.has_spawn = false;
    SpawnTile(g);
    SpawnTile(g);
}

// ── Slide line (toward index 0) with movement tracking ──
typedef struct {
    int src_idx;
    int dst_idx;
    int value;
} LineMove;

static int SlideLine(int line[GRID_SIZE], LineMove moves[], int *move_count, bool merged_at[]) {
    *move_count = 0;
    memset(merged_at, false, GRID_SIZE * sizeof(bool));

    int srcs[GRID_SIZE], vals[GRID_SIZE];
    int count = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (line[i] != 0) {
            srcs[count] = i;
            vals[count] = line[i];
            count++;
        }
    }

    int result[GRID_SIZE] = {0};
    int gained = 0;
    int dst = 0;

    for (int i = 0; i < count; i++) {
        if (i + 1 < count && vals[i] == vals[i + 1]) {
            moves[(*move_count)++] = (LineMove){srcs[i],     dst, vals[i]};
            moves[(*move_count)++] = (LineMove){srcs[i + 1], dst, vals[i + 1]};
            result[dst] = vals[i] * 2;
            gained += vals[i] * 2;
            merged_at[dst] = true;
            dst++;
            i++;
        } else {
            moves[(*move_count)++] = (LineMove){srcs[i], dst, vals[i]};
            result[dst] = vals[i];
            dst++;
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) line[i] = result[i];
    return gained;
}

// ── Start animation after a successful move ──
static void StartAnim(Game *g) {
    g->anim.active   = true;
    g->anim.sliding  = true;
    g->anim.slide_frame = 0;
    g->anim.pop_frame   = 0;
}

// ── Direction moves with animation tracking ──
static bool MoveLeft(Game *g) {
    bool moved = false;
    AnimState *a = &g->anim;
    a->tile_count  = 0;
    a->merge_count = 0;
    a->has_spawn   = false;

    for (int r = 0; r < GRID_SIZE; r++) {
        int line[GRID_SIZE], old[GRID_SIZE];
        for (int c = 0; c < GRID_SIZE; c++) {
            line[c] = g->grid[r][c];
            old[c]  = line[c];
        }

        LineMove moves[GRID_SIZE * 2];
        int move_count;
        bool merged[GRID_SIZE];

        g->score += SlideLine(line, moves, &move_count, merged);

        for (int c = 0; c < GRID_SIZE; c++) {
            g->grid[r][c] = line[c];
            if (old[c] != line[c]) moved = true;
        }

        for (int i = 0; i < move_count; i++)
            a->tiles[a->tile_count++] = (TileAnim){r, moves[i].src_idx, r, moves[i].dst_idx, moves[i].value};

        for (int c = 0; c < GRID_SIZE; c++) {
            if (merged[c]) {
                a->merge_cells[a->merge_count][0] = r;
                a->merge_cells[a->merge_count][1] = c;
                a->merge_count++;
            }
        }
    }

    if (moved) StartAnim(g);
    return moved;
}

static bool MoveRight(Game *g) {
    bool moved = false;
    AnimState *a = &g->anim;
    a->tile_count  = 0;
    a->merge_count = 0;
    a->has_spawn   = false;

    for (int r = 0; r < GRID_SIZE; r++) {
        int line[GRID_SIZE], old[GRID_SIZE];
        for (int c = 0; c < GRID_SIZE; c++) {
            line[c] = g->grid[r][GRID_SIZE - 1 - c];
            old[c]  = g->grid[r][c];
        }

        LineMove moves[GRID_SIZE * 2];
        int move_count;
        bool merged[GRID_SIZE];

        g->score += SlideLine(line, moves, &move_count, merged);

        for (int c = 0; c < GRID_SIZE; c++) {
            g->grid[r][GRID_SIZE - 1 - c] = line[c];
            if (old[GRID_SIZE - 1 - c] != line[c]) moved = true;
        }

        for (int i = 0; i < move_count; i++)
            a->tiles[a->tile_count++] = (TileAnim){
                r, GRID_SIZE - 1 - moves[i].src_idx,
                r, GRID_SIZE - 1 - moves[i].dst_idx,
                moves[i].value};

        for (int c = 0; c < GRID_SIZE; c++) {
            if (merged[c]) {
                a->merge_cells[a->merge_count][0] = r;
                a->merge_cells[a->merge_count][1] = GRID_SIZE - 1 - c;
                a->merge_count++;
            }
        }
    }

    if (moved) StartAnim(g);
    return moved;
}

static bool MoveUp(Game *g) {
    bool moved = false;
    AnimState *a = &g->anim;
    a->tile_count  = 0;
    a->merge_count = 0;
    a->has_spawn   = false;

    for (int c = 0; c < GRID_SIZE; c++) {
        int line[GRID_SIZE], old[GRID_SIZE];
        for (int r = 0; r < GRID_SIZE; r++) {
            line[r] = g->grid[r][c];
            old[r]  = line[r];
        }

        LineMove moves[GRID_SIZE * 2];
        int move_count;
        bool merged[GRID_SIZE];

        g->score += SlideLine(line, moves, &move_count, merged);

        for (int r = 0; r < GRID_SIZE; r++) {
            g->grid[r][c] = line[r];
            if (old[r] != line[r]) moved = true;
        }

        for (int i = 0; i < move_count; i++)
            a->tiles[a->tile_count++] = (TileAnim){
                moves[i].src_idx, c,
                moves[i].dst_idx, c,
                moves[i].value};

        for (int r = 0; r < GRID_SIZE; r++) {
            if (merged[r]) {
                a->merge_cells[a->merge_count][0] = r;
                a->merge_cells[a->merge_count][1] = c;
                a->merge_count++;
            }
        }
    }

    if (moved) StartAnim(g);
    return moved;
}

static bool MoveDown(Game *g) {
    bool moved = false;
    AnimState *a = &g->anim;
    a->tile_count  = 0;
    a->merge_count = 0;
    a->has_spawn   = false;

    for (int c = 0; c < GRID_SIZE; c++) {
        int line[GRID_SIZE], old[GRID_SIZE];
        for (int r = 0; r < GRID_SIZE; r++) {
            line[r] = g->grid[GRID_SIZE - 1 - r][c];
            old[r]  = g->grid[r][c];
        }

        LineMove moves[GRID_SIZE * 2];
        int move_count;
        bool merged[GRID_SIZE];

        g->score += SlideLine(line, moves, &move_count, merged);

        for (int r = 0; r < GRID_SIZE; r++) {
            g->grid[GRID_SIZE - 1 - r][c] = line[r];
            if (old[GRID_SIZE - 1 - r] != line[r]) moved = true;
        }

        for (int i = 0; i < move_count; i++)
            a->tiles[a->tile_count++] = (TileAnim){
                GRID_SIZE - 1 - moves[i].src_idx, c,
                GRID_SIZE - 1 - moves[i].dst_idx, c,
                moves[i].value};

        for (int r = 0; r < GRID_SIZE; r++) {
            if (merged[r]) {
                a->merge_cells[a->merge_count][0] = GRID_SIZE - 1 - r;
                a->merge_cells[a->merge_count][1] = c;
                a->merge_count++;
            }
        }
    }

    if (moved) StartAnim(g);
    return moved;
}

// ── Win / lose detection ──
static bool CanMove(Game *g) {
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++) {
            if (g->grid[r][c] == 0) return true;
            if (c < GRID_SIZE - 1 && g->grid[r][c] == g->grid[r][c + 1]) return true;
            if (r < GRID_SIZE - 1 && g->grid[r][c] == g->grid[r + 1][c]) return true;
        }
    return false;
}

static bool Has2048(Game *g) {
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            if (g->grid[r][c] >= 2048) return true;
    return false;
}

// ── Draw a single tile with scale ──
static void DrawTile(float x, float y, int value, float scale) {
    float size = CELL_SIZE * scale;
    float ox   = x + (CELL_SIZE - size) / 2.0f;
    float oy   = y + (CELL_SIZE - size) / 2.0f;

    DrawRectangleRounded((Rectangle){ox, oy, size, size}, 0.12f, 4,
                         tileColors[GetColorIndex(value)]);

    char txt[10];
    sprintf(txt, "%d", value);
    float fs  = (value < 100) ? 36.0f : (value < 1000) ? 28.0f : 22.0f;
    Color tc = (value <= 4) ? darkText : lightText;
    float w  = MeasureTextF(txt, fs);
    DrawTextEx(font, txt, (Vector2){ox + (size - w) / 2.0f, oy + (size - fs) / 2.0f},
               fs, 0.0f, tc);
}

// ── Drawing ──
static void DrawGame(Game *g) {
    ClearBackground((Color){250, 248, 239, 255});

    // Title
    DrawTextF("2048", GRID_PADDING, 10, 52, darkText);

    // Score boxes
    float boxW = 100, boxH = 50, boxY = 10;
    float scoreX = WINDOW_WIDTH - GRID_PADDING - boxW * 2 - 8;
    DrawRectangleRounded((Rectangle){scoreX, boxY, boxW, boxH}, 0.15, 4,
                         (Color){187, 173, 160, 255});
    float labelW = MeasureTextF("SCORE", 14);
    DrawTextF("SCORE", scoreX + (boxW - labelW) / 2, boxY + 6, 14,
              (Color){238, 228, 218, 255});
    char sBuf[16]; sprintf(sBuf, "%d", g->score);
    float sW = MeasureTextF(sBuf, 20);
    DrawTextF(sBuf, scoreX + (boxW - sW) / 2, boxY + 25, 20, RAYWHITE);

    float bestX = WINDOW_WIDTH - GRID_PADDING - boxW;
    DrawRectangleRounded((Rectangle){bestX, boxY, boxW, boxH}, 0.15, 4,
                         (Color){187, 173, 160, 255});
    float bLabelW = MeasureTextF("BEST", 14);
    DrawTextF("BEST", bestX + (boxW - bLabelW) / 2, boxY + 6, 14,
              (Color){238, 228, 218, 255});
    char bBuf[16]; sprintf(bBuf, "%d", g->bestScore);
    float bW = MeasureTextF(bBuf, 20);
    DrawTextF(bBuf, bestX + (boxW - bW) / 2, boxY + 25, 20, RAYWHITE);

    // Instructions
    DrawTextF("Arrow keys: move | R: restart", GRID_PADDING, 78, 16,
              (Color){119, 110, 101, 160});

    // Grid background
    DrawRectangleRounded(
        (Rectangle){GRID_PADDING, HEADER_HEIGHT,
                    WINDOW_WIDTH - GRID_PADDING * 2,
                    WINDOW_WIDTH - GRID_PADDING * 2},
        0.02, 4, (Color){187, 173, 160, 255});

    // Empty cell backgrounds
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            DrawRectangleRounded(
                (Rectangle){CellX(c), CellY(r), CELL_SIZE, CELL_SIZE}, 0.12, 4,
                tileColors[0]);

    AnimState *a = &g->anim;

    if (a->active && a->sliding) {
        float t = EaseOut((float)a->slide_frame / ANIM_SLIDE_FRAMES);
        for (int i = 0; i < a->tile_count; i++) {
            TileAnim *ta = &a->tiles[i];
            float fx = CellX(ta->from_c);
            float fy = CellY(ta->from_r);
            float tx = CellX(ta->to_c);
            float ty = CellY(ta->to_r);
            float x = fx + (tx - fx) * t;
            float y = fy + (ty - fy) * t;
            DrawTile(x, y, ta->value, 1.0f);
        }
    } else {
        for (int r = 0; r < GRID_SIZE; r++) {
            for (int c = 0; c < GRID_SIZE; c++) {
                int val = g->grid[r][c];
                if (val == 0) continue;

                float scale = 1.0f;

                if (a->active && !a->sliding) {
                    float pt = (float)a->pop_frame / ANIM_POP_FRAMES;

                    for (int m = 0; m < a->merge_count; m++) {
                        if (a->merge_cells[m][0] == r && a->merge_cells[m][1] == c) {
                            scale = (pt < 0.5f)
                                ? 1.0f + 0.15f * (pt / 0.5f)
                                : 1.15f - 0.15f * ((pt - 0.5f) / 0.5f);
                        }
                    }

                    if (a->has_spawn && a->spawn_r == r && a->spawn_c == c) {
                        float st = pt * (2.0f - pt);
                        scale = st;
                    }
                }

                DrawTile(CellX(c), CellY(r), val, scale);
            }
        }
    }

    // Game over overlay
    if (g->state == STATE_OVER) {
        DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                      (Color){238, 228, 218, 150});
        DrawTextCentered("Game Over!", WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f - 30, 50, darkText);
        DrawTextCentered("Press R to restart", WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f + 20, 22, darkText);
    }

    // Win overlay
    if (g->state == STATE_WON) {
        DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                      (Color){237, 194, 46, 150});
        DrawTextCentered("You Win!", WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f - 30, 50, RAYWHITE);
        DrawTextCentered("ENTER: continue | R: restart", WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f + 20, 22, RAYWHITE);
    }
}

// ── Main ──
int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "2048");
    SetTargetFPS(60);
    SetRandomSeed((unsigned int)time(NULL));

    // Audio
    InitAudioDevice();
    sndMove     = MakeSound(200.0f, 0.08f, 0.25f);   // short soft pop
    sndMerge    = MakeSound(330.0f, 0.12f, 0.3f);    // lower ding
    sndGameOver = MakeSound(110.0f, 0.4f,  0.3f);    // deep buzz

    // Load Jua font (cute rounded Korean font)
    font = LoadFontEx(FONT_PATH, 64, NULL, 0);
    if (font.glyphCount == 0) {
        font = GetFontDefault();
    }

    Game game = {0};
    InitGame(&game);

    while (!WindowShouldClose()) {
        // ── Update animation ──
        if (game.anim.active) {
            if (game.anim.sliding) {
                game.anim.slide_frame++;
                if (game.anim.slide_frame >= ANIM_SLIDE_FRAMES) {
                    game.anim.sliding = false;
                    game.anim.pop_frame = 0;
                }
            } else {
                game.anim.pop_frame++;
                if (game.anim.pop_frame >= ANIM_POP_FRAMES)
                    game.anim.active = false;
            }
        }

        // ── Input (blocked during animation) ──
        if (game.state == STATE_PLAYING && !game.anim.active) {
            bool moved = false;
            if      (IsKeyPressed(KEY_LEFT))  moved = MoveLeft(&game);
            else if (IsKeyPressed(KEY_RIGHT)) moved = MoveRight(&game);
            else if (IsKeyPressed(KEY_UP))    moved = MoveUp(&game);
            else if (IsKeyPressed(KEY_DOWN))  moved = MoveDown(&game);

            if (moved) {
                if (game.anim.merge_count > 0) PlaySound(sndMerge);
                else PlaySound(sndMove);
                SpawnTile(&game);
                if (game.score > game.bestScore) game.bestScore = game.score;
                if (!game.keepPlaying && Has2048(&game))
                    game.state = STATE_WON;
                else if (!CanMove(&game)) {
                    game.state = STATE_OVER;
                    PlaySound(sndGameOver);
                }
            }
        } else if (game.state == STATE_WON) {
            if (IsKeyPressed(KEY_ENTER)) {
                game.keepPlaying = true;
                game.state = STATE_PLAYING;
            }
        }

        if (IsKeyPressed(KEY_R)) InitGame(&game);

        // ── Draw ──
        BeginDrawing();
        DrawGame(&game);
        EndDrawing();
    }

    UnloadSound(sndMove);
    UnloadSound(sndMerge);
    UnloadSound(sndGameOver);
    CloseAudioDevice();
    UnloadFont(font);
    CloseWindow();
    return 0;
}
