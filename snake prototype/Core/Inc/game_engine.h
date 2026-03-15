#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <stdint.h>

/**
 * @brief Get current game speed in milliseconds.
 * @return Speed value from configuration (100, 200, or 400 ms)
 */
uint16_t Game_GetSpeedMs(void);

/**
 * @brief Update game state. Must be called periodically.
 *
 * This function is called from the main loop when the game is in PLAYING state.
 * It moves the snake, checks collisions, and sends delta updates.
 */
void Game_Update(void);

/**
 * @brief Handle incoming commands from PC.
 * @param cmd Command byte (CMD_SET_STATE, CMD_MOVE, etc)
 * @param payload 5-byte payload from UART packet
 *
 * This function should be registered via GameIO_RegisterCallback() in main.c
 * during system initialization.
 */
void Game_HandleCommand(uint8_t cmd, uint8_t *payload);

#endif /* GAME_ENGINE_H */
