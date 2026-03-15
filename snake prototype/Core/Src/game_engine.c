/**
 * @file game_engine.c
 */

#include "game_engine.h"
#include "game_io.h"
#include "game_config.h"
#include "uart_protocol.h"
#include <stdlib.h>
#include <string.h>

#define MAX_SNAKE_LEN 100

typedef enum {
    DIR_UP = 0,
    DIR_DOWN = 1,
    DIR_LEFT = 2,
    DIR_RIGHT = 3,
    DIR_NONE = 255
} Direction_t;

typedef struct {
    int8_t x;
    int8_t y;
} Point_t;

static Point_t snake[MAX_SNAKE_LEN];
static uint8_t snake_len = 0;
static Point_t food;
static Direction_t curr_dir = DIR_NONE;
static Direction_t next_dir = DIR_NONE;
static uint16_t score = 0;
static uint8_t walls_initialized = 0;
static uint8_t collision_map[BOARD_WIDTH][BOARD_HEIGHT];

/* ---------------- WALL HELPERS ---------------- */

static void Add_Wall_Pair(int8_t x1, int8_t y1, int8_t x2, int8_t y2)
{
    if (x1 >= 0 && x1 < BOARD_WIDTH && y1 >= 0 && y1 < BOARD_HEIGHT)
        collision_map[x1][y1] = 1;

    if (x2 >= 0 && x2 < BOARD_WIDTH && y2 >= 0 && y2 < BOARD_HEIGHT)
        collision_map[x2][y2] = 1;

    GameIO_SendDelta((uint8_t)x1, (uint8_t)y1,
        (uint8_t)x2, (uint8_t)y2, 1);
}

static void Add_Vertical_Wall(int8_t x, int8_t y_start, int8_t y_end)
{
    for (int8_t y = y_start; y <= y_end; y += 2) {
        int8_t next_y = (y + 1 <= y_end) ? y + 1 : y;
        Add_Wall_Pair(x, y, x, next_y);
    }
}

static void Add_Horizontal_Wall(int8_t y, int8_t x_start, int8_t x_end)
{
    for (int8_t x = x_start; x <= x_end; x += 2) {
        int8_t next_x = (x + 1 <= x_end) ? x + 1 : x;
        Add_Wall_Pair(x, y, next_x, y);
    }
}

/* ---------------- LEVEL 2: CONCENTRIC LABYRINTH ---------------- */
/* ---------------- LEVEL: CONCENTRIC RINGS WITH GAPS (Reference Image) ---------------- */

/* ---------------- LEVEL: CONCENTRIC RINGS WITH GAPS (Centered) ---------------- */

static void Build_Level_Labyrinth_Exact(void)
{
    int thickness = 2; // Товщина стін залишається 2

    // ОНОВЛЕНІ КООРДИНАТИ (тепер лабіринт компактніший і далі від країв)
    int o_start = 4,  o_end = 45; // Зовнішнє кільце (було 2 і 47)
    int m_start = 10, m_end = 39; // Середнє кільце (було 9 і 40)
    int i_start = 16, i_end = 33; // Внутрішнє кільце (залишив на місці, тут достатньо простору)

    // Розриви залишаємо строго по центру
    int gap_start = 22;
    int gap_resume = 27;

    for (int i = 0; i < thickness; i++) {

        // --- ЗОВНІШНЄ КІЛЬЦЕ (Розриви ЗЛІВА і СПРАВА) ---
        Add_Horizontal_Wall(o_start + i, o_start, o_end);
        Add_Horizontal_Wall(o_end - i,   o_start, o_end);

        Add_Vertical_Wall(o_start + i, o_start, gap_start);
        Add_Vertical_Wall(o_start + i, gap_resume, o_end);

        Add_Vertical_Wall(o_end - i, o_start, gap_start);
        Add_Vertical_Wall(o_end - i, gap_resume, o_end);

        // --- СЕРЕДНЄ КІЛЬЦЕ (Розриви ЗВЕРХУ і ЗНИЗУ) ---
        Add_Vertical_Wall(m_start + i, m_start, m_end);
        Add_Vertical_Wall(m_end - i,   m_start, m_end);

        Add_Horizontal_Wall(m_start + i, m_start, gap_start);
        Add_Horizontal_Wall(m_start + i, gap_resume, m_end);

        Add_Horizontal_Wall(m_end - i, m_start, gap_start);
        Add_Horizontal_Wall(m_end - i, gap_resume, m_end);

        // --- ВНУТРІШНЄ КІЛЬЦЕ (Розриви ЗЛІВА і СПРАВА) ---
        Add_Horizontal_Wall(i_start + i, i_start, i_end);
        Add_Horizontal_Wall(i_end - i,   i_start, i_end);

        Add_Vertical_Wall(i_start + i, i_start, gap_start);
        Add_Vertical_Wall(i_start + i, gap_resume, i_end);

        Add_Vertical_Wall(i_end - i, i_start, gap_start);
        Add_Vertical_Wall(i_end - i, gap_resume, i_end);
    }
}
/* ---------------- MAZE LEVEL ---------------- */

static void Build_Maze_Level(void)
{
    // Внутрішні стіни – лабіринт
    Add_Horizontal_Wall(6, 4, 30);
    Add_Horizontal_Wall(7, 4, 30);

    Add_Vertical_Wall(30, 6, 18);
    Add_Vertical_Wall(31, 6, 18);

    Add_Vertical_Wall(10, 18, 44);
    Add_Vertical_Wall(11, 18, 44);

    Add_Horizontal_Wall(34, 4, 22);
    Add_Horizontal_Wall(35, 4, 22);

    Add_Vertical_Wall(38, 18, 46);
    Add_Vertical_Wall(39, 18, 46);

    Add_Horizontal_Wall(42, 28, 46);
    Add_Horizontal_Wall(43, 28, 46);

    Add_Horizontal_Wall(30, 30, 44);
    Add_Horizontal_Wall(31, 30, 44);

    Add_Vertical_Wall(20, 30, 46);
    Add_Vertical_Wall(21, 30, 46);

    Add_Vertical_Wall(16, 8, 20);
    Add_Vertical_Wall(17, 8, 20);

    Add_Horizontal_Wall(22, 34, 46);
    Add_Horizontal_Wall(23, 34, 46);
}

/* ---------------- LEVEL 5: SYMMETRIC CORNERS ---------------- */

/* ---------------- LEVEL 5: SYMMETRIC CORNERS (C-SHAPES) ---------------- */

static void Build_Level_5_Symmetric_Corners(void)
{
    // Змінюй це значення (наприклад, 2 або 4) — фігура більше не зламається!
    int thickness = 2;

    for (int i = 0; i < thickness; i++) {

        // --- ЗОВНІШНІ КУТИ (Форма "C" / Дужки) ---

        // TOP-LEFT CORNER (Верхній лівий)
        Add_Vertical_Wall(7 + i, 7, 20);        // Головна ліва стіна
        Add_Horizontal_Wall(7 + i, 7, 20);      // Головна верхня стіна
        Add_Horizontal_Wall(20 - i, 7, 12);     // Нижній загин (гачок)
        Add_Vertical_Wall(20 - i, 7, 12);       // Правий загин (гачок)

        // TOP-RIGHT CORNER (Верхній правий)
        Add_Vertical_Wall(42 - i, 7, 20);       // Головна права стіна (прив'язка до краю)
        Add_Horizontal_Wall(7 + i, 29, 42);     // Головна верхня стіна
        Add_Horizontal_Wall(20 - i, 37, 42);    // Нижній загин
        Add_Vertical_Wall(29 + i, 7, 12);       // Лівий загин

        // BOTTOM-LEFT CORNER (Нижній лівий)
        Add_Vertical_Wall(7 + i, 29, 42);       // Головна ліва стіна
        Add_Horizontal_Wall(42 - i, 7, 20);     // Головна нижня стіна
        Add_Horizontal_Wall(29 + i, 7, 12);     // Верхній загин
        Add_Vertical_Wall(20 - i, 37, 42);      // Правий загин

        // BOTTOM-RIGHT CORNER (Нижній правий)
        Add_Vertical_Wall(42 - i, 29, 42);      // Головна права стіна
        Add_Horizontal_Wall(42 - i, 29, 42);    // Головна нижня стіна
        Add_Horizontal_Wall(29 + i, 37, 42);    // Верхній загин
        Add_Vertical_Wall(29 + i, 37, 42);      // Лівий загин

        // --- ВНУТРІШНІ КУТИ (Менші "C" навколо центру) ---

        // TOP-LEFT INNER
        Add_Vertical_Wall(16 + i, 16, 22);
        Add_Horizontal_Wall(16 + i, 16, 22);
        Add_Horizontal_Wall(22 - i, 16, 18);
        Add_Vertical_Wall(22 - i, 16, 18);

        // TOP-RIGHT INNER
        Add_Vertical_Wall(33 - i, 16, 22);
        Add_Horizontal_Wall(16 + i, 27, 33);
        Add_Horizontal_Wall(22 - i, 31, 33);
        Add_Vertical_Wall(27 + i, 16, 18);

        // BOTTOM-LEFT INNER
        Add_Vertical_Wall(16 + i, 27, 33);
        Add_Horizontal_Wall(33 - i, 16, 22);
        Add_Horizontal_Wall(27 + i, 16, 18);
        Add_Vertical_Wall(22 - i, 31, 33);

        // BOTTOM-RIGHT INNER
        Add_Vertical_Wall(33 - i, 27, 33);
        Add_Horizontal_Wall(33 - i, 27, 33);
        Add_Horizontal_Wall(27 + i, 31, 33);
        Add_Vertical_Wall(27 + i, 31, 33);
    }
}
/* ---------------- FOOD ---------------- */

static void Generate_Food(void)
{
    uint16_t attempts = 0;

    while (1) {
        food.x = rand() % BOARD_WIDTH;
        food.y = rand() % BOARD_HEIGHT;

        if (collision_map[food.x][food.y])
            continue;

        uint8_t on_snake = 0;

        for (int i = 0; i < snake_len; i++) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                on_snake = 1;
                break;
            }
        }

        if (!on_snake)
            return;

        attempts++;
        if (attempts > 1000)
            return;
    }
}

/* ---------------- LEVEL SETUP ---------------- */

static void Setup_Level(uint8_t level)
{
    memset(collision_map, 0, sizeof(collision_map));
    walls_initialized = 1;

    switch (level) {

    case 0:
        break;

    case 1: // Vertical Labyrinths
        Add_Vertical_Wall(15, 0, 9);
        Add_Vertical_Wall(15, 16, 34);
        Add_Vertical_Wall(15, 41, 49);
        Add_Vertical_Wall(35, 0, 19);
        Add_Vertical_Wall(35, 26, 49);
        break;

    case 2: // Enlarged Safe Cross
    	Build_Level_Labyrinth_Exact();
        break;

    case 3: // Corner Crosses
        Add_Vertical_Wall(10, 5, 15);
        Add_Horizontal_Wall(10, 5, 15);
        Add_Vertical_Wall(40, 5, 15);
        Add_Horizontal_Wall(10, 35, 45);
        Add_Vertical_Wall(10, 35, 45);
        Add_Horizontal_Wall(40, 5, 15);
        Add_Vertical_Wall(40, 35, 45);
        Add_Horizontal_Wall(40, 35, 45);
        break;

    case 4: // Maze
        Build_Maze_Level();
        break;

    case 5: // Symmetric Corners Pattern
        Build_Level_5_Symmetric_Corners();
        break;

    default:
        break;
    }
}

/* ---------------- GAME ---------------- */

static void Reset_Game(void)
{
    snake_len = 1;
    snake[0].x = START_HEAD_X;
    snake[0].y = START_HEAD_Y;
    curr_dir = DIR_RIGHT;      // поточний напрямок (не використовується до першої команди)
    next_dir = DIR_NONE;       // ← змінено: тепер чекаємо першої команди руху
    score = 0;
    walls_initialized = 0;
}

static void Start_Game(void)
{
    Reset_Game();

    uint8_t level = GameIO_GetSelectedLevel();

    Setup_Level(level);
    Generate_Food();

    GameIO_SetState(STATE_PLAYING);

    GameIO_SendDelta(snake[0].x, snake[0].y, food.x, food.y, 0);
    GameIO_SendScore(snake_len, score);
}

/* ---------------- PUBLIC API ---------------- */

void Game_Start(void)
{
    Start_Game();
}

uint16_t Game_GetSpeedMs(void)
{
    return GameIO_GetSpeed();
}

void Game_Update(void)
{
    if (GameIO_GetState() != STATE_PLAYING) return;
    if (next_dir == DIR_NONE) return;
    if (snake_len == 0) return;

    curr_dir = next_dir;

    Point_t next_head = snake[0];

    switch (curr_dir) {
    case DIR_UP: next_head.y--; break;
    case DIR_DOWN: next_head.y++; break;
    case DIR_LEFT: next_head.x--; break;
    case DIR_RIGHT: next_head.x++; break;
    default: return;
    }

    uint8_t boundary = GameIO_GetBoundaryMode();

    if (boundary == BOUNDARY_WRAP) {
        // Зациклення (вихід з одного краю, поява з іншого)
        if (next_head.x < 0) next_head.x = BOARD_WIDTH - 1;
        if (next_head.x >= BOARD_WIDTH) next_head.x = 0;
        if (next_head.y < 0) next_head.y = BOARD_HEIGHT - 1;
        if (next_head.y >= BOARD_HEIGHT) next_head.y = 0;
    } else {
        // Режим "стіни по краях" – смерть при виході за межі
        if (next_head.x < 0 || next_head.x >= BOARD_WIDTH ||
            next_head.y < 0 || next_head.y >= BOARD_HEIGHT) {
            GameIO_SetState(STATE_GAMEOVER);
            GameIO_SendState(STATE_GAMEOVER);
            return;
        }
    }

    // Перевірка зіткнення зі стінами
    if (collision_map[next_head.x][next_head.y]) {
        GameIO_SetState(STATE_GAMEOVER);
        GameIO_SendState(STATE_GAMEOVER);
        return;
    }

    uint8_t ate_food = (next_head.x == food.x && next_head.y == food.y);

    // --- ПЕРЕВІРКА НА САМОПЕРЕТИН ---
    // Якщо їмо, хвіст залишається, тому перевіряємо всі сегменти.
    // Якщо не їмо, хвіст зникне, тому його не перевіряємо.
    int check_end = ate_food ? snake_len : snake_len - 1;
    for (int i = 0; i < check_end; i++) {
        if (next_head.x == snake[i].x && next_head.y == snake[i].y) {
            GameIO_SetState(STATE_GAMEOVER);
            GameIO_SendState(STATE_GAMEOVER);
            return;
        }
    }
    // --- КІНЕЦЬ ПЕРЕВІРКИ ---

    Point_t old_tail = snake[snake_len - 1];

    for (int i = snake_len - 1; i > 0; i--)
        snake[i] = snake[i - 1];

    snake[0] = next_head;

    if (ate_food) {
        if (snake_len < MAX_SNAKE_LEN) {
            snake[snake_len] = old_tail;
            snake_len++;
        }

        score++;
        Generate_Food();

        GameIO_SendDelta(next_head.x, next_head.y, food.x, food.y, 0);
        GameIO_SendScore(snake_len, score);
    }
    else {
        GameIO_SendDelta(next_head.x, next_head.y, old_tail.x, old_tail.y, 0);
    }
}
/* ---------------- COMMANDS ---------------- */

void Game_HandleCommand(uint8_t cmd, uint8_t* payload)
{
    switch (cmd) {

    case CMD_SET_STATE:
        if (payload[0] == STATE_PLAYING &&
            (GameIO_GetState() == STATE_MENU || GameIO_GetState() == STATE_GAMEOVER)) {
            Start_Game();
        } else {
            GameIO_SetState(payload[0]);
            GameIO_SendState(payload[0]);
        }
        break;

    case CMD_MOVE:
        if (GameIO_GetState() == STATE_PLAYING) {
            Direction_t new_dir = (Direction_t)payload[0];
            // Захист від розвороту на 180 градусів
            if (snake_len > 1) {
                if ((new_dir == DIR_UP && curr_dir == DIR_DOWN) ||
                    (new_dir == DIR_DOWN && curr_dir == DIR_UP) ||
                    (new_dir == DIR_LEFT && curr_dir == DIR_RIGHT) ||
                    (new_dir == DIR_RIGHT && curr_dir == DIR_LEFT)) {
                    break; // Ігноруємо команду
                }
            }
            next_dir = new_dir;
        }
        break;

    case CMD_CONFIG:
        // Конфігурація обробляється в game_io
        break;

    default:
        break;
    }
}
