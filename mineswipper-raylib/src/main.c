#include "raylib.h"
#include <stdlib.h>
#include <stdbool.h>

// --- Constants ---
#define GRID_ROWS     9
#define GRID_COLS     9
#define MINE_COUNT    10
#define CELL_SIZE     32
#define GRID_OFFSET_X 16
#define GRID_OFFSET_Y 64
#define HEADER_HEIGHT 50

#define WINDOW_WIDTH  (GRID_COLS * CELL_SIZE + 2 * GRID_OFFSET_X)
#define WINDOW_HEIGHT (GRID_ROWS * CELL_SIZE + GRID_OFFSET_Y + 16)

// --- Data Structures ---
typedef struct {
    bool is_mine;
    bool is_revealed;
    bool is_flagged;
    int  adjacent_mines;
} Cell;

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_WON,
    STATE_LOST
} GameState;

typedef struct {
    Cell       grid[GRID_ROWS][GRID_COLS];
    GameState  state;
    int        flag_count;
    int        revealed_count;
    bool       mines_placed;
    double     start_time;
    double     elapsed_time;
    int        clicked_mine_row;
    int        clicked_mine_col;
    bool       clicked_mine_set;
} Game;

// --- Utility ---
static bool IsValidCell(int row, int col)
{
    return row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS;
}

static Color GetNumberColor(int count)
{
    switch (count) {
        case 1: return BLUE;
        case 2: return (Color){ 0, 128, 0, 255 };
        case 3: return RED;
        case 4: return DARKBLUE;
        case 5: return (Color){ 128, 0, 0, 255 };
        case 6: return (Color){ 0, 128, 128, 255 };
        case 7: return BLACK;
        case 8: return GRAY;
        default: return DARKGRAY;
    }
}

static bool GetCellFromMouse(int mouse_x, int mouse_y, int *row, int *col)
{
    int c = (mouse_x - GRID_OFFSET_X) / CELL_SIZE;
    int r = (mouse_y - GRID_OFFSET_Y) / CELL_SIZE;
    if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) return false;
    *row = r;
    *col = c;
    return true;
}

// --- Game Logic ---
static void InitGame(Game *game)
{
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < GRID_COLS; c++)
            game->grid[r][c] = (Cell){ false, false, false, 0 };
    game->state = STATE_MENU;
    game->flag_count = 0;
    game->revealed_count = 0;
    game->mines_placed = false;
    game->start_time = 0;
    game->elapsed_time = 0;
    game->clicked_mine_set = false;
}

static void ResetGame(Game *game)
{
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < GRID_COLS; c++)
            game->grid[r][c] = (Cell){ false, false, false, 0 };
    game->state = STATE_PLAYING;
    game->flag_count = 0;
    game->revealed_count = 0;
    game->mines_placed = false;
    game->start_time = GetTime();
    game->elapsed_time = 0;
    game->clicked_mine_set = false;
}

static void CalculateAdjacentCounts(Game *game)
{
    int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (game->grid[r][c].is_mine) continue;
            int count = 0;
            for (int d = 0; d < 8; d++) {
                int nr = r + dr[d], nc = c + dc[d];
                if (IsValidCell(nr, nc) && game->grid[nr][nc].is_mine)
                    count++;
            }
            game->grid[r][c].adjacent_mines = count;
        }
    }
}

static void PlaceMines(Game *game, int safe_row, int safe_col)
{
    int placed = 0;
    while (placed < MINE_COUNT) {
        int r = GetRandomValue(0, GRID_ROWS - 1);
        int c = GetRandomValue(0, GRID_COLS - 1);
        if (game->grid[r][c].is_mine) continue;
        if (abs(r - safe_row) <= 1 && abs(c - safe_col) <= 1) continue;
        game->grid[r][c].is_mine = true;
        placed++;
    }
    CalculateAdjacentCounts(game);
}

static bool RevealCell(Game *game, int row, int col);

static void FloodFill(Game *game, int row, int col)
{
    int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    for (int d = 0; d < 8; d++) {
        int nr = row + dr[d], nc = col + dc[d];
        if (IsValidCell(nr, nc) && !game->grid[nr][nc].is_revealed && !game->grid[nr][nc].is_flagged)
            RevealCell(game, nr, nc);
    }
}

static bool RevealCell(Game *game, int row, int col)
{
    Cell *cell = &game->grid[row][col];
    if (cell->is_revealed || cell->is_flagged) return false;

    cell->is_revealed = true;
    game->revealed_count++;

    if (cell->is_mine) return true;

    if (cell->adjacent_mines == 0)
        FloodFill(game, row, col);

    return false;
}

static void ToggleFlag(Game *game, int row, int col)
{
    Cell *cell = &game->grid[row][col];
    if (cell->is_revealed) return;
    cell->is_flagged = !cell->is_flagged;
    game->flag_count += cell->is_flagged ? 1 : -1;
}

static bool CheckWinCondition(Game *game)
{
    return game->revealed_count == GRID_ROWS * GRID_COLS - MINE_COUNT;
}

static void RevealAllMines(Game *game)
{
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < GRID_COLS; c++)
            if (game->grid[r][c].is_mine)
                game->grid[r][c].is_revealed = true;
}

// --- Drawing ---
static void DrawCell(Cell *cell, int x, int y, bool is_hovered)
{
    Color bg = (Color){ 192, 192, 192, 255 };
    Color revealed_bg = (Color){ 189, 189, 189, 255 };
    Color highlight = WHITE;
    Color shadow = (Color){ 128, 128, 128, 255 };

    if (cell->is_revealed) {
        DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, revealed_bg);
        DrawLine(x, y, x + CELL_SIZE - 1, y, shadow);
        DrawLine(x, y, x, y + CELL_SIZE - 1, shadow);

        if (cell->is_mine) {
            bool is_clicked = false;
            if (is_hovered) is_clicked = true; // handled via clicked_mine_set
            DrawRectangle(x + 1, y + 1, CELL_SIZE - 2, CELL_SIZE - 2, RED);
            DrawCircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, CELL_SIZE / 4, BLACK);
        } else if (cell->adjacent_mines > 0) {
            const char *num = TextFormat("%d", cell->adjacent_mines);
            int font_size = 20;
            int tw = MeasureText(num, font_size);
            DrawText(num, x + (CELL_SIZE - tw) / 2, y + (CELL_SIZE - font_size) / 2, font_size, GetNumberColor(cell->adjacent_mines));
        }
    } else {
        Color cell_color = bg;
        if (is_hovered && !cell->is_flagged)
            cell_color = (Color){ 200, 200, 200, 255 };

        DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, cell_color);
        // 3D raised effect
        DrawLine(x, y, x + CELL_SIZE - 1, y, highlight);
        DrawLine(x, y, x, y + CELL_SIZE - 1, highlight);
        DrawLine(x + CELL_SIZE - 1, y, x + CELL_SIZE - 1, y + CELL_SIZE - 1, shadow);
        DrawLine(x, y + CELL_SIZE - 1, x + CELL_SIZE - 1, y + CELL_SIZE - 1, shadow);

        if (cell->is_flagged) {
            // Flag pole
            int px = x + CELL_SIZE / 2 + 2;
            int top = y + 5;
            int bot = y + CELL_SIZE - 6;
            DrawLine(px, top, px, bot, BLACK);
            // Flag triangle (pointing right)
            DrawTriangle(
                (Vector2){ px, top },
                (Vector2){ px - 10, top + 5 },
                (Vector2){ px, top + 10 },
                RED
            );
            // Base
            DrawLine(px - 4, bot, px + 4, bot, BLACK);
            DrawLine(px - 2, bot, px, bot - 3, BLACK);
            DrawLine(px + 2, bot, px, bot - 3, BLACK);
        }
    }
}

static void DrawBoard(Game *game)
{
    int mouse_x = GetMouseX();
    int mouse_y = GetMouseY();
    int hover_row = -1, hover_col = -1;
    bool hovering = false;
    if (game->state == STATE_PLAYING)
        hovering = GetCellFromMouse(mouse_x, mouse_y, &hover_row, &hover_col);

    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            int x = GRID_OFFSET_X + c * CELL_SIZE;
            int y = GRID_OFFSET_Y + r * CELL_SIZE;
            bool is_hover = hovering && r == hover_row && c == hover_col;
            DrawCell(&game->grid[r][c], x, y, is_hover);
        }
    }
}

static void DrawHeader(Game *game)
{
    // Header background
    DrawRectangle(0, 0, WINDOW_WIDTH, HEADER_HEIGHT, (Color){ 192, 192, 192, 255 });
    DrawLine(0, HEADER_HEIGHT - 2, WINDOW_WIDTH, HEADER_HEIGHT - 2, (Color){ 128, 128, 128, 255 });

    // Mine counter (left)
    int remaining = MINE_COUNT - game->flag_count;
    const char *mine_text = TextFormat("%02d", remaining > 0 ? remaining : 0);
    DrawRectangle(8, 8, 56, 34, BLACK);
    DrawText(mine_text, 14, 12, 28, RED);

    // Small mine icon
    DrawCircle(72, 25, 5, BLACK);

    // Timer (right)
    int display_time = (int)game->elapsed_time;
    if (display_time > 999) display_time = 999;
    const char *timer_text = TextFormat("%03d", display_time);
    DrawRectangle(WINDOW_WIDTH - 64, 8, 56, 34, BLACK);
    DrawText(timer_text, WINDOW_WIDTH - 58, 12, 28, RED);

    // Smiley face (center) - reset button
    int cx = WINDOW_WIDTH / 2;
    int cy = HEADER_HEIGHT / 2;
    int radius = 15;

    // Button 3D effect
    DrawRectangle(cx - radius - 2, cy - radius - 2, (radius + 2) * 2, (radius + 2) * 2, (Color){ 192, 192, 192, 255 });
    DrawLine(cx - radius - 2, cy - radius - 2, cx + radius + 1, cy - radius - 2, WHITE);
    DrawLine(cx - radius - 2, cy - radius - 2, cx - radius - 2, cy + radius + 1, WHITE);
    DrawLine(cx + radius + 1, cy - radius - 2, cx + radius + 1, cy + radius + 1, (Color){ 128, 128, 128, 255 });
    DrawLine(cx - radius - 2, cy + radius + 1, cx + radius + 1, cy + radius + 1, (Color){ 128, 128, 128, 255 });

    // Face
    DrawCircle(cx, cy, radius, YELLOW);
    DrawCircle(cx - 5, cy - 3, 2, BLACK);
    DrawCircle(cx + 5, cy - 3, 2, BLACK);

    if (game->state == STATE_LOST) {
        // Dead face: X eyes
        DrawLine(cx - 7, cy - 5, cx - 3, cy - 1, BLACK);
        DrawLine(cx - 3, cy - 5, cx - 7, cy - 1, BLACK);
        DrawLine(cx + 3, cy - 5, cx + 7, cy - 1, BLACK);
        DrawLine(cx + 7, cy - 5, cx + 3, cy - 1, BLACK);
        // Frown (inverted arc with lines)
        DrawLine(cx - 5, cy + 10, cx + 5, cy + 10, BLACK);
    } else if (game->state == STATE_WON) {
        // Cool face: sunglasses
        DrawRectangle(cx - 9, cy - 5, 8, 5, BLACK);
        DrawRectangle(cx + 1, cy - 5, 8, 5, BLACK);
        DrawLine(cx - 1, cy - 3, cx + 1, cy - 3, BLACK);
        // Grin
        DrawLine(cx - 6, cy + 5, cx - 4, cy + 8, BLACK);
        DrawLine(cx - 4, cy + 8, cx + 4, cy + 8, BLACK);
        DrawLine(cx + 4, cy + 8, cx + 6, cy + 5, BLACK);
    } else {
        // Normal smile
        DrawLine(cx - 5, cy + 4, cx - 3, cy + 7, BLACK);
        DrawLine(cx - 3, cy + 7, cx + 3, cy + 7, BLACK);
        DrawLine(cx + 3, cy + 7, cx + 5, cy + 4, BLACK);
    }
}

static void DrawMenu(void)
{
    ClearBackground((Color){ 192, 192, 192, 255 });

    const char *title = "MINESWEEPER";
    int title_size = 30;
    int tw = MeasureText(title, title_size);
    DrawText(title, (WINDOW_WIDTH - tw) / 2, WINDOW_HEIGHT / 2 - 80, title_size, DARKBLUE);

    const char *info = "Beginner 9x9 - 10 mines";
    int info_size = 16;
    int iw = MeasureText(info, info_size);
    DrawText(info, (WINDOW_WIDTH - iw) / 2, WINDOW_HEIGHT / 2 - 30, info_size, BLACK);

    const char *prompt = "Click to Start";
    int prompt_size = 20;
    int pw = MeasureText(prompt, prompt_size);
    DrawText(prompt, (WINDOW_WIDTH - pw) / 2, WINDOW_HEIGHT / 2 + 20, prompt_size, DARKGRAY);

    const char *hint = "R to restart anytime";
    int hint_size = 14;
    int hw = MeasureText(hint, hint_size);
    DrawText(hint, (WINDOW_WIDTH - hw) / 2, WINDOW_HEIGHT / 2 + 60, hint_size, GRAY);
}

static void DrawEndOverlay(Game *game)
{
    // Semi-transparent overlay
    Color overlay = (Color){ 0, 0, 0, 100 };
    DrawRectangle(0, GRID_OFFSET_Y, WINDOW_WIDTH, WINDOW_HEIGHT - GRID_OFFSET_Y, overlay);

    if (game->state == STATE_WON) {
        const char *text = "YOU WIN!";
        int size = 30;
        int tw = MeasureText(text, size);
        DrawText(text, (WINDOW_WIDTH - tw) / 2, WINDOW_HEIGHT / 2 + 10, size, GREEN);
    } else {
        const char *text = "GAME OVER";
        int size = 28;
        int tw = MeasureText(text, size);
        DrawText(text, (WINDOW_WIDTH - tw) / 2, WINDOW_HEIGHT / 2 + 10, size, RED);
    }

    const char *restart = "Click or press R to restart";
    int rsize = 14;
    int rw = MeasureText(restart, rsize);
    DrawText(restart, (WINDOW_WIDTH - rw) / 2, WINDOW_HEIGHT / 2 + 50, rsize, WHITE);

    // Show wrong flags on game over
    if (game->state == STATE_LOST) {
        for (int r = 0; r < GRID_ROWS; r++) {
            for (int c = 0; c < GRID_COLS; c++) {
                Cell *cell = &game->grid[r][c];
                if (cell->is_flagged && !cell->is_mine) {
                    int x = GRID_OFFSET_X + c * CELL_SIZE;
                    int y = GRID_OFFSET_Y + r * CELL_SIZE;
                    // X over the flag
                    DrawLine(x + 4, y + 4, x + CELL_SIZE - 4, y + CELL_SIZE - 4, RED);
                    DrawLine(x + CELL_SIZE - 4, y + 4, x + 4, y + CELL_SIZE - 4, RED);
                }
            }
        }
    }
}

static void DrawGame(Game *game)
{
    ClearBackground((Color){ 192, 192, 192, 255 });
    DrawHeader(game);
    DrawBoard(game);

    if (game->state == STATE_WON || game->state == STATE_LOST)
        DrawEndOverlay(game);
}

// --- Input ---
static bool IsSmileyClicked(void)
{
    int cx = WINDOW_WIDTH / 2;
    int cy = HEADER_HEIGHT / 2;
    int mx = GetMouseX(), my = GetMouseY();
    int dx = mx - cx, dy = my - cy;
    return (dx * dx + dy * dy) <= 17 * 17;
}

static void HandleInput(Game *game)
{
    // R key to restart
    if (IsKeyPressed(KEY_R)) {
        ResetGame(game);
        return;
    }

    // End states: click or smiley to restart
    if (game->state == STATE_WON || game->state == STATE_LOST) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (IsSmileyClicked()) {
                ResetGame(game);
            }
        }
        return;
    }

    if (game->state == STATE_MENU) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ResetGame(game);
        }
        return;
    }

    // Smiley reset
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && IsSmileyClicked()) {
        ResetGame(game);
        return;
    }

    // Update timer
    game->elapsed_time = GetTime() - game->start_time;

    // Right click - flag
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        int row, col;
        if (GetCellFromMouse(GetMouseX(), GetMouseY(), &row, &col))
            ToggleFlag(game, row, col);
    }

    // Left click - reveal
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int row, col;
        if (GetCellFromMouse(GetMouseX(), GetMouseY(), &row, &col)) {
            if (!game->mines_placed) {
                PlaceMines(game, row, col);
                game->mines_placed = true;
            }

            Cell *cell = &game->grid[row][col];
            if (!cell->is_revealed && !cell->is_flagged) {
                bool hit_mine = RevealCell(game, row, col);
                if (hit_mine) {
                    game->state = STATE_LOST;
                    game->clicked_mine_row = row;
                    game->clicked_mine_col = col;
                    game->clicked_mine_set = true;
                    RevealAllMines(game);
                } else if (CheckWinCondition(game)) {
                    game->state = STATE_WON;
                    // Auto-flag remaining mines
                    for (int r = 0; r < GRID_ROWS; r++)
                        for (int c = 0; c < GRID_COLS; c++)
                            if (game->grid[r][c].is_mine && !game->grid[r][c].is_flagged) {
                                game->grid[r][c].is_flagged = true;
                                game->flag_count++;
                            }
                }
            }
        }
    }
}

// --- Main ---
int main(void)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Minesweeper");
    SetTargetFPS(60);

    Game game;
    InitGame(&game);

    while (!WindowShouldClose()) {
        HandleInput(&game);

        BeginDrawing();
        if (game.state == STATE_MENU)
            DrawMenu();
        else
            DrawGame(&game);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
