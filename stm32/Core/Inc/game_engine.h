/**
 * @file game_engine.h
 * @brief Game engine interface.
 */

#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <stdint.h>

/**
 * @brief Get the game update speed in milliseconds.
 * @return Speed in milliseconds (e.g., 100, 200, 400)
 */
uint16_t Game_GetSpeedMs(void);

/**
 * @brief Periodic game update (called from main loop when playing).
 */
void Game_Update(void);

/**
 * @brief Process a command received from the PC.
 * @param cmd Command byte (CMD_MOVE, CMD_SET_STATE, etc)
 * @param payload Pointer to 5-byte payload
 */
void Game_HandleCommand(uint8_t cmd, uint8_t *payload);

/**
 * @brief Start a new game (transition from MENU to PLAYING).   // NEW
 */
void Game_Start(void);

#endif /* GAME_ENGINE_H */
