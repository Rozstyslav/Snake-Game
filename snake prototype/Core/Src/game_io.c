#include "game_io.h"
#include "uart_protocol.h"
#include "debug.h"
#include <stddef.h>

extern uint8_t current_state;

static uint16_t config_speed = 200;
static uint8_t config_wall_difficulty = 0;   // тепер зберігатиме номер рівня 0..4
static uint8_t config_boundary_mode = 0;
static uint8_t selected_level = 0;            // нове

static GameIO_CommandCallback user_callback = NULL;

/* Нова функція – викликається з UI при виборі рівня */
void GameIO_SetSelectedLevel(uint8_t level)
{
    if (level <= 4) {
        selected_level = level;
    }
}

uint8_t GameIO_GetSelectedLevel(void)
{
    return selected_level;
}

static void UART_Handler(uint8_t cmd, uint8_t *payload)
{
    switch (cmd) {
        case CMD_CONFIG:
            switch (payload[0]) {
                case CONFIG_SPEED:
                    switch (payload[1]) {
                        case SPEED_SLOW:   config_speed = 400; break;
                        case SPEED_MEDIUM: config_speed = 200; break;
                        case SPEED_FAST:   config_speed = 100; break;
                        default: break;
                    }
                    break;

                case CONFIG_WALLS:
                    // Дозволяємо будь-яке значення 0..4 (рівень)
                    config_wall_difficulty = payload[1];
                    // Також оновлюємо selected_level, якщо команда прийшла з ПК
                    selected_level = payload[1];
                    break;

                case CONFIG_BOUNDARY:
                    if (payload[1] <= BOUNDARY_GAMEOVER) {
                        config_boundary_mode = payload[1];
                    }
                    break;

                default:
                    break;
            }
            break;

        case CMD_DEBUG:
            if (payload[0] <= 6) {
                Debug_SetMode(payload[0]);
            }
            break;

        default:
            break;
    }
    Debug_OnCommand(cmd, payload);

    if (user_callback != NULL) {
        user_callback(cmd, payload);
    }
}
void GameIO_Init(void)
{
    UART_RegisterHandler(UART_Handler);
}

uint8_t GameIO_GetDebugMode(void)
{
    return Debug_GetMode();
}

void GameIO_RegisterCallback(GameIO_CommandCallback cb)
{
    user_callback = cb;
}

/* --- STATE --- */

uint8_t GameIO_GetState(void)
{
    return current_state;
}

void GameIO_SetState(uint8_t state)
{
    if (state <= 4) {
        current_state = state;
    }
}

/* --- CONFIGURATION GETTERS --- */

uint16_t GameIO_GetSpeed(void)
{
    return config_speed;
}

uint8_t GameIO_GetWallDifficulty(void)
{
    return config_wall_difficulty;
}

uint8_t GameIO_GetBoundaryMode(void)
{
    return config_boundary_mode;
}

/* --- OUTPUT --- */

void GameIO_SendDelta(uint8_t head_x, uint8_t head_y,
                      uint8_t x2, uint8_t y2, uint8_t wall_flag)
{
    uint8_t payload[5] = {head_x, head_y, x2, y2, wall_flag};
    UART_SendPacket(CMD_DELTA, payload, 5);
}

void GameIO_SendScore(uint8_t len, uint16_t score)
{
    UART_SendScore(len, score);
}

void GameIO_SendState(uint8_t state)
{
    UART_SendState((GameState_t)state);
}
