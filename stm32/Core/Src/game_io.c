#include "game_io.h"
#include "uart_protocol.h"
#include "debug.h"
#include "game_config.h"
#include <stddef.h>

/* File I/O for simulator; Flash HAL for real hardware. */
#include <stdio.h>      /* FILE – simulator only */
#include <string.h>     /* memset */
#include "main.h"       /* HAL_FLASH_* – real device */

extern uint8_t current_state;

static uint16_t config_speed = SPEED_MS_MEDIUM; /**< Default speed: medium */
static uint8_t  config_wall_difficulty = 0;               /**< Stores the current level index */
static uint8_t  config_boundary_mode = 0;
static uint8_t  selected_level = 0;

static GameIO_CommandCallback user_callback = NULL;

/* High score cached in RAM; written to persistent storage on update. */
static uint16_t high_score = 0;

/*---------------------------------------------------------------------------*
 * Persistent storage (abstraction layer)                                    *
 *---------------------------------------------------------------------------*/

 /**
  * @brief Read the high score from persistent memory.
  * @return Stored high score, or 0 if no record exists.
  */
static uint16_t storage_read_highscore(void)
{
#ifdef SIMULATOR
    FILE* f = fopen("highscore.bin", "rb");
    if (!f) return 0u;
    uint16_t val;
    if (fread(&val, sizeof(val), 1, f) != 1) val = 0u;
    fclose(f);
    return val;
#else
    /* Real STM32: read directly from Flash as a half-word.
     * 0xFFFF is the erased state, interpreted as "no record". */
    uint16_t val = *(volatile uint16_t*)HIGHSCORE_FLASH_ADDR;
    return (val == 0xFFFFu) ? 0u : val;
#endif
}

/**
 * @brief Write the high score to persistent memory.
 * @param score Value to store.
 */
static void storage_write_highscore(uint16_t score)
{
#ifdef SIMULATOR
    FILE* f = fopen("highscore.bin", "wb");
    if (f) {
        fwrite(&score, sizeof(score), 1, f);
        fclose(f);
    }
#else
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError = 0u;

    HAL_FLASH_Unlock();

    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = HIGHSCORE_FLASH_ADDR;
    eraseInit.NbPages = 1u;

    status = HAL_FLASHEx_Erase(&eraseInit, &pageError);
    (void)status; /* Error is non-critical; ignore and attempt program anyway. */

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
        HIGHSCORE_FLASH_ADDR, (uint64_t)score);
    (void)status; /* Non-critical: high score will be retried next game over. */

    HAL_FLASH_Lock();
#endif
}

/*---------------------------------------------------------------------------*
 * Level selection                                                           *
 *---------------------------------------------------------------------------*/

 /** Called from UI when the player selects a level. */
void GameIO_SetSelectedLevel(uint8_t level)
{
    if (level <= GAME_LEVEL_MAX) {
        selected_level = level;
    }
}

uint8_t GameIO_GetSelectedLevel(void)
{
    return selected_level;
}

/*---------------------------------------------------------------------------*
 * Internal UART command handler                                             *
 *---------------------------------------------------------------------------*/

static void UART_Handler(uint8_t cmd, uint8_t* payload)
{
    switch (cmd) {
    case CMD_CONFIG:
        switch (payload[0]) {
        case CONFIG_SPEED:
            switch (payload[1]) {
            case SPEED_SLOW:   config_speed = SPEED_MS_SLOW;   break;
            case SPEED_MEDIUM: config_speed = SPEED_MS_MEDIUM; break;
            case SPEED_FAST:   config_speed = SPEED_MS_FAST;   break;
            default: break;
            }
            break;

        case CONFIG_WALLS:
            /* Accept any value 0..GAME_LEVEL_MAX (level index from PC). */
            config_wall_difficulty = payload[1];
            /* Mirror to selected_level so the game engine picks it up. */
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
        if (payload[0] <= DEBUG_STEP_MODE) {
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

/*---------------------------------------------------------------------------*
 * Initialization                                                            *
 *---------------------------------------------------------------------------*/

void GameIO_Init(void)
{
    UART_RegisterHandler(UART_Handler);
    /* Load high score from persistent storage on startup. */
    high_score = storage_read_highscore();
}

/*---------------------------------------------------------------------------*
 * Accessors                                                                 *
 *---------------------------------------------------------------------------*/

uint8_t GameIO_GetDebugMode(void)
{
    return Debug_GetMode();
}

void GameIO_RegisterCallback(GameIO_CommandCallback cb)
{
    user_callback = cb;
}

/* --- State --- */

uint8_t GameIO_GetState(void)
{
    return current_state;
}

void GameIO_SetState(uint8_t state)
{
    if (state <= STATE_SLEEP) {
        current_state = state;
        /* When entering MENU, send the high score with snake_len = 0
         * so the PC display can show it on the menu screen. */
        if (state == STATE_MENU) {
            UART_SendScore(0u, high_score);
        }
    }
}

/* --- Configuration getters --- */

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

/*---------------------------------------------------------------------------*
 * Output                                                                    *
 *---------------------------------------------------------------------------*/

void GameIO_SendDelta(uint8_t head_x, uint8_t head_y,
    uint8_t x2, uint8_t y2, uint8_t wall_flag)
{
    uint8_t payload[5] = { head_x, head_y, x2, y2, wall_flag };
    UART_SendPacket(CMD_DELTA, payload, 5u);
}

void GameIO_SendScore(uint8_t len, uint16_t score)
{
    UART_SendScore(len, score);
}

void GameIO_SendState(uint8_t state)
{
    UART_SendState((GameState_t)state);
}

/*---------------------------------------------------------------------------*
 * High score                                                                *
 *---------------------------------------------------------------------------*/

void GameIO_SubmitScore(uint16_t score)
{
    if (score > high_score) {
        high_score = score;
        storage_write_highscore(high_score);
    }
}