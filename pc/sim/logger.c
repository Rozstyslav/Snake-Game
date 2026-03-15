/**
 * @file logger.c
 * @brief Snake Simulator session logger implementation.
 *
 * Log file format (each line):
 *
 *   [HH:MM:SS.mmm] [CAT]  <description>                 | <hex bytes>
 *
 * Categories:
 *   RX  – packet sent by the PC to STM32
 *   TX  – packet sent by STM32 to the PC
 *   ACK – single-byte ACK from STM32
 *   NAK – single-byte NACK from STM32
 *   BTN – button press / release
 *   LED – LED state change
 *   STA – game-state transition
 *   DBG – debug-mode change
 *   CMD – user command
 *   INF – general information
 */

#include "logger.h"
#include "sim_config.h"
#include "hal_stub.h"
#include "uart_protocol.h"
#include "game_io.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ── Module state ────────────────────────────────────────────────────── */
static FILE    *log_file      = NULL;
static uint32_t session_start = 0u;   /* HAL tick at session open (ms) */

/* Previous values used to detect state / LED changes between poll calls. */
static int     prev_green = -1;
static int     prev_blue  = -1;
static uint8_t prev_state = SIM_PREV_UNINIT;
static uint8_t prev_dbg   = SIM_PREV_UNINIT;

/* ── Name helpers (local copies to avoid a dependency on ui.c) ───────── */
static const char *sname(uint8_t s)
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

static const char *dname(uint8_t d)
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

/* ── Timestamp formatting ────────────────────────────────────────────── */
static void fmt_timestamp(char *buf, int blen)
{
    uint32_t elapsed = HAL_GetTick() - session_start;
    uint32_t ms  =  elapsed % 1000u;
    uint32_t sec = (elapsed / 1000u)    % 60u;
    uint32_t min = (elapsed / 60000u)   % 60u;
    uint32_t hr  =  elapsed / 3600000u;
    snprintf(buf, blen, "%02u:%02u:%02u.%03u", hr, min, sec, ms);
}

static const char *cat_str(LogCategory c)
{
    switch (c) {
    case LOG_RX:    return "RX ";
    case LOG_TX:    return "TX ";
    case LOG_ACK:   return "ACK";
    case LOG_NACK:  return "NAK";
    case LOG_BTN:   return "BTN";
    case LOG_LED:   return "LED";
    case LOG_STATE: return "STA";
    case LOG_DBG:   return "DBG";
    case LOG_CMD:   return "CMD";
    case LOG_INFO:  return "INF";
    default:        return "???";
    }
}

/* ── Init ────────────────────────────────────────────────────────────── */
void Logger_Init(void)
{
    char filename[LOG_FILENAME_LEN];
    time_t now = time(NULL);
    struct tm *t;

#ifdef _WIN32
    t = localtime(&now);
#else
    struct tm tbuf;
    t = localtime_r(&now, &tbuf);
#endif

    snprintf(filename, sizeof(filename),
             "snake_sim_%04d%02d%02d_%02d%02d%02d.log",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    log_file = fopen(filename, "w");
    if (!log_file) {
        fprintf(stderr, "[logger] cannot open %s\n", filename);
        return;
    }

    session_start = HAL_GetTick();

    /* Write file header in separate calls to avoid format-truncation warnings. */
    fprintf(log_file, "Snake Simulator Session Log\n");
    fprintf(log_file, "Started: %04d-%02d-%02d %02d:%02d:%02d\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    fprintf(log_file, "File:    %s\n", filename);
    fprintf(log_file, "Columns: [HH:MM:SS.mmm] [CAT]  description               | hex\n");
    fprintf(log_file, "Categories: RX=PC->STM  TX=STM->PC  ACK  NAK  BTN  LED  STA  DBG  CMD\n");
    fprintf(log_file, "%.*s\n", LOG_SEPARATOR_LEN,
            "================================================================================");
    fflush(log_file);

    fprintf(stderr, "[logger] logging to %s\n", filename);
}

void Logger_Close(void)
{
    char ts[LOG_TIMESTAMP_LEN];
    if (!log_file) return;
    fmt_timestamp(ts, sizeof(ts));
    fprintf(log_file, "[%s] [INF]  === Session ended ===\n", ts);
    fclose(log_file);
    log_file = NULL;
}

/* ── Write raw string ────────────────────────────────────────────────── */
void Logger_Write(LogCategory cat, const char *msg)
{
    char ts[LOG_TIMESTAMP_LEN];
    if (!log_file) return;
    fmt_timestamp(ts, sizeof(ts));
    fprintf(log_file, "[%s] [%s]  %s\n", ts, cat_str(cat), msg);
    fflush(log_file);
}

/* ── Decode and write packet ─────────────────────────────────────────── */
void Logger_Packet(LogCategory cat, const uint8_t *p, uint16_t len)
{
    char    ts[LOG_TIMESTAMP_LEN];
    char    decoded[LOG_DECODED_LEN];
    char    hex[LOG_HEX_LEN];
    uint8_t cmd, crc;
    int     i;

    if (!log_file || !p || len < 1u) return;
    fmt_timestamp(ts, sizeof(ts));

    /* ACK and NACK are single-byte responses. */
    if (len == 1u) {
        if (p[0] == ACK) {
            fprintf(log_file, "[%s] [ACK]  <--\n", ts);
            fflush(log_file);
            return;
        }
        if (p[0] == NACK) {
            fprintf(log_file, "[%s] [NAK]  <--\n", ts);
            fflush(log_file);
            return;
        }
    }

    if (len < UART_PACKET_SIZE || p[0] != SIM_UART_PACKET_HEADER) {
        fprintf(log_file, "[%s] [%s]  ??? (len=%d)\n", ts, cat_str(cat), (int)len);
        fflush(log_file);
        return;
    }

    /* Build hex string for the full 8-byte packet. */
    snprintf(hex, sizeof(hex), "%02X %02X %02X %02X %02X %02X %02X %02X",
             p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);

    cmd = p[UART_CMD_POS];
    crc = 0u;
    for (i = 0; i < (int)(UART_PACKET_SIZE - 1u); i++) crc ^= p[i];

    if (crc != p[UART_CRC_OFFSET]) {
        snprintf(decoded, sizeof(decoded),
                 "CRC ERROR (got %02X exp %02X)", p[UART_CRC_OFFSET], crc);
    } else {
        switch (cmd) {
        case CMD_SET_STATE:
            snprintf(decoded, sizeof(decoded), "SET_STATE %s", sname(p[2]));
            break;
        case CMD_MOVE: {
            const char *dir[] = { "UP", "DOWN", "LEFT", "RIGHT" };
            snprintf(decoded, sizeof(decoded),
                     "MOVE %s", p[2] < 4u ? dir[p[2]] : "?");
            break;
        }
        case CMD_CONFIG:
            snprintf(decoded, sizeof(decoded),
                     "CONFIG [id=%02X val=%02X]", p[2], p[3]);
            break;
        case CMD_DEBUG:
            snprintf(decoded, sizeof(decoded),
                     "DEBUG mode=%d (%s)", p[2], dname(p[2]));
            break;
        case CMD_STATE:
            snprintf(decoded, sizeof(decoded), "STATE %s", sname(p[2]));
            break;
        case CMD_DELTA:
            snprintf(decoded, sizeof(decoded),
                     "DELTA head=(%d,%d) second=(%d,%d)%s",
                     p[2], p[3], p[4], p[5], p[6] ? " WALL" : "");
            break;
        case CMD_SCORE:
            snprintf(decoded, sizeof(decoded),
                     "SCORE len=%d val=%d",
                     p[2], (int)(p[3] | (p[4] << 8u)));
            break;
        default:
            snprintf(decoded, sizeof(decoded), "CMD=%02X", cmd);
            break;
        }
    }

    fprintf(log_file, "[%s] [%s]  %-32s| %s\n",
            ts, cat_str(cat), decoded, hex);
    fflush(log_file);
}

/* ── LED state ───────────────────────────────────────────────────────── */
void Logger_LedState(int green, int blue)
{
    char    ts[LOG_TIMESTAMP_LEN];
    char    line[LOG_LINE_LEN];
    uint8_t st, dbg;

    if (!log_file) return;
    if (green == prev_green && blue == prev_blue) return;   /* no change */

    prev_green = green;
    prev_blue  = blue;

    st  = GameIO_GetState();
    dbg = Debug_GetMode();

    fmt_timestamp(ts, sizeof(ts));
    snprintf(line, sizeof(line), "G:%-3s B:%-3s  | state=%-8s dbg=%s",
             green ? "ON" : "OFF",
             blue  ? "ON" : "OFF",
             sname(st), dname(dbg));

    fprintf(log_file, "[%s] [LED]  %s\n", ts, line);
    fflush(log_file);
}

/* ── Button ──────────────────────────────────────────────────────────── */
void Logger_Button(const char *action, uint32_t duration_ms)
{
    char ts[LOG_TIMESTAMP_LEN];
    char line[LOG_LINE_LEN];

    if (!log_file) return;
    fmt_timestamp(ts, sizeof(ts));

    if (duration_ms > 0u)
        snprintf(line, sizeof(line), "%-20s | duration=%ums",
                 action, (unsigned)duration_ms);
    else
        snprintf(line, sizeof(line), "%s", action);

    fprintf(log_file, "[%s] [BTN]  %s\n", ts, line);
    fflush(log_file);
}

/* ── Auto-detect state / debug / LED changes ─────────────────────────── */

/**
 * @brief Poll for game-state, debug-mode, and LED changes; log any transitions.
 * Intended to be called once per UI render cycle from UI_RenderStatus().
 */
void Logger_CheckStateChanges(void)
{
    char    ts[LOG_TIMESTAMP_LEN];
    uint8_t st  = GameIO_GetState();
    uint8_t dbg = Debug_GetMode();

    if (!log_file) return;
    fmt_timestamp(ts, sizeof(ts));

    if (st != prev_state) {
        fprintf(log_file, "[%s] [STA]  %s -> %s\n", ts,
                (prev_state == SIM_PREV_UNINIT) ? "---" : sname(prev_state),
                sname(st));
        fflush(log_file);
        prev_state = st;
    }

    if (dbg != prev_dbg) {
        fprintf(log_file, "[%s] [DBG]  %s -> %s (%d)\n", ts,
                (prev_dbg == SIM_PREV_UNINIT) ? "---" : dname(prev_dbg),
                dname(dbg), (int)dbg);
        fflush(log_file);
        prev_dbg = dbg;
    }

    /* LED changes are handled inside Logger_LedState (skips if unchanged). */
    Logger_LedState(gpio_state[GPIO_IDX_LED_GREEN] == GPIO_PIN_SET,
                    gpio_state[GPIO_IDX_LED_BLUE]  == GPIO_PIN_SET);
}
