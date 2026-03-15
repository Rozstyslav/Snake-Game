/**
 * @file game_io.h
 * @brief Game I/O driver - interface between UART protocol and game logic.
 */

#ifndef GAME_IO_H
#define GAME_IO_H

#include <stdint.h>

/*---------------------------------------------------------------------------*
 * STATE MANAGEMENT                                                          *
 *---------------------------------------------------------------------------*/
uint8_t GameIO_GetState(void);
void GameIO_SetState(uint8_t state);

/*---------------------------------------------------------------------------*
 * CONFIGURATION (read by game engine, written by driver via UART)          *
 *---------------------------------------------------------------------------*/
uint16_t GameIO_GetSpeed(void);
uint8_t GameIO_GetWallDifficulty(void);
uint8_t GameIO_GetBoundaryMode(void);
/* Нові функції для встановлення/отримання рівня */
void GameIO_SetSelectedLevel(uint8_t level);
uint8_t GameIO_GetSelectedLevel(void);

/*---------------------------------------------------------------------------*
 * OUTPUT (called by game engine to send data to PC)                        *
 *---------------------------------------------------------------------------*/
void GameIO_SendDelta(uint8_t head_x, uint8_t head_y,
                      uint8_t x2, uint8_t y2, uint8_t wall_flag);

/**
 * @brief Send score and snake length to PC
 * @param snake_len Current snake length (1 byte)
 * @param score Current score (2 bytes)
 *
 * Packet format: CMD_SCORE | snake_len (1 byte) | score_high | score_low
 */
void GameIO_SendScore(uint8_t snake_len, uint16_t score);

void GameIO_SendState(uint8_t state);

/*---------------------------------------------------------------------------*
 * COMMAND CALLBACK                                                          *
 *---------------------------------------------------------------------------*/
typedef void (*GameIO_CommandCallback)(uint8_t cmd, uint8_t *payload);
void GameIO_RegisterCallback(GameIO_CommandCallback cb);

/*---------------------------------------------------------------------------*
 * INITIALIZATION                                                            *
 *---------------------------------------------------------------------------*/
void GameIO_Init(void);

/*---------------------------------------------------------------------------*
 * DEBUG MODE                                                                *
 *---------------------------------------------------------------------------*/
/**
 * @brief Get current debug mode state.
 * @return Debug mode value (0..6) from debug module
 */
uint8_t GameIO_GetDebugMode(void);

// REMOVED: void GameIO_SetDebugMode(uint8_t mode);

#endif /* GAME_IO_H */
