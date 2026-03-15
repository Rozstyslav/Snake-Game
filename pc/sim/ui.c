/**
 * @file ui.c
 * @brief Snake Simulator terminal UI  v4.0 (walls & level testing)
 *
 * Three-panel ANSI terminal layout:
 *   Left   (COL_L) – console / command log
 *   Middle (COL_M) – PC -> STM32 injected packets
 *   Right  (COL_R) – STM32 -> PC received packets
 *
 * All layout geometry, buffer sizes, and button simulation durations
 * are defined in sim_config.h.  Protocol command codes come from
 * uart_protocol.h; board dimensions come from game_config.h.
 */

#include "ui.h"
#include "sim_config.h"
#include "hal_stub.h"
#include "uart_protocol.h"
#include "game_config.h"
#include "game_io.h"
#include "debug.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

 /* ── ANSI escape codes ───────────────────────────────────────────────── */
#define RST    "\033[0m"
#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define GRN    "\033[92m"
#define YEL    "\033[93m"
#define CYN    "\033[96m"
#define WHT    "\033[97m"
#define RED    "\033[91m"
#define BLU    "\033[94m"
#define MAG    "\033[95m"

/* ── Line buffers (UI_LINE_HISTORY x UI_LINE_WIDTH from sim_config.h) ── */
static char lb[UI_LINE_HISTORY][UI_LINE_WIDTH];
static char mb[UI_LINE_HISTORY][UI_LINE_WIDTH];
static char rb[UI_LINE_HISTORY][UI_LINE_WIDTH];
static int  lc = 0, mc = 0, rc = 0;

/* ── Wall storage ────────────────────────────────────────────────────── */
/* UI_MAX_WALLS defined in sim_config.h */
typedef struct {
    uint8_t x1, y1, x2, y2;
} WallPair_t;

static WallPair_t walls[UI_MAX_WALLS];
static int        wall_count = 0;
static uint8_t    current_level = 0u;

/* ── External symbols ────────────────────────────────────────────────── */
extern UART_HandleTypeDef huart1;
extern uint8_t rx_byte;

/* ── Input ───────────────────────────────────────────────────────────── */
static char cmd_buf[UI_CMD_BUF_LEN];
static int  cmd_pos = 0;

/* ── Button simulation state ─────────────────────────────────────────── */
static uint32_t button_press_end = 0u;
static uint8_t  button_pressed = 0u;

/* ── Forward declarations ───────────────────────────────────────────── */
static void full_redraw(void);
static void log_l(const char* s);
static void log_m(const char* s);
static void log_r(const char* s);
static void show_level_layout(int level);
static void show_current_walls(void);
static void show_wall_map(void);
static void test_walls(void);
static void print_help(void);

/* ── Cursor helpers ──────────────────────────────────────────────────── */
static void at(int r, int c) { printf("\033[%d;%dH", r, c); }
static void eol(void) { printf("\033[K"); }
static void hide_cursor(void) { printf("\033[?25l"); }
static void show_cursor(void) { printf("\033[?25h"); }

/* ── Name helpers ────────────────────────────────────────────────────── */
static const char* sname(uint8_t s)
{
    switch (s) {
    case 0: return "MENU";
    case 1: return "PLAYING";
    case 2: return "PAUSE";
    case 3: return "GAMEOVER";
    case 4: return "SLEEP";
    default: return "?";
    }
}

static const char* dname(uint8_t d)
{
    switch (d) {
    case 0: return "OFF";
    case 1: return "RANDOM";
    case 2: return "SILENT";
    case 3: return "SCENARIO";
    case 4: return "LOOPBACK";
    case 5: return "ERR_INJ";
    case 6: return "STEP";
    default: return "?";
    }
}

static const char* level_name(uint8_t level)
{
    switch (level) {
    case 0: return "Empty";
    case 1: return "Vertical Labyrinths";
    case 2: return "Safe Cross";
    case 3: return "Corner Crosses";
    case 4: return "Maze";
    case 5: return "Symmetric Corners";
    default: return "Unknown";
    }
}

/* ── Buffer push ─────────────────────────────────────────────────────── */
static void push(char b[][UI_LINE_WIDTH], int* n, const char* s)
{
    if (*n < UI_LINE_HISTORY) {
        strncpy(b[*n], s, UI_LINE_WIDTH - 1);
        b[*n][UI_LINE_WIDTH - 1] = '\0';
        (*n)++;
    }
    else {
        for (int i = 0; i < UI_LINE_HISTORY - 1; i++) {
            memcpy(b[i], b[i + 1], UI_LINE_WIDTH);
        }
        strncpy(b[UI_LINE_HISTORY - 1], s, UI_LINE_WIDTH - 1);
        b[UI_LINE_HISTORY - 1][UI_LINE_WIDTH - 1] = '\0';
    }
}

/* ── Per-panel log functions ─────────────────────────────────────────── */
static void log_l(const char* s)
{
    push(lb, &lc, s);
    hide_cursor();
    if (lc > PANEL_ROWS) {
        int start = lc - PANEL_ROWS;
        int row = ROW_CONTENT;
        for (int i = start; i < lc && i < UI_LINE_HISTORY && row < ROW_INPUT - 1; i++, row++) {
            at(row, COL_L);
            printf("%s%-*.*s" RST, WHT, COL_L_W, COL_L_W, lb[i]);
        }
    }
    else {
        int row = ROW_CONTENT + lc - 1;
        at(row, COL_L);
        printf("%s%-*.*s" RST, WHT, COL_L_W, COL_L_W, s);
    }
}

static void log_m(const char* s)
{
    push(mb, &mc, s);
    hide_cursor();
    if (mc > PANEL_ROWS) {
        int start = mc - PANEL_ROWS;
        int row = ROW_CONTENT;
        for (int i = start; i < mc && i < UI_LINE_HISTORY && row < ROW_INPUT - 1; i++, row++) {
            at(row, COL_M);
            printf("%s%-*.*s" RST, GRN, COL_M_W, COL_M_W, mb[i]);
        }
    }
    else {
        int row = ROW_CONTENT + mc - 1;
        at(row, COL_M);
        printf("%s%-*.*s" RST, GRN, COL_M_W, COL_M_W, s);
    }
}

static void log_r(const char* s)
{
    push(rb, &rc, s);
    hide_cursor();
    if (rc > PANEL_ROWS) {
        int start = rc - PANEL_ROWS;
        int row = ROW_CONTENT;
        for (int i = start; i < rc && i < UI_LINE_HISTORY && row < ROW_INPUT - 1; i++, row++) {
            at(row, COL_R);
            printf("%s%-*.*s" RST, CYN, COL_R_W, COL_R_W, rb[i]);
        }
    }
    else {
        int row = ROW_CONTENT + rc - 1;
        at(row, COL_R);
        printf("%s%-*.*s" RST, CYN, COL_R_W, COL_R_W, s);
    }
}

/* ── Redraw one panel column ─────────────────────────────────────────── */
static void redraw_col(char b[][UI_LINE_WIDTH], int n, int col, int w, const char* clr)
{
    int start = n > PANEL_ROWS ? n - PANEL_ROWS : 0;
    int row = ROW_CONTENT;

    for (int i = start; i < n && i < UI_LINE_HISTORY && row < ROW_INPUT - 1; i++, row++) {
        at(row, col);
        printf("%s%-*.*s" RST, clr, w, w, b[i]);
    }
    for (; row < ROW_INPUT - 1; row++) {
        at(row, col);
        eol();
    }
}

/* ── Draw static frame ───────────────────────────────────────────────── */
static void draw_frame(void)
{
    /* Top border */
    at(1, 1);
    printf(DIM "╔");
    for (int c = 0; c < TERM_COLS - 2; c++) printf("═");
    printf("╗" RST);

    /* Status bar side borders */
    at(ROW_STATUS, 1);           printf(DIM "║" RST);
    at(ROW_STATUS, TERM_COLS);   printf(DIM "║" RST);

    /* Separator below status bar */
    at(3, 1); printf(DIM "╠");
    for (int c = 0; c < COL_M - 3; c++) printf("═");
    printf("╦");
    for (int c = 0; c < COL_R - COL_M - 1; c++) printf("═");
    printf("╦");
    for (int c = 0; c < (TERM_COLS - 2) - (COL_R - 1); c++) printf("═");
    printf("╣" RST);

    /* Panel headers */
    at(ROW_HDR, 1);            printf(DIM "║" RST);
    at(ROW_HDR, COL_L);        printf(BOLD YEL " %-*s" RST, COL_L_W - 1, "CONSOLE");
    at(ROW_HDR, COL_M - 1);    printf(DIM "║" RST);
    at(ROW_HDR, COL_M);        printf(BOLD GRN " %-*s" RST, COL_M_W - 1, "PC → STM32");
    at(ROW_HDR, COL_R - 1);    printf(DIM "║" RST);
    at(ROW_HDR, COL_R);        printf(BOLD CYN " %-*s" RST, COL_R_W - 1, "STM32 → PC");
    at(ROW_HDR, TERM_COLS);    printf(DIM "║" RST);

    /* Separator below panel headers */
    at(ROW_HDR + 1, 1); printf(DIM "╠");
    for (int c = 0; c < COL_M - 3; c++) printf("═");
    printf("╬");
    for (int c = 0; c < COL_R - COL_M - 1; c++) printf("═");
    printf("╬");
    for (int c = 0; c < (TERM_COLS - 2) - (COL_R - 1); c++) printf("═");
    printf("╣" RST);

    /* Side borders for content area */
    for (int r = ROW_CONTENT; r < ROW_INPUT - 1; r++) {
        at(r, 1);            printf(DIM "║" RST);
        at(r, COL_M - 1);   printf(DIM "║" RST);
        at(r, COL_R - 1);   printf(DIM "║" RST);
        at(r, TERM_COLS);   printf(DIM "║" RST);
    }

    /* Separator above input row */
    at(ROW_INPUT - 1, 1); printf(DIM "╠");
    for (int c = 0; c < TERM_COLS - 2; c++) printf("═");
    printf("╣" RST);

    /* Input row side borders */
    at(ROW_INPUT, 1);          printf(DIM "║" RST);
    at(ROW_INPUT, TERM_COLS);  printf(DIM "║" RST);

    /* Bottom border */
    at(ROW_INPUT + 1, 1); printf(DIM "╚");
    for (int c = 0; c < TERM_COLS - 2; c++) printf("═");
    printf("╝" RST);
}

/* ── Full redraw ─────────────────────────────────────────────────────── */
static void full_redraw(void)
{
    printf("\033[2J");
    draw_frame();
    redraw_col(lb, lc, COL_L, COL_L_W, WHT);
    redraw_col(mb, mc, COL_M, COL_M_W, GRN);
    redraw_col(rb, rc, COL_R, COL_R_W, CYN);
    UI_RenderStatus();
}

/* ── Show level layout description ──────────────────────────────────── */
static void show_level_layout(int level)
{
    char buf[UI_LINE_WIDTH];

    snprintf(buf, sizeof(buf), "┌─ Level %d: %s", level, level_name((uint8_t)level));
    int len = (int)strlen(buf);
    for (int i = len; i < COL_L_W - 2; i++) strcat(buf, "─");
    strcat(buf, "┐");
    log_l(buf);

    switch (level) {
    case 0:
        log_l("│  No walls, empty board               │");
        break;
    case 1:
        log_l("│  Level 1: Vertical Labyrinths        │");
        log_l("│  Left wall (X=15):                   │");
        log_l("│    Y=0-9, 16-34, 41-49              │");
        log_l("│  Right wall (X=35):                  │");
        log_l("│    Y=0-19, 26-49                     │");
        log_l("│  Gaps: Y=10-15 and Y=35-40           │");
        break;
    case 2:
        log_l("│  Level 2: Safe Cross                 │");
        log_l("│  Vertical line: X=25                 │");
        log_l("│    Y=15-23 and Y=27-35               │");
        log_l("│  Horizontal line: Y=25               │");
        log_l("│    X=15-23 and X=27-35               │");
        log_l("│  Snake starts at (25,25) - safe zone │");
        break;
    case 3:
        log_l("│  Level 3: Corner Crosses             │");
        log_l("│  Top-Left:    (5-15,5-15)            │");
        log_l("│  Top-Right:   (35-45,5-15)           │");
        log_l("│  Bottom-Left: (5-15,35-45)           │");
        log_l("│  Bottom-Right: (35-45,35-45)         │");
        log_l("│  Center is completely free           │");
        break;
    case 4:
        log_l("│  Level 4: Maze                       │");
        log_l("│  Multiple intersecting corridors     │");
        break;
    case 5:
        log_l("│  Level 5: Symmetric Corners          │");
        log_l("│  Nested C-shapes at each corner      │");
        break;
    default:
        log_l("│  (no layout description available)   │");
        break;
    }

    log_l("└─────────────────────────────────────┘");
}

/* ── Show current walls ─────────────────────────────────────────────── */
static void show_current_walls(void)
{
    char buf[UI_LINE_WIDTH];

    log_l("┌─ Current Walls ───────────────────┐");
    snprintf(buf, sizeof(buf), "│  Level: %d (%s)                 │",
        current_level, level_name(current_level));
    log_l(buf);
    log_l("├──────────────────────────────────┤");

    if (wall_count == 0) {
        log_l("│  No walls                          │");
    }
    else {
        int show_count = wall_count > UI_WALLS_DISPLAY_MAX
            ? UI_WALLS_DISPLAY_MAX : wall_count;
        for (int i = 0; i < show_count; i++) {
            snprintf(buf, sizeof(buf),
                "│  Wall %2d: (%2d,%2d) - (%2d,%2d) │",
                i + 1, walls[i].x1, walls[i].y1,
                walls[i].x2, walls[i].y2);
            log_l(buf);
        }
        if (wall_count > UI_WALLS_DISPLAY_MAX) {
            snprintf(buf, sizeof(buf), "│  ... and %d more walls           │",
                wall_count - UI_WALLS_DISPLAY_MAX);
            log_l(buf);
        }
    }

    log_l("└──────────────────────────────────┘");
}

/* ── Show ASCII map of walls ─────────────────────────────────────────── */
static void show_wall_map(void)
{
    char buf[UI_LINE_WIDTH];
    /* BOARD_WIDTH characters + null terminator + one spare byte for safety */
    char map_line[BOARD_WIDTH + 2];

    log_l("┌─ Wall Map (50x50) ────────────────────┐");
    log_l("│  X→ 0        10        20        30        40        49 │");

    for (int y = 0; y < BOARD_HEIGHT; y += 2) {
        memset(map_line, ' ', BOARD_WIDTH);
        map_line[BOARD_WIDTH] = '\0';

        for (int w = 0; w < wall_count && w < UI_MAX_WALLS; w++) {
            if (walls[w].y1 == y || walls[w].y1 == y + 1) {
                if (walls[w].x1 < BOARD_WIDTH) map_line[walls[w].x1] = '#';
            }
            if (walls[w].y2 == y || walls[w].y2 == y + 1) {
                if (walls[w].x2 < BOARD_WIDTH) map_line[walls[w].x2] = '#';
            }
        }

        /* Overlay column grid marks at UI_MAP_GRID_STEP intervals. */
        for (int x = UI_MAP_GRID_STEP; x < BOARD_WIDTH; x += UI_MAP_GRID_STEP) {
            if (map_line[x] == ' ') map_line[x] = '|';
        }

        snprintf(buf, sizeof(buf), "│Y=%2d %s │", y, map_line);
        log_l(buf);
    }

    log_l("└────────────────────────────────────────────┘");
}

/* ── Run wall tests ──────────────────────────────────────────────────── */
static void test_walls(void)
{
    log_l("┌─ Running Wall Tests ─────────────────┐");

    log_l("│  Test 1: Level 0 (no walls)         │");
    UI_InjectPacket(CMD_CONFIG, CONFIG_WALLS, 0, 0, 0, 0);
    HAL_Delay(SIM_TEST_STEP_DELAY_MS);

    log_l("│  Test 2: Level 1 (vertical labs)    │");
    UI_InjectPacket(CMD_CONFIG, CONFIG_WALLS, 1, 0, 0, 0);
    HAL_Delay(SIM_TEST_STEP_DELAY_MS);

    log_l("│  Test 3: Level 2 (safe cross)       │");
    UI_InjectPacket(CMD_CONFIG, CONFIG_WALLS, 2, 0, 0, 0);
    HAL_Delay(SIM_TEST_STEP_DELAY_MS);

    log_l("│  Starting game to see walls...      │");
    UI_InjectPacket(CMD_SET_STATE, STATE_PLAYING, 0, 0, 0, 0);

    log_l("└──────────────────────────────────────┘");
}

/* ── Packet decoder ──────────────────────────────────────────────────── */
static void decode(const uint8_t* p, char* out, int olen)
{
    uint8_t cmd, crc;
    int     i;

    if (!p || olen <= 0) return;
    if (p[0] == ACK) { snprintf(out, olen, "[ACK]");  return; }
    if (p[0] == NACK) { snprintf(out, olen, "[NACK]"); return; }
    if (p[0] != SIM_UART_PACKET_HEADER) { snprintf(out, olen, "???"); return; }

    cmd = p[UART_CMD_POS];
    crc = 0u;
    for (i = 0; i < (int)(UART_PACKET_SIZE - 1u); i++) crc ^= p[i];

    if (crc != p[UART_CRC_OFFSET]) {
        snprintf(out, olen, "CRC ERR! [%02X]", p[UART_CMD_POS]);
        return;
    }

    switch (cmd) {
    case CMD_SET_STATE:
        snprintf(out, olen, "SET_STATE %s", sname(p[2]));
        break;
    case CMD_MOVE: {
        const char* d[] = { "UP", "DOWN", "LEFT", "RIGHT" };
        snprintf(out, olen, "MOVE %s", p[2] < 4u ? d[p[2]] : "?");
        break;
    }
    case CMD_CONFIG:
        if (p[2] == CONFIG_WALLS) {
            snprintf(out, olen, "CONFIG WALLS=%d (%s)", p[3],
                p[3] == WALLS_NONE ? "none" :
                (p[3] == WALLS_EASY ? "easy" : "hard"));
        }
        else {
            snprintf(out, olen, "CONFIG [%02X=%02X]", p[2], p[3]);
        }
        break;
    case CMD_DEBUG:
        snprintf(out, olen, "DEBUG mode=%d (%s)", p[2], dname(p[2]));
        break;
    case CMD_STATE:
        snprintf(out, olen, "STATE %s", sname(p[2]));
        break;
    case CMD_DELTA:
        if (p[6] == 1u) {
            snprintf(out, olen, "WALL (%d,%d)-(%d,%d)",
                p[2], p[3], p[4], p[5]);
        }
        else {
            snprintf(out, olen, "DELTA h(%d,%d) f/t(%d,%d)",
                p[2], p[3], p[4], p[5]);
        }
        break;
    case CMD_SCORE:
        snprintf(out, olen, "SCORE len=%d val=%d",
            p[2], (int)(p[3] | (p[4] << 8u)));
        break;
    default:
        snprintf(out, olen, "CMD=%02X [%02X%02X%02X%02X%02X]",
            cmd, p[2], p[3], p[4], p[5], p[6]);
        break;
    }
}

/* ── Inject PC -> STM32 packet ───────────────────────────────────────── */
void UI_InjectPacket(uint8_t cmd, uint8_t p0, uint8_t p1,
    uint8_t p2, uint8_t p3, uint8_t p4)
{
    uint8_t pkt[UART_PACKET_SIZE];
    char    line[UI_LINE_WIDTH];
    int     i;

    pkt[UART_HEADER_POS] = SIM_UART_PACKET_HEADER;
    pkt[UART_CMD_POS] = cmd;
    pkt[UART_PAYLOAD_OFFSET + 0] = p0;
    pkt[UART_PAYLOAD_OFFSET + 1] = p1;
    pkt[UART_PAYLOAD_OFFSET + 2] = p2;
    pkt[UART_PAYLOAD_OFFSET + 3] = p3;
    pkt[UART_PAYLOAD_OFFSET + 4] = p4;
    pkt[UART_CRC_OFFSET] = 0u;

    for (i = 0; i < (int)(UART_PACKET_SIZE - 1u); i++)
        pkt[UART_CRC_OFFSET] ^= pkt[i];

    decode(pkt, line, sizeof(line));
    log_m(line);
    Logger_Packet(LOG_RX, pkt, UART_PACKET_SIZE);

    for (i = 0; i < (int)UART_PACKET_SIZE; i++) {
        rx_byte = pkt[i];
        HAL_UART_RxCpltCallback(&huart1);
    }
}

/* ── Flush STM32 -> PC TX buffer ────────────────────────────────────── */
void UI_FlushTxPackets(void)
{
    char     line[UI_LINE_WIDTH];
    uint16_t i = 0u;

    if (!tx_buf_len) return;

    while (i < tx_buf_len) {
        if (tx_buf[i] == ACK) {
            log_l("  [ACK]");
            uint8_t a = ACK;
            Logger_Packet(LOG_ACK, &a, 1u);
            i++;
        }
        else if (tx_buf[i] == NACK) {
            log_l("  [NACK]");
            uint8_t a = NACK;
            Logger_Packet(LOG_NACK, &a, 1u);
            i++;
        }
        else if (tx_buf[i] == SIM_UART_PACKET_HEADER &&
            (tx_buf_len - i) >= UART_PACKET_SIZE) {
            decode(tx_buf + i, line, sizeof(line));
            log_r(line);

            /* Capture wall packets for display / visualisation. */
            if (tx_buf[i + UART_CMD_POS] == CMD_DELTA &&
                tx_buf[i + UART_PAYLOAD_OFFSET + 4] == 1u &&
                wall_count < UI_MAX_WALLS) {
                walls[wall_count].x1 = tx_buf[i + UART_PAYLOAD_OFFSET + 0];
                walls[wall_count].y1 = tx_buf[i + UART_PAYLOAD_OFFSET + 1];
                walls[wall_count].x2 = tx_buf[i + UART_PAYLOAD_OFFSET + 2];
                walls[wall_count].y2 = tx_buf[i + UART_PAYLOAD_OFFSET + 3];
                wall_count++;

                char wall_msg[64];
                snprintf(wall_msg, sizeof(wall_msg),
                    "  WALL: (%d,%d)-(%d,%d) #%d",
                    walls[wall_count - 1].x1, walls[wall_count - 1].y1,
                    walls[wall_count - 1].x2, walls[wall_count - 1].y2,
                    wall_count);
                log_l(wall_msg);
            }

            Logger_Packet(LOG_TX, tx_buf + i, UART_PACKET_SIZE);
            i += UART_PACKET_SIZE;
        }
        else {
            i++;
        }
    }

    tx_buf_len = 0u;
}

/* ── Print help ──────────────────────────────────────────────────────── */
static void print_help(void)
{
    log_l("┌─ Commands ───────────────────────────┐");
    log_l("│  GAME CONTROL:                        │");
    log_l("│    menu, play, pause, gameover, sleep │");
    log_l("│    up, down, left, right              │");
    log_l("│                                        │");
    log_l("│  LEVELS:                              │");
    log_l("│    level 0..5  or  lvl0..lvl5        │");
    log_l("│    0=empty 1=corridors 2=rings        │");
    log_l("│    3=crosses 4=maze 5=sym.corners     │");
    log_l("│                                        │");
    log_l("│  WALL VISUALISATION:                   │");
    log_l("│    walls      - list all walls        │");
    log_l("│    map        - show ASCII wall map   │");
    log_l("│    clearwalls - clear wall list       │");
    log_l("│                                        │");
    log_l("│  DEBUG:                                │");
    log_l("│    dbg 0..6                           │");
    log_l("│    btn short/medium/long              │");
    log_l("│    btn press / btn release            │");
    log_l("│                                        │");
    log_l("│  CONFIG:                               │");
    log_l("│    speed slow/medium/fast             │");
    log_l("│    wrap / nowrap                       │");
    log_l("│                                        │");
    log_l("│  TESTS:                                │");
    log_l("│    test walls - run wall tests        │");
    log_l("│    cls        - clear panels          │");
    log_l("│    help       - this help             │");
    log_l("│                                        │");
    log_l("│  RAW:                                  │");
    log_l("│    send AA BB CC DD EE FF GG HH       │");
    log_l("└────────────────────────────────────────┘");
}

/* ── Process command ─────────────────────────────────────────────────── */
static void process_command(const char* line)
{
    char  echo[UI_LINE_WIDTH];
    char  cmd_copy[UI_CMD_BUF_LEN];
    char* cmd;
    char* arg;

    /* Strip leading whitespace. */
    while (*line && isspace((unsigned char)*line)) line++;

    strncpy(cmd_copy, line, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    snprintf(echo, sizeof(echo), "► %s", line);
    log_l(echo);
    Logger_Write(LOG_CMD, line);

    cmd = cmd_copy;
    arg = strchr(cmd_copy, ' ');
    if (arg) {
        *arg = '\0';
        arg++;
        while (*arg && isspace((unsigned char)*arg)) arg++;
    }

    /* ── Game control ──────────────────────────────────────────────── */
    if (strcmp(cmd, "menu") == 0) UI_InjectPacket(CMD_SET_STATE, STATE_MENU, 0, 0, 0, 0);
    else if (strcmp(cmd, "play") == 0) UI_InjectPacket(CMD_SET_STATE, STATE_PLAYING, 0, 0, 0, 0);
    else if (strcmp(cmd, "pause") == 0) UI_InjectPacket(CMD_SET_STATE, STATE_PAUSE, 0, 0, 0, 0);
    else if (strcmp(cmd, "gameover") == 0) UI_InjectPacket(CMD_SET_STATE, STATE_GAMEOVER, 0, 0, 0, 0);
    else if (strcmp(cmd, "sleep") == 0) UI_InjectPacket(CMD_SET_STATE, STATE_SLEEP, 0, 0, 0, 0);

    /* ── Movement ──────────────────────────────────────────────────── */
    else if (strcmp(cmd, "up") == 0) UI_InjectPacket(CMD_MOVE, 0, 0, 0, 0, 0);
    else if (strcmp(cmd, "down") == 0) UI_InjectPacket(CMD_MOVE, 1, 0, 0, 0, 0);
    else if (strcmp(cmd, "left") == 0) UI_InjectPacket(CMD_MOVE, 2, 0, 0, 0, 0);
    else if (strcmp(cmd, "right") == 0) UI_InjectPacket(CMD_MOVE, 3, 0, 0, 0, 0);

    /* ── Boundary config ───────────────────────────────────────────── */
    else if (strcmp(cmd, "wrap") == 0) UI_InjectPacket(CMD_CONFIG, CONFIG_BOUNDARY, BOUNDARY_WRAP, 0, 0, 0);
    else if (strcmp(cmd, "nowrap") == 0) UI_InjectPacket(CMD_CONFIG, CONFIG_BOUNDARY, BOUNDARY_GAMEOVER, 0, 0, 0);

    /* ── Speed config ──────────────────────────────────────────────── */
    else if (strcmp(cmd, "speed") == 0 && arg) {
        if (strcmp(arg, "slow") == 0) UI_InjectPacket(CMD_CONFIG, CONFIG_SPEED, SPEED_SLOW, 0, 0, 0);
        else if (strcmp(arg, "medium") == 0) UI_InjectPacket(CMD_CONFIG, CONFIG_SPEED, SPEED_MEDIUM, 0, 0, 0);
        else if (strcmp(arg, "fast") == 0) UI_InjectPacket(CMD_CONFIG, CONFIG_SPEED, SPEED_FAST, 0, 0, 0);
        else log_l("  speed: slow/medium/fast");
    }

    /* ── Level selection ───────────────────────────────────────────── */
    else if (strcmp(cmd, "level") == 0 || strcmp(cmd, "lvl") == 0 ||
        strncmp(cmd, "level", 5) == 0 || strncmp(cmd, "lvl", 3) == 0) {

        int level = -1;

        if (arg) {
            level = atoi(arg);
        }
        else {
            if (strncmp(cmd, "level", 5) == 0 && strlen(cmd) > 5) {
                level = atoi(cmd + 5);
            }
            else if (strncmp(cmd, "lvl", 3) == 0 && strlen(cmd) > 3) {
                level = atoi(cmd + 3);
            }
        }

        if (level >= 0 && level <= GAME_LEVEL_MAX) {
            current_level = (uint8_t)level;
            /* CONFIG_WALLS payload is the level index directly. */
            UI_InjectPacket(CMD_CONFIG, CONFIG_WALLS, (uint8_t)level, 0, 0, 0);

            char buf[64];
            snprintf(buf, sizeof(buf), "  Level %d set", level);
            log_l(buf);
            show_level_layout(level);
        }
        else {
            char buf[64];
            snprintf(buf, sizeof(buf), "  Level must be 0-%d", GAME_LEVEL_MAX);
            log_l(buf);
        }
    }

    /* ── Wall visualisation ────────────────────────────────────────── */
    else if (strcmp(cmd, "walls") == 0) show_current_walls();
    else if (strcmp(cmd, "map") == 0) show_wall_map();
    else if (strcmp(cmd, "clearwalls") == 0) {
        wall_count = 0;
        log_l("  Wall list cleared");
    }

    /* ── Automated tests ───────────────────────────────────────────── */
    else if ((strcmp(cmd, "test") == 0 && arg && strcmp(arg, "walls") == 0) ||
        strcmp(cmd, "testwalls") == 0) {
        test_walls();
    }

    /* ── Debug mode ────────────────────────────────────────────────── */
    else if (strcmp(cmd, "dbg") == 0 && arg) {
        int m = atoi(arg);
        if (m >= 0 && m <= (int)DEBUG_STEP_MODE) {
            UI_InjectPacket(CMD_DEBUG, (uint8_t)m, 0, 0, 0, 0);
        }
        else {
            log_l("  dbg: 0..6 only");
        }
    }

    /* ── Button simulation ─────────────────────────────────────────── */
    else if (strcmp(cmd, "btn") == 0 && arg) {
        if (strcmp(arg, "press") == 0) {
            sim_button_state = GPIO_PIN_SET;
            button_pressed = 1u;
            button_press_end = UINT32_MAX;   /* hold indefinitely */
            log_l("  ◉ BTN HELD");
            Logger_Button("press (held)", 0u);

        }
        else if (strcmp(arg, "release") == 0) {
            sim_button_state = GPIO_PIN_RESET;
            button_pressed = 0u;
            button_press_end = 0u;
            log_l("  ○ BTN released");
            Logger_Button("release", 0u);

        }
        else if (strcmp(arg, "short") == 0) {
            /* SIM_BTN_SHORT_MS < BUTTON_SHORT_PRESS_MS → short press */
            sim_button_state = GPIO_PIN_SET;
            button_pressed = 1u;
            button_press_end = HAL_GetTick() + SIM_BTN_SHORT_MS;
            char buf[64];
            snprintf(buf, sizeof(buf), "  ◉ BTN short (%ums)...", SIM_BTN_SHORT_MS);
            log_l(buf);
            Logger_Button("short press started", SIM_BTN_SHORT_MS);

        }
        else if (strcmp(arg, "medium") == 0) {
            /* SIM_BTN_MEDIUM_MS in [SHORT, LONG) → long press (cycle debug) */
            sim_button_state = GPIO_PIN_SET;
            button_pressed = 1u;
            button_press_end = HAL_GetTick() + SIM_BTN_MEDIUM_MS;
            char buf[64];
            snprintf(buf, sizeof(buf), "  ◉ BTN medium (%ums)...", SIM_BTN_MEDIUM_MS);
            log_l(buf);
            Logger_Button("medium press started", SIM_BTN_MEDIUM_MS);

        }
        else if (strcmp(arg, "long") == 0) {
            /* SIM_BTN_LONG_MS >= BUTTON_LONG_PRESS_MS → very-long press (force MENU) */
            sim_button_state = GPIO_PIN_SET;
            button_pressed = 1u;
            button_press_end = HAL_GetTick() + SIM_BTN_LONG_MS;
            char buf[64];
            snprintf(buf, sizeof(buf), "  ◉ BTN long (%ums)...", SIM_BTN_LONG_MS);
            log_l(buf);
            Logger_Button("long press started", SIM_BTN_LONG_MS);
        }
    }

    /* ── Raw packet ────────────────────────────────────────────────── */
    else if (strcmp(cmd, "send") == 0 && arg) {
        uint8_t raw[UART_PACKET_SIZE] = { 0u };
        int     n = 0;
        char* token = strtok(arg, " ");
        while (token && n < (int)UART_PACKET_SIZE) {
            raw[n++] = (uint8_t)strtol(token, NULL, 16);
            token = strtok(NULL, " ");
        }
        snprintf(echo, sizeof(echo), "  RAW %d bytes →", n);
        log_m(echo);
        for (int i = 0; i < n; i++) {
            rx_byte = raw[i];
            HAL_UART_RxCpltCallback(&huart1);
        }
    }

    /* ── Help and clear ────────────────────────────────────────────── */
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        print_help();
    }
    else if (strcmp(cmd, "cls") == 0 || strcmp(cmd, "clear") == 0) {
        lc = mc = rc = 0;
        wall_count = 0;
        full_redraw();
        print_help();
    }
    else {
        snprintf(echo, sizeof(echo), "  ? '%s'  (type 'help')", line);
        log_l(echo);
    }
}

/* ── Render status bar ───────────────────────────────────────────────── */
void UI_RenderStatus(void)
{
    uint8_t       st = GameIO_GetState();
    uint8_t       dbg = Debug_GetMode();
    GPIO_PinState g = gpio_state[GPIO_IDX_LED_GREEN];
    GPIO_PinState b = gpio_state[GPIO_IDX_LED_BLUE];

    at(ROW_STATUS, 3);
    printf(BOLD " G:");
    printf(g == GPIO_PIN_SET ? GRN "●" RST BOLD : DIM "○" RST BOLD);
    printf("  ");
    printf(YEL "%-8s" RST BOLD "  ", sname(st));
    printf(" B:");
    printf(b == GPIO_PIN_SET ? BLU "●" RST BOLD : DIM "○" RST BOLD);
    printf("  ");
    printf("│  dbg: " MAG "%-9s" RST BOLD, dname(dbg));
    printf("│  level: " CYN "%d" RST BOLD, current_level);
    printf("│  btn: %s",
        sim_button_state == GPIO_PIN_SET ? RED "HELD" RST : DIM "idle" RST);
    printf(BOLD "          Snake Simulator v4.0" RST);
    eol();

    at(ROW_INPUT, 3);
    eol();
    printf(YEL "► " RST "%.*s", cmd_pos, cmd_buf);
    show_cursor();
    fflush(stdout);

    Logger_CheckStateChanges();
}

/* ── Keyboard input ──────────────────────────────────────────────────── */
void UI_ProcessInput(void)
{
    int ch;
#ifdef _WIN32
    if (!_kbhit()) return;
    ch = _getch();
#else
    ch = getchar();
    if (ch == EOF) return;
#endif

    if (ch == '\r' || ch == '\n') {
        cmd_buf[cmd_pos] = '\0';
        if (cmd_pos > 0) process_command(cmd_buf);
        cmd_pos = 0;

        at(ROW_INPUT, 3);
        eol();
        printf(YEL "► " RST);
        fflush(stdout);
    }
    else if (ch == '\b' || ch == 127) {   /* backspace or DEL */
        if (cmd_pos > 0) {
            cmd_pos--;
            at(ROW_INPUT, 3 + cmd_pos + 2);
            printf(" \b");
        }
    }
    else if (ch >= 32 && ch < 127 && cmd_pos < (int)(sizeof(cmd_buf) - 1u)) {
        cmd_buf[cmd_pos++] = (char)ch;
        printf("%c", ch);
        fflush(stdout);
    }
}

/* ── Button state auto-release ───────────────────────────────────────── */
void UI_UpdateButton(void)
{
    if (button_pressed && button_press_end != UINT32_MAX) {
        if (HAL_GetTick() >= button_press_end) {
            sim_button_state = GPIO_PIN_RESET;
            button_pressed = 0u;
            button_press_end = 0u;
            log_l("  ○ BTN auto-released");
            Logger_Button("auto-release", 0u);
        }
    }
}

/* ── Init ────────────────────────────────────────────────────────────── */
void UI_Init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);   /* UTF-8 output */
    SetConsoleCP(65001);         /* UTF-8 input  */
    setvbuf(stdout, NULL, _IONBF, 0);
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD  m = 0u;
        GetConsoleMode(h, &m);
        SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING
            | DISABLE_NEWLINE_AUTO_RETURN);
    }
#else
    struct termios t;
    tcgetattr(0, &t);
    t.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &t);
    fcntl(0, F_SETFL, O_NONBLOCK);
#endif

    Logger_Init();
    hide_cursor();
    full_redraw();
    print_help();
    show_level_layout(0);
}