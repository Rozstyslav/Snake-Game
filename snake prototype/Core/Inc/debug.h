#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

typedef enum {
    DEBUG_OFF = 0,
    DEBUG_RANDOM_PACKETS,   // 0x01: random delta packets with possible errors
    DEBUG_SILENT_FLAG,      // 0x02: only flag, no actions (LED may indicate)
    DEBUG_SCENARIO_1,       // 0x03: pre‑defined scenario playback
    DEBUG_LOOPBACK,         // 0x04: echo received commands as delta packets
    DEBUG_ERROR_INJECTION,  // 0x05: inject CRC errors, delays, etc.
    DEBUG_STEP_MODE         // 0x06: step‑by‑step mode, one step per button press  // NEW
} DebugMode_t;

/**
 * @brief Initialize debug module.
 */
void Debug_Init(void);

/**
 * @brief Set current debug mode.
 * @param mode 0..6
 */
void Debug_SetMode(uint8_t mode);

/**
 * @brief Get current debug mode.
 */
uint8_t Debug_GetMode(void);

/**
 * @brief Process debug tasks. Call this periodically from main loop.
 * @param current_tick HAL tick in milliseconds.
 */
void Debug_Process(uint32_t current_tick);

/**
 * @brief Handle an incoming UART command (called from game_io when packet received).
 * @param cmd Command byte
 * @param payload 5‑byte payload
 */
void Debug_OnCommand(uint8_t cmd, uint8_t *payload);

#endif
