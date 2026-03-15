#ifndef LED_H
#define LED_H

#include <stdint.h>

typedef enum {
    LED_ID_GREEN = 0,
    LED_ID_RED = 1
} LedId_t;

typedef enum {
    LED_OFF,
    LED_ON,
    LED_SLOW_BLINK,   // 1 Hz (500 ms half‑period)
    LED_FAST_BLINK    // 4 Hz (125 ms half‑period)
} LedMode_t;

// NEW: Events for temporary indication (override normal mode)
typedef enum {
    LED_EVENT_ACK,           // green fast flash
    LED_EVENT_NACK,          // red fast flash
    LED_EVENT_CRC_ERROR,     // both fast flashes (double flash)
    LED_EVENT_DEBUG_MODE_CHANGE  // green shows mode number by flashes
} LED_Event_t;

/**
 * @brief Initialize LED module.
 */
void LED_Init(void);

/**
 * @brief Update LED physical states based on current modes and overrides.
 * Call this periodically (e.g., every main loop iteration).
 */
void LED_Update(void);

/**
 * @brief Set normal operation mode for an LED based on game state and debug.
 * Call whenever game state or debug mode changes.
 */
void LED_UpdateModes(void);

/**
 * @brief Set a temporary override for an LED (event indication).
 * @param led LED ID (GREEN or RED)
 * @param mode Blinking mode
 * @param duration_ms Duration in milliseconds
 */
void LED_SetTempEvent(LedId_t led, LedMode_t mode, uint32_t duration_ms);

// NEW: Trigger a predefined event
void LED_TriggerEvent(LED_Event_t event);

// NEW: Indicate debug mode by a sequence of flashes (green LED)
void LED_IndicateDebugMode(uint8_t mode);

#endif
