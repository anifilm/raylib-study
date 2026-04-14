#include "raylib.h"
#include <math.h>

// --- 상수 ---
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 600
#define FPS           60

#define BIRD_RADIUS   15
#define BIRD_X        80
#define GRAVITY       0.5f
#define JUMP_FORCE    -7.0f

#define PIPE_WIDTH    52
#define PIPE_CAP_H    20
#define PIPE_CAP_W    60
#define PIPE_GAP_MIN  100
#define PIPE_GAP_INIT 140
#define PIPE_SPEED_INIT 2.0f
#define PIPE_SPEED_MAX  4.0f
#define PIPE_INTERVAL 90
#define MAX_PIPES     10

#define GROUND_H      60

// --- 게임 상태 ---
typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_GAMEOVER
} GameState;

// --- 새 구조체 ---
typedef struct {
    float y;
    float velocity;
} Bird;

// --- 파이프 구조체 ---
typedef struct {
    float x;
    float gap_y;
    bool passed;
    bool active;
} Pipe;

// --- 전역: 땅 스크롤 ---
static float ground_scroll = 0.0f;

// --- 난이도 계산 ---
float GetPipeSpeed(int score)
{
    float speed = PIPE_SPEED_INIT + score * 0.05f;
    if (speed > PIPE_SPEED_MAX) speed = PIPE_SPEED_MAX;
    return speed;
}

float GetPipeGap(int score)
{
    float gap = PIPE_GAP_INIT - score * 1.5f;
    if (gap < PIPE_GAP_MIN) gap = PIPE_GAP_MIN;
    return gap;
}

void Bird_Reset(Bird *bird)
{
    bird->y = SCREEN_HEIGHT / 2.0f;
    bird->velocity = 0.0f;
}

void Bird_Jump(Bird *bird)
{
    bird->velocity = JUMP_FORCE;
}

void Bird_Update(Bird *bird)
{
    bird->velocity += GRAVITY;
    bird->y += bird->velocity;
}

void Bird_Draw(const Bird *bird)
{
    // 속도에 따른 회전 각도 (-30 ~ 90도)
    float angle = bird->velocity * 4.0f;
    if (angle < -30.0f) angle = -30.0f;
    if (angle > 90.0f)  angle = 90.0f;

    float rad = angle * PI / 180.0f;

    // 새 몸체 (회전 적용)
    Vector2 center = { BIRD_X, bird->y };

    // 날개 (타원)
    float wing_offset = sinf((float)GetTime() * 10.0f) * 4.0f;
    Vector2 wing = {
        BIRD_X + cosf(rad + PI) * 5.0f,
        bird->y + sinf(rad + PI) * 5.0f + wing_offset
    };
    DrawEllipse((int)wing.x, (int)wing.y, 12, 6, (Color){ 200, 180, 0, 255 });

    // 몸통
    DrawCircleV(center, BIRD_RADIUS, YELLOW);

    // 눈 (회전 적용)
    Vector2 eye_center = {
        BIRD_X + cosf(rad) * 6.0f + cosf(rad - PI / 2) * 4.0f,
        bird->y + sinf(rad) * 6.0f + sinf(rad - PI / 2) * 4.0f
    };
    DrawCircleV(eye_center, 5, WHITE);
    Vector2 pupil = {
        eye_center.x + cosf(rad) * 1.5f,
        eye_center.y + sinf(rad) * 1.5f
    };
    DrawCircleV(pupil, 2, BLACK);

    // 부리 (회전 적용)
    Vector2 beak1 = {
        BIRD_X + cosf(rad) * BIRD_RADIUS,
        bird->y + sinf(rad) * BIRD_RADIUS - sinf(rad) * 3.0f
    };
    Vector2 beak2 = {
        BIRD_X + cosf(rad) * (BIRD_RADIUS + 10),
        bird->y + sinf(rad) * (BIRD_RADIUS + 10)
    };
    Vector2 beak3 = {
        BIRD_X + cosf(rad) * BIRD_RADIUS,
        bird->y + sinf(rad) * BIRD_RADIUS + sinf(rad) * 3.0f
    };
    DrawTriangle(beak1, beak2, beak3, ORANGE);
}

void Pipe_ResetAll(Pipe pipes[])
{
    for (int i = 0; i < MAX_PIPES; i++)
        pipes[i].active = false;
}

int Pipe_Spawn(Pipe pipes[], float gap_y)
{
    for (int i = 0; i < MAX_PIPES; i++)
    {
        if (!pipes[i].active)
        {
            pipes[i].x = SCREEN_WIDTH + PIPE_WIDTH;
            pipes[i].gap_y = gap_y;
            pipes[i].passed = false;
            pipes[i].active = true;
            return i;
        }
    }
    return -1;
}

void Pipe_Update(Pipe pipes[], float speed)
{
    for (int i = 0; i < MAX_PIPES; i++)
    {
        if (!pipes[i].active) continue;
        pipes[i].x -= speed;
        if (pipes[i].x < -PIPE_WIDTH)
            pipes[i].active = false;
    }
}

void Pipe_Draw(const Pipe pipes[], float gap)
{
    for (int i = 0; i < MAX_PIPES; i++)
    {
        if (!pipes[i].active) continue;

        float top_h = pipes[i].gap_y - gap / 2.0f;
        float bot_y = pipes[i].gap_y + gap / 2.0f;
        float bot_h = SCREEN_HEIGHT - GROUND_H - bot_y;

        // 위 파이프 몸체
        DrawRectangle((int)pipes[i].x, 0, PIPE_WIDTH, (int)top_h, GREEN);
        // 위 파이프 캡 (뚜껑)
        DrawRectangle((int)pipes[i].x - (PIPE_CAP_W - PIPE_WIDTH) / 2,
                      (int)top_h - PIPE_CAP_H, PIPE_CAP_W, PIPE_CAP_H, DARKGREEN);

        // 아래 파이프 몸체
        DrawRectangle((int)pipes[i].x, (int)bot_y, PIPE_WIDTH, (int)bot_h, GREEN);
        // 아래 파이프 캡 (뚜껑)
        DrawRectangle((int)pipes[i].x - (PIPE_CAP_W - PIPE_WIDTH) / 2,
                      (int)bot_y, PIPE_CAP_W, PIPE_CAP_H, DARKGREEN);
    }
}

// --- 충돌 감지 ---
bool CheckCollision(const Bird *bird, const Pipe pipes[], float gap)
{
    // 화면 상단 + 땅 경계
    if (bird->y - BIRD_RADIUS < 0 || bird->y + BIRD_RADIUS > SCREEN_HEIGHT - GROUND_H)
        return true;

    for (int i = 0; i < MAX_PIPES; i++)
    {
        if (!pipes[i].active) continue;

        float bx1 = BIRD_X - BIRD_RADIUS;
        float by1 = bird->y - BIRD_RADIUS;
        float bx2 = BIRD_X + BIRD_RADIUS;
        float by2 = bird->y + BIRD_RADIUS;

        float px1 = pipes[i].x;
        float px2 = pipes[i].x + PIPE_WIDTH;
        float py2_top = pipes[i].gap_y - gap / 2.0f;
        float py1_bot = pipes[i].gap_y + gap / 2.0f;
        float py2_bot = SCREEN_HEIGHT - GROUND_H;

        if (bx2 > px1 && bx1 < px2 && by2 > 0 && by1 < py2_top)
            return true;
        if (bx2 > px1 && bx1 < px2 && by2 > py1_bot && by1 < py2_bot)
            return true;
    }
    return false;
}

// --- 배경 그리기 ---
void DrawBackground(void)
{
    // 하늘 그라데이션
    for (int y = 0; y < SCREEN_HEIGHT - GROUND_H; y++)
    {
        float t = (float)y / (SCREEN_HEIGHT - GROUND_H);
        Color c = {
            (unsigned char)(135 + (200 - 135) * t),  // R
            (unsigned char)(206 + (230 - 206) * t),  // G
            (unsigned char)(235 + (255 - 235) * t),   // B
            255
        };
        DrawLine(0, y, SCREEN_WIDTH, y, c);
    }

    // 구름 (간단한 타원)
    DrawEllipse(60, 80, 50, 20, (Color){ 255, 255, 255, 180 });
    DrawEllipse(80, 75, 40, 18, (Color){ 255, 255, 255, 180 });
    DrawEllipse(280, 120, 55, 22, (Color){ 255, 255, 255, 160 });
    DrawEllipse(310, 115, 35, 16, (Color){ 255, 255, 255, 160 });
}

// --- 땅 그리기 ---
void DrawGround(float speed)
{
    // 땅 스크롤
    ground_scroll -= speed;
    if (ground_scroll <= -24) ground_scroll = 0;

    // 갈색 땅
    DrawRectangle(0, SCREEN_HEIGHT - GROUND_H, SCREEN_WIDTH, GROUND_H, (Color){ 222, 184, 135, 255 });
    // 초록 풀
    DrawRectangle(0, SCREEN_HEIGHT - GROUND_H, SCREEN_WIDTH, 15, (Color){ 100, 200, 50, 255 });
    // 풀 패턴 (스트라이프)
    for (int x = (int)ground_scroll; x < SCREEN_WIDTH + 24; x += 24)
    {
        DrawRectangle(x, SCREEN_HEIGHT - GROUND_H + 15, 12, GROUND_H - 15, (Color){ 210, 170, 120, 255 });
    }
}

// --- 점수 팝업 효과 ---
typedef struct {
    float y;
    int timer;
    bool active;
} ScorePopup;

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy Bird");
    SetTargetFPS(FPS);

    GameState state = STATE_TITLE;
    Bird bird;
    Pipe pipes[MAX_PIPES];
    int frame_counter = 0;
    int score = 0;
    int best_score = 0;
    ScorePopup popup = { 0, 0, false };
    float current_speed = PIPE_SPEED_INIT;
    float current_gap = PIPE_GAP_INIT;

    Bird_Reset(&bird);
    Pipe_ResetAll(pipes);

    while (!WindowShouldClose())
    {
        // --- 입력 ---
        switch (state)
        {
        case STATE_TITLE:
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Bird_Reset(&bird);
                Pipe_ResetAll(pipes);
                frame_counter = 0;
                score = 0;
                ground_scroll = 0;
                current_speed = PIPE_SPEED_INIT;
                current_gap = PIPE_GAP_INIT;
                popup.active = false;
                Bird_Jump(&bird);
                state = STATE_PLAYING;
            }
            break;
        case STATE_PLAYING:
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                Bird_Jump(&bird);
            break;
        case STATE_GAMEOVER:
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                state = STATE_TITLE;
            break;
        }

        // --- 업데이트 ---
        if (state == STATE_PLAYING)
        {
            current_speed = GetPipeSpeed(score);
            current_gap = GetPipeGap(score);

            Bird_Update(&bird);
            Pipe_Update(pipes, current_speed);

            frame_counter++;
            if (frame_counter >= PIPE_INTERVAL)
            {
                float gap_y = current_gap / 2.0f + (GetRandomValue(0, SCREEN_HEIGHT - GROUND_H - (int)current_gap));
                Pipe_Spawn(pipes, gap_y);
                frame_counter = 0;
            }

            if (CheckCollision(&bird, pipes, current_gap))
            {
                if (score > best_score)
                    best_score = score;
                state = STATE_GAMEOVER;
            }

            // 점수: 파이프 통과 감지
            for (int i = 0; i < MAX_PIPES; i++)
            {
                if (!pipes[i].active || pipes[i].passed) continue;
                if (pipes[i].x + PIPE_WIDTH < BIRD_X - BIRD_RADIUS)
                {
                    pipes[i].passed = true;
                    score++;
                    // 점수 팝업 활성화
                    popup.y = bird.y - 30;
                    popup.timer = 30;
                    popup.active = true;
                }
            }
        }

        // 팝업 업데이트
        if (popup.active)
        {
            popup.y -= 1.0f;
            popup.timer--;
            if (popup.timer <= 0)
                popup.active = false;
        }

        // --- 그리기 ---
        BeginDrawing();
        DrawBackground();

        static int global_frame = 0;
        global_frame++;

        switch (state)
        {
        case STATE_TITLE:
        {
            DrawGround(PIPE_SPEED_INIT);
            // 타이틀 배경 패널
            DrawRectangle(50, 150, SCREEN_WIDTH - 100, 260, (Color){ 255, 255, 255, 200 });
            DrawRectangleLines(50, 150, SCREEN_WIDTH - 100, 260, DARKGRAY);

            DrawText("FLAPPY", SCREEN_WIDTH / 2 - MeasureText("FLAPPY", 50) / 2, 180, 50, DARKGREEN);
            DrawText("BIRD", SCREEN_WIDTH / 2 - MeasureText("BIRD", 50) / 2, 235, 50, DARKGREEN);

            // 새 미리보기 (살짝 위아래로 떠있는 효과)
            float bob = sinf((float)GetTime() * 3.0f) * 8.0f;
            DrawCircle(SCREEN_WIDTH / 2, 320 + (int)bob, BIRD_RADIUS, YELLOW);
            DrawCircle(SCREEN_WIDTH / 2 + 6, 316 + (int)bob, 4, WHITE);
            DrawCircle(SCREEN_WIDTH / 2 + 7, 316 + (int)bob, 2, BLACK);
            DrawTriangle(
                (Vector2){ SCREEN_WIDTH / 2 + BIRD_RADIUS, 317 + (int)bob },
                (Vector2){ SCREEN_WIDTH / 2 + BIRD_RADIUS + 10, 320 + (int)bob },
                (Vector2){ SCREEN_WIDTH / 2 + BIRD_RADIUS, 323 + (int)bob },
                ORANGE
            );

            if ((global_frame / 30) % 2 == 0)
                DrawText("Press SPACE", SCREEN_WIDTH / 2 - MeasureText("Press SPACE", 20) / 2, 430, 20, DARKGRAY);

            if (best_score > 0)
            {
                const char *b = TextFormat("Best: %d", best_score);
                DrawText(b, SCREEN_WIDTH / 2 - MeasureText(b, 18) / 2, 480, 18, GRAY);
            }
            break;
        }
        case STATE_PLAYING:
            Pipe_Draw(pipes, current_gap);
            Bird_Draw(&bird);
            DrawGround(current_speed);
            // 점수 HUD (외곽선)
            {
                const char *score_text = TextFormat("%d", score);
                int text_w = MeasureText(score_text, 40);
                DrawText(score_text, SCREEN_WIDTH / 2 - text_w / 2 + 2, 52, 40, BLACK);
                DrawText(score_text, SCREEN_WIDTH / 2 - text_w / 2 - 2, 48, 40, BLACK);
                DrawText(score_text, SCREEN_WIDTH / 2 - text_w / 2, 50, 40, WHITE);
            }
            // 점수 팝업
            if (popup.active)
            {
                const char *pop = "+1";
                DrawText(pop, BIRD_X + 20, (int)popup.y, 20, GOLD);
            }
            break;
        case STATE_GAMEOVER:
            Pipe_Draw(pipes, current_gap);
            Bird_Draw(&bird);
            DrawGround(0);
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 100 });
            // 게임오버 패널
            DrawRectangle(50, 170, SCREEN_WIDTH - 100, 230, (Color){ 255, 255, 255, 220 });
            DrawRectangleLines(50, 170, SCREEN_WIDTH - 100, 230, DARKGRAY);

            DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, 190, 40, RED);
            {
                const char *s = TextFormat("Score: %d", score);
                const char *b = TextFormat("Best: %d", best_score);
                DrawText(s, SCREEN_WIDTH / 2 - MeasureText(s, 28) / 2, 260, 28, BLACK);
                DrawText(b, SCREEN_WIDTH / 2 - MeasureText(b, 24) / 2, 300, 24, DARKGRAY);
            }
            if ((global_frame / 30) % 2 == 0)
                DrawText("Press SPACE", SCREEN_WIDTH / 2 - MeasureText("Press SPACE", 20) / 2, 350, 20, DARKGRAY);
            break;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
