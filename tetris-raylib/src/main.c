#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define BOARD_COLS 10
#define BOARD_ROWS 20
#define CELL_SIZE 30
#define BOARD_X_OFFSET 50
#define BOARD_Y_OFFSET 50

#define TETROMINO_SIZE 4
#define PIECE_COUNT 7
#define INVALID_PIECE -1
#define MAX_LEVEL 20

#define LOCK_DELAY 0.5f
#define CLEAR_ANIM_DURATION 0.3f

// 낙하 간격 (초)
static const float DROP_INTERVALS[] = {
    0.80f, 0.72f, 0.63f, 0.55f, 0.47f,
    0.38f, 0.30f, 0.22f, 0.15f, 0.10f,
    0.08f, 0.08f, 0.08f, 0.07f, 0.07f,
    0.07f, 0.05f, 0.05f, 0.05f, 0.03f};

// 게임 상태
typedef enum
{
    STATE_TITLE,
    STATE_PLAYING,
    STATE_CLEARING,
    STATE_GAMEOVER
} GameState;

// 7종 테트로미노
static const int PIECES[7][4][4][4] = {
    // I
    {
        {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
        {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}},
        {{0, 0, 0, 0}, {0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}},
        {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}}},
    // O
    {
        {{1, 1}, {1, 1}, {0, 0}, {0, 0}},
        {{1, 1}, {1, 1}, {0, 0}, {0, 0}},
        {{1, 1}, {1, 1}, {0, 0}, {0, 0}},
        {{1, 1}, {1, 1}, {0, 0}, {0, 0}}},
    // T
    {
        {{0, 1, 0}, {1, 1, 1}, {0, 0, 0}, {0, 0, 0}},
        {{0, 1, 0}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}},
        {{0, 0, 0}, {1, 1, 1}, {0, 1, 0}, {0, 0, 0}},
        {{0, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}}},
    // S
    {
        {{0, 1, 1}, {1, 1, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 1, 0}, {0, 1, 1}, {0, 0, 1}, {0, 0, 0}},
        {{0, 0, 0}, {0, 1, 1}, {1, 1, 0}, {0, 0, 0}},
        {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}}},
    // Z
    {
        {{1, 1, 0}, {0, 1, 1}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}},
        {{0, 0, 0}, {1, 1, 0}, {0, 1, 1}, {0, 0, 0}},
        {{0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0}}},
    // J
    {
        {{1, 0, 0}, {1, 1, 1}, {0, 0, 0}, {0, 0, 0}},
        {{0, 1, 1}, {0, 1, 0}, {0, 1, 0}, {0, 0, 0}},
        {{0, 0, 0}, {1, 1, 1}, {0, 0, 1}, {0, 0, 0}},
        {{0, 1, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 0}}},
    // L
    {
        {{0, 0, 1}, {1, 1, 1}, {0, 0, 0}, {0, 0, 0}},
        {{0, 1, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 0}},
        {{0, 0, 0}, {1, 1, 1}, {1, 0, 0}, {0, 0, 0}},
        {{1, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 0, 0}}}};

static const Color PIECE_COLORS[7] = {
    SKYBLUE, YELLOW, PURPLE, GREEN, RED, BLUE, ORANGE};

// --- 전역 변수 ---
static int board[BOARD_ROWS][BOARD_COLS];

typedef struct
{
    int type, rot, row, col;
} Piece;

static Piece current;
static GameState state;
static bool paused;

// 낙하 & 락 딜레이
static float drop_timer;
static float lock_timer;
static bool on_ground; // 바닥/블록 위에 있고 락 대기 중인지
static int level;

// 점수
static int score;
static int total_lines;

// 다음 조각 & 홀드
static int next_piece;
static int hold_piece;
static bool can_hold;

// 7-bag
static int bag[PIECE_COUNT];
static int bag_index;

// 라인 클리어 애니메이션
static bool clearing_rows[BOARD_ROWS];
static float clear_anim_timer;
static int clearing_count;

// 사운드
static Sound snd_move;
static Sound snd_rotate;
static Sound snd_drop;
static Sound snd_clear;
static Sound snd_clear4;
static Sound snd_gameover;

// --- 사운드 생성 ---
static Sound MakeSound(float freq, float duration, float volume)
{
    int sample_rate = 44100;
    int sample_count = (int)(sample_rate * duration);
    short *data = (short *)malloc(sample_count * sizeof(short));

    for (int i = 0; i < sample_count; i++)
    {
        float t = (float)i / sample_rate;
        float envelope = 1.0f - t / duration; // 감쇠
        data[i] = (short)(32767 * volume * envelope * sinf(2 * PI * freq * t));
    }

    Wave wave = {0};
    wave.frameCount = sample_count;
    wave.sampleRate = sample_rate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave); // wave 데이터는 sound가 복사하므로 해제 가능
    return sound;
}

// 상승음 효과 (라인 클리어용)
static Sound MakeRiseSound(float freq_start, float freq_end, float duration, float volume)
{
    int sample_rate = 44100;
    int sample_count = (int)(sample_rate * duration);
    short *data = (short *)malloc(sample_count * sizeof(short));

    for (int i = 0; i < sample_count; i++)
    {
        float t = (float)i / sample_rate;
        float progress = t / duration;
        float freq = freq_start + (freq_end - freq_start) * progress;
        float envelope = 1.0f - progress;
        data[i] = (short)(32767 * volume * envelope * sinf(2 * PI * freq * t));
    }

    Wave wave = {0};
    wave.frameCount = sample_count;
    wave.sampleRate = sample_rate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}

// 하강음 효과 (게임 오버용)
static Sound MakeFallSound(float freq_start, float freq_end, float duration, float volume)
{
    int sample_rate = 44100;
    int sample_count = (int)(sample_rate * duration);
    short *data = (short *)malloc(sample_count * sizeof(short));

    for (int i = 0; i < sample_count; i++)
    {
        float t = (float)i / sample_rate;
        float progress = t / duration;
        float freq = freq_start + (freq_end - freq_start) * progress;
        float envelope = 1.0f - progress * 0.5f;
        data[i] = (short)(32767 * volume * envelope * sinf(2 * PI * freq * t));
    }

    Wave wave = {0};
    wave.frameCount = sample_count;
    wave.sampleRate = sample_rate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}

static void InitSounds(void)
{
    snd_move = MakeSound(200, 0.05f, 0.15f);
    snd_rotate = MakeSound(400, 0.08f, 0.15f);
    snd_drop = MakeSound(150, 0.1f, 0.2f);
    snd_clear = MakeRiseSound(300, 600, 0.2f, 0.2f);
    snd_clear4 = MakeRiseSound(400, 900, 0.4f, 0.25f);
    snd_gameover = MakeFallSound(500, 100, 0.6f, 0.25f);
}

static void UnloadSounds(void)
{
    UnloadSound(snd_move);
    UnloadSound(snd_rotate);
    UnloadSound(snd_drop);
    UnloadSound(snd_clear);
    UnloadSound(snd_clear4);
    UnloadSound(snd_gameover);
}

// --- 7-bag ---
static void ShuffleBag(void)
{
    for (int i = PIECE_COUNT - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int tmp = bag[i];
        bag[i] = bag[j];
        bag[j] = tmp;
    }
    bag_index = 0;
}

static int NextFromBag(void)
{
    if (bag_index >= PIECE_COUNT)
        ShuffleBag();
    return bag[bag_index++];
}

// --- 충돌 ---
static bool Collides(int type, int rot, int row, int col)
{
    for (int r = 0; r < TETROMINO_SIZE; r++)
    {
        for (int c = 0; c < TETROMINO_SIZE; c++)
        {
            if (!PIECES[type][rot][r][c])
                continue;
            int br = row + r, bc = col + c;
            if (bc < 0 || bc >= BOARD_COLS || br >= BOARD_ROWS)
                return true;
            if (br < 0)
                continue;
            if (board[br][bc] != 0)
                return true;
        }
    }
    return false;
}

// --- 조각 ---
static void SpawnPiece(void)
{
    current.type = next_piece;
    current.rot = 0;
    current.row = 0;
    current.col = BOARD_COLS / 2 - TETROMINO_SIZE / 2;
    can_hold = true;
    on_ground = false;
    lock_timer = 0.0f;

    next_piece = NextFromBag();

    if (Collides(current.type, current.rot, current.row, current.col))
    {
        state = STATE_GAMEOVER;
        PlaySound(snd_gameover);
    }
}

static void LockPiece(void)
{
    for (int r = 0; r < TETROMINO_SIZE; r++)
    {
        for (int c = 0; c < TETROMINO_SIZE; c++)
        {
            if (!PIECES[current.type][current.rot][r][c])
                continue;
            int br = current.row + r, bc = current.col + c;
            if (br >= 0 && br < BOARD_ROWS && bc >= 0 && bc < BOARD_COLS)
                board[br][bc] = current.type + 1;
        }
    }
}

// 완성된 줄 찾기 (삭제는 않고 플래그만 세팅)
static int FindFullRows(void)
{
    int count = 0;
    for (int r = 0; r < BOARD_ROWS; r++)
        clearing_rows[r] = false;

    for (int r = BOARD_ROWS - 1; r >= 0; r--)
    {
        bool full = true;
        for (int c = 0; c < BOARD_COLS; c++)
        {
            if (board[r][c] == 0)
            {
                full = false;
                break;
            }
        }
        if (full)
        {
            clearing_rows[r] = true;
            count++;
        }
    }
    return count;
}

// 실제 줄 삭제 & 점수
static void RemoveClearedRows(void)
{
    int cleared = 0;

    for (int r = BOARD_ROWS - 1; r >= 0; r--)
    {
        if (!clearing_rows[r])
            continue;

        cleared++;
        for (int above = r; above > 0; above--)
            for (int c = 0; c < BOARD_COLS; c++)
                board[above][c] = board[above - 1][c];
        for (int c = 0; c < BOARD_COLS; c++)
            board[0][c] = 0;
        // 당겨진 줄을 다시 검사하기 위해 r 유지
        // clearing_rows는 이미 위에서 아래로 스캔했으므로 단순히 삭제만
    }

    if (cleared > 0)
    {
        const int line_scores[] = {0, 40, 100, 300, 1200};
        score += line_scores[cleared] * level;
        total_lines += cleared;

        int new_level = total_lines / 10 + 1;
        if (new_level > MAX_LEVEL)
            new_level = MAX_LEVEL;
        level = new_level;
    }
}

// 락 후 줄 클리어 애니메이션 시작
static void LockAndStartClear(void)
{
    LockPiece();

    int full = FindFullRows();
    if (full > 0)
    {
        clearing_count = full;
        clear_anim_timer = 0.0f;
        if (full == 4)
            PlaySound(snd_clear4);
        else
            PlaySound(snd_clear);
        state = STATE_CLEARING;
    }
    else
    {
        SpawnPiece();
    }
}

// --- 조작 ---
static void MoveHorizontal(int dir)
{
    if (!Collides(current.type, current.rot, current.row, current.col + dir))
    {
        current.col += dir;
        PlaySound(snd_move);
        // 바닥 위에서 이동하면 락 타이머 리셋
        if (on_ground)
            lock_timer = 0.0f;
    }
}

static void Rotate(int dir)
{
    int new_rot = (current.rot + dir + 4) % 4;
    if (!Collides(current.type, new_rot, current.row, current.col))
    {
        current.rot = new_rot;
        PlaySound(snd_rotate);
        if (on_ground)
            lock_timer = 0.0f;
        return;
    }
    for (int kick = 1; kick <= 2; kick++)
    {
        if (!Collides(current.type, new_rot, current.row, current.col - kick))
        {
            current.rot = new_rot;
            current.col -= kick;
            PlaySound(snd_rotate);
            if (on_ground)
                lock_timer = 0.0f;
            return;
        }
        if (!Collides(current.type, new_rot, current.row, current.col + kick))
        {
            current.rot = new_rot;
            current.col += kick;
            PlaySound(snd_rotate);
            if (on_ground)
                lock_timer = 0.0f;
            return;
        }
    }
}

static bool SoftDrop(void)
{
    if (!Collides(current.type, current.rot, current.row + 1, current.col))
    {
        current.row++;
        return true;
    }
    return false;
}

static int HardDrop(void)
{
    int dropped = 0;
    while (!Collides(current.type, current.rot, current.row + 1, current.col))
    {
        current.row++;
        dropped++;
    }
    return dropped;
}

static int GetGhostRow(void)
{
    int ghost_row = current.row;
    while (!Collides(current.type, current.rot, ghost_row + 1, current.col))
        ghost_row++;
    return ghost_row;
}

static void Hold(void)
{
    if (!can_hold)
        return;
    int cur_type = current.type;

    if (hold_piece == INVALID_PIECE)
    {
        hold_piece = cur_type;
        SpawnPiece();
    }
    else
    {
        int tmp = hold_piece;
        hold_piece = cur_type;
        current.type = tmp;
        current.rot = 0;
        current.row = 0;
        current.col = BOARD_COLS / 2 - TETROMINO_SIZE / 2;
        on_ground = false;
        lock_timer = 0.0f;
    }
    can_hold = false;
    PlaySound(snd_rotate);
}

// --- 그리기 ---
static void DrawMiniPiece(int type, int cx, int cy, int cell_size)
{
    if (type == INVALID_PIECE)
        return;
    Color color = PIECE_COLORS[type];

    int min_r = TETROMINO_SIZE, max_r = 0;
    int min_c = TETROMINO_SIZE, max_c = 0;
    for (int r = 0; r < TETROMINO_SIZE; r++)
        for (int c = 0; c < TETROMINO_SIZE; c++)
            if (PIECES[type][0][r][c])
            {
                if (r < min_r)
                    min_r = r;
                if (r > max_r)
                    max_r = r;
                if (c < min_c)
                    min_c = c;
                if (c > max_c)
                    max_c = c;
            }

    int pw = (max_c - min_c + 1) * cell_size;
    int ph = (max_r - min_r + 1) * cell_size;
    int sx = cx - pw / 2, sy = cy - ph / 2;

    for (int r = min_r; r <= max_r; r++)
        for (int c = min_c; c <= max_c; c++)
            if (PIECES[type][0][r][c])
            {
                int x = sx + (c - min_c) * cell_size;
                int y = sy + (r - min_r) * cell_size;
                DrawRectangle(x + 1, y + 1, cell_size - 2, cell_size - 2, color);
            }
}

static void DrawBoard(void)
{
    DrawRectangle(BOARD_X_OFFSET, BOARD_Y_OFFSET,
                  BOARD_COLS * CELL_SIZE, BOARD_ROWS * CELL_SIZE, DARKGRAY);

    for (int r = 0; r < BOARD_ROWS; r++)
    {
        for (int c = 0; c < BOARD_COLS; c++)
        {
            int x = BOARD_X_OFFSET + c * CELL_SIZE;
            int y = BOARD_Y_OFFSET + r * CELL_SIZE;

            if (board[r][c] != 0)
            {
                // 라인 클리어 애니메이션: 깜빡임
                if (state == STATE_CLEARING && clearing_rows[r])
                {
                    float flash = sinf(clear_anim_timer / CLEAR_ANIM_DURATION * PI * 4);
                    if (flash > 0)
                        DrawRectangle(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2, WHITE);
                    else
                        DrawRectangle(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2, PIECE_COLORS[board[r][c] - 1]);
                }
                else
                {
                    DrawRectangle(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2, PIECE_COLORS[board[r][c] - 1]);
                }
            }
            else
            {
                DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, GRAY);
            }
        }
    }

    DrawRectangleLines(BOARD_X_OFFSET, BOARD_Y_OFFSET,
                       BOARD_COLS * CELL_SIZE, BOARD_ROWS * CELL_SIZE, WHITE);
}

static void DrawGhost(void)
{
    int ghost_row = GetGhostRow();
    Color color = PIECE_COLORS[current.type];
    Color ghost_color = Fade(color, 0.15f);
    Color ghost_line_color = Fade(color, 0.7f);

    for (int r = 0; r < TETROMINO_SIZE; r++)
        for (int c = 0; c < TETROMINO_SIZE; c++)
        {
            if (!PIECES[current.type][current.rot][r][c])
                continue;
            int br = ghost_row + r, bc = current.col + c;
            if (br >= 0 && br < BOARD_ROWS && bc >= 0 && bc < BOARD_COLS)
            {
                int x = BOARD_X_OFFSET + bc * CELL_SIZE;
                int y = BOARD_Y_OFFSET + br * CELL_SIZE;
                DrawRectangle(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2, ghost_color);
                DrawRectangleLines(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2, ghost_line_color);
            }
        }
}

static void DrawPiece(void)
{
    Color color = PIECE_COLORS[current.type];
    for (int r = 0; r < TETROMINO_SIZE; r++)
        for (int c = 0; c < TETROMINO_SIZE; c++)
        {
            if (!PIECES[current.type][current.rot][r][c])
                continue;
            int br = current.row + r, bc = current.col + c;
            if (br >= 0 && br < BOARD_ROWS && bc >= 0 && bc < BOARD_COLS)
            {
                int x = BOARD_X_OFFSET + bc * CELL_SIZE;
                int y = BOARD_Y_OFFSET + br * CELL_SIZE;
                DrawRectangle(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2, color);
            }
        }
}

static void DrawUI(void)
{
    int panel_x = BOARD_X_OFFSET + BOARD_COLS * CELL_SIZE + 30;
    int panel_y = BOARD_Y_OFFSET;

    DrawText("HOLD", panel_x, panel_y, 20, GRAY);
    DrawRectangleLines(panel_x - 2, panel_y + 25, 80, 60, GRAY);
    if (!can_hold && hold_piece != INVALID_PIECE)
        DrawMiniPiece(hold_piece, panel_x + 38, panel_y + 55, 18);
    else
        DrawMiniPiece(hold_piece, panel_x + 38, panel_y + 55, 18);

    DrawText("NEXT", panel_x, panel_y + 100, 20, GRAY);
    DrawRectangleLines(panel_x - 2, panel_y + 125, 80, 60, GRAY);
    DrawMiniPiece(next_piece, panel_x + 38, panel_y + 155, 18);

    DrawText("SCORE", panel_x, panel_y + 210, 20, GRAY);
    DrawText(TextFormat("%d", score), panel_x, panel_y + 235, 28, WHITE);

    DrawText("LEVEL", panel_x, panel_y + 280, 20, GRAY);
    DrawText(TextFormat("%d", level), panel_x, panel_y + 305, 28, WHITE);

    DrawText("LINES", panel_x, panel_y + 350, 20, GRAY);
    DrawText(TextFormat("%d", total_lines), panel_x, panel_y + 375, 28, WHITE);

    int help_y = panel_y + 430;
    DrawText("CONTROLS", panel_x, help_y, 18, GRAY);
    DrawText("LEFT/RIGHT  Move", panel_x, help_y + 25, 14, DARKGRAY);
    DrawText("X           Rotate CW", panel_x, help_y + 44, 14, DARKGRAY);
    DrawText("Z / UP      Rotate CCW", panel_x, help_y + 63, 14, DARKGRAY);
    DrawText("DOWN        Soft Drop", panel_x, help_y + 82, 14, DARKGRAY);
    DrawText("SPACE       Hard Drop", panel_x, help_y + 101, 14, DARKGRAY);
    DrawText("C           Hold", panel_x, help_y + 120, 14, DARKGRAY);
    DrawText("P / ESC     Pause", panel_x, help_y + 139, 14, DARKGRAY);
}

static void DrawTitleScreen(void)
{
    int cx = 400, cy = 280;

    int title_size = 60;
    int title_w = MeasureText("TETRIS", title_size);
    DrawText("TETRIS", cx - title_w / 2, cy - 80, title_size, WHITE);

    const int deco[] = {0, 1, 2, 3, 4, 5, 6};
    for (int i = 0; i < 7; i++)
        DrawRectangle(cx - 105 + i * 30, cy - 10, 28, 28, PIECE_COLORS[deco[i]]);

    if ((int)(GetTime() * 2) % 2 == 0)
        DrawText("Press ENTER to Start", cx - 130, cy + 60, 24, YELLOW);

    int col1_x = cx - 195;
    int col2_x = cx + 10;

    DrawText("LEFT/RIGHT  Move", col1_x, cy + 120, 16, GRAY);
    DrawText("X           Rotate CW", col2_x, cy + 120, 16, GRAY);
    DrawText("Z / UP      Rotate CCW", col1_x, cy + 145, 16, GRAY);
    DrawText("DOWN        Soft Drop", col2_x, cy + 145, 16, GRAY);
    DrawText("SPACE       Hard Drop", col1_x, cy + 170, 16, GRAY);
    DrawText("C           Hold", col2_x, cy + 170, 16, GRAY);
    DrawText("P / ESC     Pause", col1_x, cy + 195, 16, GRAY);
}

static void DrawPauseOverlay(void)
{
    DrawRectangle(BOARD_X_OFFSET, BOARD_Y_OFFSET,
                  BOARD_COLS * CELL_SIZE, BOARD_ROWS * CELL_SIZE, Fade(BLACK, 0.6f));

    int cx = BOARD_X_OFFSET + BOARD_COLS * CELL_SIZE / 2;
    int cy = BOARD_Y_OFFSET + BOARD_ROWS * CELL_SIZE / 2;

    int paused_w = MeasureText("PAUSED", 40);
    DrawText("PAUSED", cx - paused_w / 2, cy - 30, 40, WHITE);
    if ((int)(GetTime() * 2) % 2 == 0)
        DrawText("Press P to Resume", cx - 100, cy + 25, 20, YELLOW);
}

// --- 초기화 ---
static void ResetGame(void)
{
    for (int r = 0; r < BOARD_ROWS; r++)
        for (int c = 0; c < BOARD_COLS; c++)
            board[r][c] = 0;

    level = 1;
    score = 0;
    total_lines = 0;
    drop_timer = 0.0f;
    lock_timer = 0.0f;
    on_ground = false;
    hold_piece = INVALID_PIECE;
    can_hold = true;
    paused = false;

    for (int r = 0; r < BOARD_ROWS; r++)
        clearing_rows[r] = false;
    clearing_count = 0;
    clear_anim_timer = 0.0f;

    ShuffleBag();
    next_piece = NextFromBag();
    SpawnPiece();
    state = STATE_PLAYING;
}

int main(void)
{
    InitWindow(800, 700, "Tetris");
    SetExitKey(0); // ESC 기본 종료 동작 비활성화
    SetTargetFPS(60);
    InitAudioDevice();

    srand((unsigned int)time(NULL));
    for (int i = 0; i < PIECE_COUNT; i++)
        bag[i] = i;

    InitSounds();
    state = STATE_TITLE;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // --- 타이틀 ---
        if (state == STATE_TITLE)
        {
            if (IsKeyPressed(KEY_ENTER))
                ResetGame();
        }

        // --- 게임 플레이 ---
        if (state == STATE_PLAYING && !paused)
        {
            // 일시정지
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
            {
                paused = true;
            }
            else
            {
                // 입력
                if (IsKeyPressed(KEY_LEFT))
                    MoveHorizontal(-1);
                if (IsKeyPressed(KEY_RIGHT))
                    MoveHorizontal(1);
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_Z))
                    Rotate(-1);
                if (IsKeyPressed(KEY_X))
                    Rotate(1);
                if (IsKeyPressed(KEY_C))
                    Hold();
                if (IsKeyPressed(KEY_DOWN))
                {
                    if (SoftDrop())
                        score += 1;
                }
                if (IsKeyPressed(KEY_SPACE))
                {
                    int dropped = HardDrop();
                    score += dropped * 2;
                    PlaySound(snd_drop);
                    LockAndStartClear();
                    drop_timer = 0.0f;
                    on_ground = false;
                    lock_timer = 0.0f;
                }

                // 바닥 확인
                if (Collides(current.type, current.rot, current.row + 1, current.col))
                {
                    if (!on_ground)
                    {
                        on_ground = true;
                        lock_timer = 0.0f;
                    }
                }
                else
                {
                    on_ground = false;
                    lock_timer = 0.0f;
                }

                // 락 딜레이
                if (on_ground)
                {
                    lock_timer += dt;
                    if (lock_timer >= LOCK_DELAY)
                    {
                        LockAndStartClear();
                        drop_timer = 0.0f;
                        on_ground = false;
                        lock_timer = 0.0f;
                    }
                }

                // 자동 낙하
                drop_timer += dt;
                float interval = DROP_INTERVALS[level < MAX_LEVEL ? level - 1 : MAX_LEVEL - 1];
                if (drop_timer >= interval)
                {
                    drop_timer = 0.0f;
                    if (!SoftDrop())
                    {
                        // 낙하로 바닥 도달 시 락 딜레이 시작 (이미 on_ground면 타이머 진행 중)
                        if (!on_ground)
                        {
                            on_ground = true;
                            lock_timer = 0.0f;
                        }
                    }
                }
            }
        }

        // 일시정지 해제
        else if (paused && (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)))
        {
            paused = false;
        }

        // --- 클리어 애니메이션 ---
        if (state == STATE_CLEARING)
        {
            clear_anim_timer += dt;
            if (clear_anim_timer >= CLEAR_ANIM_DURATION)
            {
                RemoveClearedRows();
                SpawnPiece();
                state = STATE_PLAYING;
            }
        }

        // --- 게임 오버 ---
        if (state == STATE_GAMEOVER && IsKeyPressed(KEY_R))
            ResetGame();

        // --- 그리기 ---
        BeginDrawing();
        ClearBackground(BLACK);

        if (state == STATE_TITLE)
        {
            DrawTitleScreen();
        }
        else
        {
            DrawBoard();
            DrawUI();

            if (state == STATE_PLAYING || state == STATE_CLEARING)
            {
                if (state == STATE_PLAYING && !paused)
                {
                    if (level == 1)
                        DrawGhost();
                    DrawPiece();
                }
                else if (state == STATE_PLAYING && paused)
                {
                    if (level == 1)
                        DrawGhost();
                    DrawPiece();
                    DrawPauseOverlay();
                }
            }
            else if (state == STATE_GAMEOVER)
            {
                DrawRectangle(BOARD_X_OFFSET, BOARD_Y_OFFSET + 200,
                              BOARD_COLS * CELL_SIZE, 150, Fade(BLACK, 0.7f));
                DrawText("GAME OVER",
                         BOARD_X_OFFSET + 50, BOARD_Y_OFFSET + 225, 36, RED);
                DrawText("Press R to Restart",
                         BOARD_X_OFFSET + 30, BOARD_Y_OFFSET + 275, 20, WHITE);
                DrawText(TextFormat("Score: %d", score),
                         BOARD_X_OFFSET + 55, BOARD_Y_OFFSET + 310, 22, YELLOW);
            }
        }

        EndDrawing();
    }

    UnloadSounds();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
