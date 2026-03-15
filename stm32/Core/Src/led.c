/**
 * @file led.c
 * @brief LED driver with explicit 3-layer priority model.
 *
 * Hardware: STM32F0Discovery
 *   LD3 -> PC9 (green)
 *   LD4 -> PC8 (blue, referred to as "red" in the API to match LedId_t)
 *
 * Priority (highest wins, evaluated every LED_Update() call):
 *
 *   Layer 1 – seq    Flash sequence for debug-mode indication.
 *                    Drives green only; blue held LOW during the sequence.
 *                    Expires automatically after (count x period) ms.
 *
 *   Layer 2 – temp   Per-LED timed override (ACK / NACK / CRC events).
 *                    Independent per LED; expires after duration_ms.
 *
 *   Layer 3 – normal Steady pattern from pattern_table[state][debug].
 *                    Updated by LED_UpdateModes() whenever state or debug
 *                    mode changes.
 *
 * Layers 2 and 3 are evaluated independently for each LED so that, for
 * example, a NACK flash on the blue LED does not disturb the green LED's
 * normal blink pattern.
 */

#include "led.h"
#include "main.h"
#include "game_io.h"
#include "debug.h"
#include "game_config.h"
#include "uart_protocol.h"

/* ── Pin aliases ─────────────────────────────────────────────────────────── */
#define LED_GREEN_PIN  LD3_Pin   /* PC9 – physically green */
#define LED_BLUE_PIN   LD4_Pin   /* PC8 – physically blue  */

/* ── Normal-mode lookup table [state 0..4][debug 0..6] ───────────────────── */
typedef struct { LedMode_t green; LedMode_t blue; } LedPattern_t;

/*
 * Columns = debug modes : OFF, RANDOM, SILENT, SCENARIO, LOOPBACK, ERR_INJ, STEP
 * Rows    = game states : MENU, PLAYING, PAUSE, GAMEOVER, SLEEP
 */
static const LedPattern_t pattern_table[5][7] = {
    /* STATE_MENU (0) */
    {
        { LED_OFF,        LED_OFF        },  /* DEBUG_OFF      */
        { LED_OFF,        LED_SLOW_BLINK },  /* DEBUG_RANDOM   */
        { LED_OFF,        LED_FAST_BLINK },  /* DEBUG_SILENT   */
        { LED_OFF,        LED_ON         },  /* DEBUG_SCENARIO */
        { LED_SLOW_BLINK, LED_OFF        },  /* DEBUG_LOOPBACK */
        { LED_SLOW_BLINK, LED_SLOW_BLINK },  /* DEBUG_ERR_INJ  */
        { LED_SLOW_BLINK, LED_FAST_BLINK },  /* DEBUG_STEP     */
    },
    /* STATE_PLAYING (1) */
    {
        { LED_ON,         LED_OFF        },
        { LED_ON,         LED_SLOW_BLINK },
        { LED_ON,         LED_FAST_BLINK },
        { LED_ON,         LED_ON         },
        { LED_FAST_BLINK, LED_OFF        },
        { LED_FAST_BLINK, LED_SLOW_BLINK },
        { LED_FAST_BLINK, LED_FAST_BLINK },
    },
    /* STATE_PAUSE (2) */
    {
        { LED_SLOW_BLINK, LED_OFF        },
        { LED_SLOW_BLINK, LED_SLOW_BLINK },
        { LED_SLOW_BLINK, LED_FAST_BLINK },
        { LED_SLOW_BLINK, LED_ON         },
        { LED_ON,         LED_OFF        },
        { LED_ON,         LED_SLOW_BLINK },
        { LED_ON,         LED_FAST_BLINK },
    },
    /* STATE_GAMEOVER (3) */
    {
        { LED_FAST_BLINK, LED_OFF        },
        { LED_FAST_BLINK, LED_SLOW_BLINK },
        { LED_FAST_BLINK, LED_FAST_BLINK },
        { LED_FAST_BLINK, LED_ON         },
        { LED_OFF,        LED_OFF        },
        { LED_OFF,        LED_SLOW_BLINK },
        { LED_OFF,        LED_FAST_BLINK },
    },
    /* STATE_SLEEP (4) */
    {
        { LED_OFF, LED_OFF }, { LED_OFF, LED_OFF }, { LED_OFF, LED_OFF },
        { LED_OFF, LED_OFF }, { LED_OFF, LED_OFF }, { LED_OFF, LED_OFF },
        { LED_OFF, LED_OFF },
    },
};

/* ── Layer 3: normal mode (set by LED_UpdateModes) ───────────────────────── */
static LedMode_t normal[2] = { LED_OFF, LED_OFF };

/* ── Layer 2: per-LED timed override ─────────────────────────────────────── */
typedef struct {
    LedMode_t mode;
    uint32_t  start_ms;
    uint32_t  duration_ms;   /* 0 = inactive */
} TempLayer_t;

static TempLayer_t temp[2];  /* indexed by LedId_t */

/* ── Layer 1: debug-mode indication flash sequence ───────────────────────── */
/* Timing constants are defined in game_config.h:
 *   LED_SEQ_FLASH_MS  – ON  time per individual flash
 *   LED_SEQ_PAUSE_MS  – OFF gap between consecutive flashes            */

static struct {
    uint8_t  active;
    uint8_t  count;      /* number of flashes = mode + 1 */
    uint32_t start_ms;
} seq;

/* ── Low-level helpers ───────────────────────────────────────────────────── */

/** Resolve a LedMode_t to a GPIO pin state at time 'now'. */
static GPIO_PinState resolve(LedMode_t mode, uint32_t now)
{
    switch (mode) {
    case LED_ON:         return GPIO_PIN_SET;
    case LED_OFF:        return GPIO_PIN_RESET;
    case LED_SLOW_BLINK: return ((now / LED_SLOW_BLINK_HALF) & 1u) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    case LED_FAST_BLINK: return ((now / LED_FAST_BLINK_HALF) & 1u) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    default:             return GPIO_PIN_RESET;
    }
}

/** Write physical GPIO pins for both LEDs. */
static void write_pins(LedMode_t green, LedMode_t blue, uint32_t now)
{
    HAL_GPIO_WritePin(GPIOC, LED_GREEN_PIN, resolve(green, now));
    HAL_GPIO_WritePin(GPIOC, LED_BLUE_PIN,  resolve(blue,  now));
}

/** Return the effective mode for one LED (layer 2 vs layer 3). */
static LedMode_t effective_mode(LedId_t id, uint32_t now)
{
    TempLayer_t *t = &temp[id];

    if (t->duration_ms > 0u && (now - t->start_ms) < t->duration_ms) {
        return t->mode;
    }

    t->duration_ms = 0u;   /* mark expired for a fast-path on next call */
    return normal[id];
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void LED_Init(void)
{
    temp[LED_ID_GREEN] = (TempLayer_t){ LED_OFF, 0u, 0u };
    temp[LED_ID_RED]   = (TempLayer_t){ LED_OFF, 0u, 0u };
    seq.active = 0u;
    LED_UpdateModes();
}

void LED_UpdateModes(void)
{
    uint8_t state = GameIO_GetState();
    uint8_t dbg   = Debug_GetMode();

    if (state > 4u) state = 0u;
    if (dbg   > 6u) dbg   = 0u;

    normal[LED_ID_GREEN] = pattern_table[state][dbg].green;
    normal[LED_ID_RED]   = pattern_table[state][dbg].blue;
}

void LED_SetTempEvent(LedId_t led, LedMode_t mode, uint32_t duration_ms)
{
    if ((uint8_t)led > 1u) return;

    temp[led].mode        = mode;
    temp[led].start_ms    = HAL_GetTick();
    temp[led].duration_ms = duration_ms;
}

void LED_TriggerEvent(LED_Event_t event)
{
    switch (event) {
    case LED_EVENT_ACK:
        LED_SetTempEvent(LED_ID_GREEN, LED_FAST_BLINK, LED_EVENT_ACK_MS);
        break;

    case LED_EVENT_NACK:
        LED_SetTempEvent(LED_ID_RED, LED_FAST_BLINK, LED_EVENT_NACK_MS);
        break;

    case LED_EVENT_CRC_ERROR:
        LED_SetTempEvent(LED_ID_GREEN, LED_FAST_BLINK, LED_EVENT_CRC_MS);
        LED_SetTempEvent(LED_ID_RED,   LED_FAST_BLINK, LED_EVENT_CRC_MS);
        break;

    case LED_EVENT_DEBUG_MODE_CHANGE:
        /* Handled explicitly via LED_IndicateDebugMode(). */
        break;
    }
}

void LED_IndicateDebugMode(uint8_t mode)
{
    /* Cancel any active temp overrides so the sequence is clearly visible.
     * The seq layer takes priority over temp anyway, but clearing temp
     * ensures a clean transition back to normal when the sequence ends. */
    temp[LED_ID_GREEN].duration_ms = 0u;
    temp[LED_ID_RED].duration_ms   = 0u;

    seq.count    = mode + 1u;   /* mode 0 -> 1 flash, mode 6 -> 7 flashes */
    seq.start_ms = HAL_GetTick();
    seq.active   = 1u;
}

void LED_Update(void)
{
    uint32_t now = HAL_GetTick();

    /* ── Layer 1: debug-mode flash sequence ──────────────────────────────── */
    if (seq.active) {
        uint32_t period    = LED_SEQ_FLASH_MS + LED_SEQ_PAUSE_MS;
        uint32_t total_dur = (uint32_t)seq.count * period;
        uint32_t elapsed   = now - seq.start_ms;

        if (elapsed < total_dur) {
            uint32_t within = elapsed % period;
            /* Green flashes seq.count times; blue stays OFF throughout
             * so the two channels remain visually distinct. */
            GPIO_PinState g = (within < LED_SEQ_FLASH_MS) ? GPIO_PIN_SET : GPIO_PIN_RESET;
            HAL_GPIO_WritePin(GPIOC, LED_GREEN_PIN, g);
            HAL_GPIO_WritePin(GPIOC, LED_BLUE_PIN,  GPIO_PIN_RESET);
            return;
        }

        /* Sequence finished – restore normal mode and fall through. */
        seq.active = 0u;
        LED_UpdateModes();
    }

    /* ── Layers 2 + 3: resolve each LED independently ───────────────────── */
    write_pins(effective_mode(LED_ID_GREEN, now),
               effective_mode(LED_ID_RED,   now),
               now);
}
