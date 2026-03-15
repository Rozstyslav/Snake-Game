/**
 * @file button.c
 * @brief Button driver with 3-state debounce FSM.
 *
 * Hardware: STM32F0Discovery, B1 on PA0, active HIGH (pull-down).
 *
 * State machine:
 *   IDLE -> DEBOUNCING -> HELD -> IDLE
 *
 * Press classification (measured from the first confirmed edge):
 *   < BUTTON_SHORT_PRESS_MS   -> short press  (game-state action)
 *   < BUTTON_LONG_PRESS_MS    -> long press   (cycle debug mode)
 *   >= BUTTON_LONG_PRESS_MS   -> very long    (force MENU + LED flash)
 */

#include "button.h"
#include "game_io.h"
#include "led.h"
#include "main.h"
#include "game_config.h"
#include "debug.h"
#include "game_engine.h"
#include "uart_protocol.h"

/* ── External flags set by this module, consumed elsewhere ───────────────── */
extern uint8_t step_requested;
extern uint8_t wakeup_from_sleep;

/* ── FSM ─────────────────────────────────────────────────────────────────── */
typedef enum {
    BTN_IDLE,
    BTN_DEBOUNCING,
    BTN_HELD
} BtnFsmState_t;

static BtnFsmState_t fsm_state = BTN_IDLE;
static uint32_t      press_tick = 0u;   /* HAL tick recorded on the rising edge */

/* ── Short-press action table ────────────────────────────────────────────── */
static void handle_short_press(void)
{
    uint8_t state = GameIO_GetState();
    uint8_t dbg   = Debug_GetMode();

    switch (state) {
    case STATE_MENU:
        Game_Start();
        break;

    case STATE_PLAYING:
        if (dbg == DEBUG_STEP_MODE) {
            step_requested = 1u;
        } else {
            GameIO_SetState(STATE_PAUSE);
            GameIO_SendState(STATE_PAUSE);
        }
        break;

    case STATE_PAUSE:
        GameIO_SetState(STATE_PLAYING);
        GameIO_SendState(STATE_PLAYING);
        break;

    case STATE_GAMEOVER:
        GameIO_SetState(STATE_MENU);
        GameIO_SendState(STATE_MENU);
        break;

    case STATE_SLEEP:
        wakeup_from_sleep = 1u;
        break;

    default:
        break;
    }
}

/* ── On-release dispatcher ───────────────────────────────────────────────── */
static void handle_release(uint32_t duration_ms)
{
    if (duration_ms >= BUTTON_LONG_PRESS_MS) {
        /* Very long press: force MENU and signal with a brief dual-LED blink. */
        GameIO_SetState(STATE_MENU);
        GameIO_SendState(STATE_MENU);
        LED_SetTempEvent(LED_ID_GREEN, LED_FAST_BLINK, BUTTON_VLONG_LED_MS);
        LED_SetTempEvent(LED_ID_RED,   LED_FAST_BLINK, BUTTON_VLONG_LED_MS);

    } else if (duration_ms >= BUTTON_SHORT_PRESS_MS) {
        /* Long press: advance to the next debug mode and show a flash sequence. */
        uint8_t next = (uint8_t)((Debug_GetMode() + 1u) % DEBUG_MODE_COUNT);
        Debug_SetMode(next);
        LED_IndicateDebugMode(next);

    } else {
        /* Short press: perform the context-sensitive game-state action. */
        handle_short_press();
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void Button_Init(void)
{
    fsm_state  = BTN_IDLE;
    press_tick = 0u;
}

void Button_Process(void)
{
    uint32_t      now = HAL_GetTick();
    GPIO_PinState pin = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

    switch (fsm_state) {

    /* -------------------------------------------------------------------- */
    case BTN_IDLE:
        if (pin == GPIO_PIN_SET) {
            press_tick = now;       /* record the candidate rising edge */
            fsm_state  = BTN_DEBOUNCING;
        }
        break;

    /* -------------------------------------------------------------------- */
    case BTN_DEBOUNCING:
        if (pin == GPIO_PIN_RESET) {
            /* Pin went LOW before the debounce window closed -> glitch.
             * Return to IDLE without triggering any action. */
            fsm_state = BTN_IDLE;

        } else if ((now - press_tick) >= BUTTON_DEBOUNCE_MS) {
            /* Stable HIGH for the full debounce window -> confirmed press. */
            fsm_state = BTN_HELD;
        }
        break;

    /* -------------------------------------------------------------------- */
    case BTN_HELD:
        if (pin == GPIO_PIN_RESET) {
            uint32_t duration = now - press_tick;
            fsm_state = BTN_IDLE;
            handle_release(duration);
        }
        break;

    /* -------------------------------------------------------------------- */
    default:
        fsm_state = BTN_IDLE;
        break;
    }
}
