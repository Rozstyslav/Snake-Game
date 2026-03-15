/**
 * @file game_config.h
 * @brief Central configuration file for the Snake game.
 */

#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <stdint.h>

 /* Button timing (milliseconds) */
#define BUTTON_DEBOUNCE_MS       50
#define BUTTON_SHORT_PRESS_MS    800
#define BUTTON_LONG_PRESS_MS     2000
#define BUTTON_GLITCH_IGNORE_MS  10    /**< Ignore presses shorter than this (anti-glitch) */

/* LED blink half-periods (milliseconds) */
#define LED_SLOW_BLINK_HALF      500
#define LED_FAST_BLINK_HALF      125

/* UART timeouts (milliseconds) */
#define UART_ACK_TIMEOUT         10
#define UART_PACKET_TIMEOUT      100

/* Game board dimensions */
#define BOARD_WIDTH              50
#define BOARD_HEIGHT             50

/* Initial head position (centered) */
#define START_HEAD_X             (BOARD_WIDTH / 2)
#define START_HEAD_Y             (BOARD_HEIGHT / 2)

/* Gameplay parameters */
#define FOOD_GENERATION_INTERVAL 10

/* Debug mode percentages */
#define DEBUG_OUT_OF_RANGE_PCT   10
#define DEBUG_CRC_ERROR_PCT      15

#endif /* GAME_CONFIG_H */