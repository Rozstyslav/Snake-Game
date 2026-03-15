/**
 * @file game_config.h
 * @brief Central configuration file for the Snake game.
 *
 * All tuneable parameters are gathered here so that related modules
 * share the same source of truth and magic numbers are eliminated.
 */

#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <stdint.h>

 /*---------------------------------------------------------------------------*
  * Button timing (milliseconds)                                              *
  *---------------------------------------------------------------------------*/
#define BUTTON_DEBOUNCE_MS          50   /**< Minimum stable-HIGH time to confirm a press */
#define BUTTON_SHORT_PRESS_MS      800   /**< Release before this → short press */
#define BUTTON_LONG_PRESS_MS      2000   /**< Release before this → long press */
#define BUTTON_GLITCH_IGNORE_MS     10   /**< Pulses shorter than this are ignored (anti-glitch) */

  /** Duration (ms) both LEDs blink on a very-long button press. */
#define BUTTON_VLONG_LED_MS        500

/*---------------------------------------------------------------------------*
 * LED blink half-periods (milliseconds)                                     *
 *---------------------------------------------------------------------------*/
#define LED_SLOW_BLINK_HALF        500   /**< Slow blink: 1 Hz, 500 ms half-period */
#define LED_FAST_BLINK_HALF        125   /**< Fast blink: 4 Hz, 125 ms half-period */

 /*---------------------------------------------------------------------------*
  * LED event flash durations (milliseconds)                                  *
  *---------------------------------------------------------------------------*/
#define LED_EVENT_ACK_MS           200   /**< Green fast-blink on ACK */
#define LED_EVENT_NACK_MS          200   /**< Red   fast-blink on NACK */
#define LED_EVENT_CRC_MS           300   /**< Both  fast-blink on CRC error */

  /*---------------------------------------------------------------------------*
   * LED debug-mode sequence flash timing (milliseconds)                       *
   *---------------------------------------------------------------------------*/
#define LED_SEQ_FLASH_MS           100   /**< ON time per individual flash */
#define LED_SEQ_PAUSE_MS           150   /**< OFF gap between consecutive flashes */

   /*---------------------------------------------------------------------------*
	* UART timeouts (milliseconds)                                              *
	*---------------------------------------------------------------------------*/
#define UART_ACK_TIMEOUT            10   /**< Timeout for sending ACK/NACK byte */
#define UART_PACKET_TIMEOUT        100   /**< Timeout for sending a full 8-byte packet */

	/*---------------------------------------------------------------------------*
	 * Game board dimensions                                                     *
	 *---------------------------------------------------------------------------*/
#define BOARD_WIDTH                 50
#define BOARD_HEIGHT                50

	 /** Initial head position (centred on the board). */
#define START_HEAD_X               (BOARD_WIDTH  / 2)
#define START_HEAD_Y               (BOARD_HEIGHT / 2)

/*---------------------------------------------------------------------------*
 * Snake and food parameters                                                 *
 *---------------------------------------------------------------------------*/
#define MAX_SNAKE_LEN              100   /**< Hard limit on snake segment count */
#define FOOD_GENERATION_INTERVAL    10   /**< Steps between food spawns (legacy) */
#define FOOD_FIND_MAX_ATTEMPTS    1000   /**< Give up trying to place food after this many retries */

 /*---------------------------------------------------------------------------*
  * Game speed: milliseconds per step for each speed setting                  *
  *---------------------------------------------------------------------------*/
#define SPEED_MS_SLOW              400   /**< Corresponds to SPEED_SLOW  (0x00) */
#define SPEED_MS_MEDIUM            200   /**< Corresponds to SPEED_MEDIUM (0x01) – default */
#define SPEED_MS_FAST              100   /**< Corresponds to SPEED_FAST  (0x02) */

  /*---------------------------------------------------------------------------*
   * Level selection                                                            *
   *---------------------------------------------------------------------------*/
   /**
	* Highest level index accepted by GameIO_SetSelectedLevel().
	* Levels 0-5 have explicit layouts; values above GAME_LEVEL_MAX_LAYOUT
	* fall through to the default (no walls) case in Setup_Level().
	*/
#define GAME_LEVEL_MAX               6

	/*---------------------------------------------------------------------------*
	 * Debug mode cycling                                                         *
	 *---------------------------------------------------------------------------*/
	 /**
	  * Total number of debug modes (DEBUG_OFF = 0 … DEBUG_STEP_MODE = 6).
	  * Used when cycling modes: next = (current + 1) % DEBUG_MODE_COUNT.
	  */
#define DEBUG_MODE_COUNT             7

	  /*---------------------------------------------------------------------------*
	   * Debug mode timing (milliseconds)                                          *
	   *---------------------------------------------------------------------------*/
#define DEBUG_RANDOM_INTERVAL_MS   500   /**< Period between random delta packets */
#define DEBUG_ERROR_INJECT_INTERVAL_MS 1000 /**< Period between injected CRC-error packets */

	   /*---------------------------------------------------------------------------*
		* Debug mode injection percentages                                          *
		*---------------------------------------------------------------------------*/
#define DEBUG_OUT_OF_RANGE_PCT      10   /**< Probability (%) of out-of-range coordinates */
#define DEBUG_CRC_ERROR_PCT         15   /**< Probability (%) of CRC errors in random mode */

		/*---------------------------------------------------------------------------*
		 * Persistent storage                                                        *
		 *---------------------------------------------------------------------------*/
		 /**
		  * Flash address for high-score storage.
		  * Uses the last 1 KB page of the STM32F0 64 KB Flash.
		  * Value 0xFFFF (erased state) is treated as "no record".
		  */
#define HIGHSCORE_FLASH_ADDR  0x0800FC00u

#endif /* GAME_CONFIG_H */