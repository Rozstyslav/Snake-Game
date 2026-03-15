#ifndef LOGGER_H
#define LOGGER_H

/**
 * @file logger.h
 * @brief Snake Simulator session logger.
 *
 * Writes timestamped entries to a log file for:
 *   - RX packets  (PC -> STM32)
 *   - TX packets  (STM32 -> PC)
 *   - ACK / NACK responses
 *   - Button actions
 *   - LED state changes
 *   - Debug-mode and game-state transitions
 */

#include <stdint.h>

typedef enum {
    LOG_RX,       /**< Packet sent by the PC to STM32 */
    LOG_TX,       /**< Packet sent by STM32 to the PC */
    LOG_ACK,      /**< ACK response from STM32 */
    LOG_NACK,     /**< NACK response from STM32 */
    LOG_BTN,      /**< Button press / release event */
    LOG_LED,      /**< LED state change */
    LOG_STATE,    /**< Game-state transition */
    LOG_DBG,      /**< Debug-mode change */
    LOG_CMD,      /**< User command entered in the UI */
    LOG_INFO      /**< General informational message */
} LogCategory;

/** Open the log file (snake_sim_YYYYMMDD_HHMMSS.log). Call once at startup. */
void Logger_Init(void);

/** Flush and close the log file. Call before exiting. */
void Logger_Close(void);

/** Write a plain text event with category tag and timestamp. */
void Logger_Write(LogCategory cat, const char *msg);

/** Decode and log a raw packet (hex dump + human-readable description). */
void Logger_Packet(LogCategory cat, const uint8_t *pkt, uint16_t len);

/** Log the current LED state; skips the call if nothing has changed. */
void Logger_LedState(int green, int blue);

/** Log a button action with an optional duration in milliseconds. */
void Logger_Button(const char *action, uint32_t duration_ms);

/**
 * @brief Poll for game-state / debug-mode / LED changes and log them.
 * Call once per UI render cycle (e.g. from UI_RenderStatus()).
 */
void Logger_CheckStateChanges(void);

#endif /* LOGGER_H */
