/**
 * @file sim_main.c
 * @brief Entry point for the Snake simulator on Windows / Linux.
 *
 * Replaces Core/Src/main.c from CubeIDE; the game-engine sources are
 * compiled without any modification.
 */

#include "hal_stub.h"
#include "main.h"
#include "sim_config.h"
#include "game_config.h"
#include "uart_protocol.h"
#include "game_io.h"
#include "game_engine.h"
#include "button.h"
#include "led.h"
#include "debug.h"
#include "ui.h"
#include "logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

 /* ── Variables that mirror those in the original main.c ─────────────── */
UART_HandleTypeDef huart1;

/* Defined in uart_protocol.c; read here after each received byte. */
extern uint8_t uart_packet_received;
extern uint8_t uart_last_crc_error;
uint8_t rx_byte = 0u;

uint8_t  current_state = STATE_MENU;   /* initial game state */
uint32_t last_command_tick = 0u;
uint8_t  wakeup_from_sleep = 0u;
uint8_t  step_requested = 0u;

/* ── Signal handler: flush the logger on Ctrl-C / SIGTERM ───────────── */
static void signal_handler(int sig)
{
    (void)sig;   /* suppress unused-parameter warning */
    Logger_Close();
    printf("\nSimulator terminated. Log file saved.\n");
    exit(0);
}

/* ── UART RX callback (mirrors the ISR in the original main.c) ──────── */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    (void)huart;

    /* Wakeup-from-sleep is detected but handled in the main loop. */
    last_command_tick = HAL_GetTick();
    UART_ProcessByte(rx_byte);

    if (uart_packet_received) {
        LED_TriggerEvent(LED_EVENT_ACK);
        uart_packet_received = 0u;
    }
    if (uart_last_crc_error) {
        LED_TriggerEvent(LED_EVENT_CRC_ERROR);
        uart_last_crc_error = 0u;
    }
    /* HAL_UART_Receive_IT is not needed: the next byte arrives via
     * UI_InjectPacket() calling this callback directly. */
}

void Error_Handler(void)
{
    printf("\n[ERROR_HANDLER called]\n");
    exit(1);
}

/* ── Precise loop delay ──────────────────────────────────────────────
 * Keeps each main-loop iteration close to target_ms without calling
 * Sleep(1) (which is too coarse on Windows for 1 ms targets).          */
static void precise_delay(uint32_t target_ms)
{
    static uint32_t last_tick = 0u;
    uint32_t now = HAL_GetTick();

    if ((now - last_tick) < target_ms) {
        uint32_t wait_ms = target_ms - (now - last_tick);
#ifdef _WIN32
        /* Windows Sleep() resolution is ~15 ms; use a spin-loop instead. */
        uint32_t start = HAL_GetTick();
        while ((HAL_GetTick() - start) < wait_ms) {
            /* active spin */
        }
#else
        /* POSIX nanosleep() is sufficiently accurate. */
        struct timespec ts = { (time_t)(wait_ms / 1000u),
                               (long)((wait_ms % 1000u) * 1000000L) };
        nanosleep(&ts, NULL);
#endif
    }

    last_tick = HAL_GetTick();
}

/* ── main ────────────────────────────────────────────────────────────── */
int main(void)
{
    /* Register signal handlers to ensure the log file is closed cleanly. */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#ifdef _WIN32
    signal(SIGBREAK, signal_handler);
#endif

    /* Initialisation – mirrors the sequence in the original main.c. */
    UART_Protocol_Init(&huart1);
    GameIO_Init();
    GameIO_RegisterCallback(Game_HandleCommand);
    Button_Init();
    LED_Init();
    Debug_Init();

    srand(HAL_GetTick());
    LED_UpdateModes();
    last_command_tick = HAL_GetTick();

    UI_Init();

    /* ── Main loop ──────────────────────────────────────────────────── */
    static uint32_t last_ui_render = 0u;
    static uint32_t last_game_update = 0u;
    static uint8_t  last_state_cache = SIM_PREV_UNINIT;
    static uint8_t  last_debug_cache = SIM_PREV_UNINIT;

    while (1) {
        uint32_t now = HAL_GetTick();

        /* 1. Button FSM (identical logic to the original main.c). */
        Button_Process();

        /* 2. LED driver update. */
        LED_Update();

        /* 3. Debug-mode tasks (random packets, scenario playback, etc.). */
        Debug_Process(now);

        /* 4. Wake from sleep. */
        if (wakeup_from_sleep) {
            wakeup_from_sleep = 0u;
            if (GameIO_GetState() == STATE_SLEEP) {
                GameIO_SetState(STATE_MENU);
                GameIO_SendState(STATE_MENU);
                LED_UpdateModes();
            }
            last_command_tick = now;
        }

        /* 5. Game engine update. */
        if (GameIO_GetState() == STATE_PLAYING) {
            uint16_t speed_ms = Game_GetSpeedMs();
            if (Debug_GetMode() == DEBUG_STEP_MODE) {
                if (step_requested) {
                    step_requested = 0u;
                    Game_Update();
                }
            }
            else {
                if ((now - last_game_update) >= speed_ms) {
                    last_game_update = now;
                    Game_Update();
                }
            }
        }

        /* 6. Resync LED pattern after any state or debug-mode change. */
        {
            uint8_t cur_state = GameIO_GetState();
            uint8_t cur_debug = Debug_GetMode();
            if (cur_state != last_state_cache || cur_debug != last_debug_cache) {
                last_state_cache = cur_state;
                last_debug_cache = cur_debug;
                LED_UpdateModes();
            }
        }

        /* 7. UI: flush new TX packets to the right panel and read keyboard. */
        UI_FlushTxPackets();
        UI_ProcessInput();
        UI_UpdateButton();   /* non-blocking simulated button timeout check */

        /* 8. Refresh the status bar at SIM_UI_RENDER_INTERVAL_MS cadence. */
        if ((now - last_ui_render) >= SIM_UI_RENDER_INTERVAL_MS) {
            last_ui_render = now;
            UI_RenderStatus();
        }

        /* 9. Throttle the loop to approximately SIM_LOOP_TICK_MS per iteration. */
        precise_delay(SIM_LOOP_TICK_MS);
    }

    return 0;
}