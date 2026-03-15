/**
 * @file sim_config.h
 * @brief Central configuration for the Snake PC simulator.
 *
 * All simulator-specific tuneable parameters are gathered here so that
 * magic numbers are eliminated and related modules share one source of
 * truth.  Game-engine constants remain in game_config.h.
 */

#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

/*---------------------------------------------------------------------------*
 * HAL stub – UART transmit buffer                                           *
 *---------------------------------------------------------------------------*/
/** Size of the simulated UART transmit ring buffer in bytes. */
#define SIM_TX_BUF_SIZE         1024

/*---------------------------------------------------------------------------*
 * Main loop timing                                                          *
 *---------------------------------------------------------------------------*/
/** Target duration (ms) of one main-loop iteration. */
#define SIM_LOOP_TICK_MS           1

/** How often (ms) the status bar is refreshed (~10 times per second). */
#define SIM_UI_RENDER_INTERVAL_MS 100

/*---------------------------------------------------------------------------*
 * Button simulation durations                                               *
 *---------------------------------------------------------------------------*/
/**
 * Simulated press durations must be consistent with the thresholds defined
 * in game_config.h:
 *   BUTTON_SHORT_PRESS_MS  = 800  ms   (release before  → short press)
 *   BUTTON_LONG_PRESS_MS   = 2000 ms   (release before  → long press)
 *
 *   SIM_BTN_SHORT_MS  < BUTTON_SHORT_PRESS_MS   → classified as short
 *   SIM_BTN_MEDIUM_MS in [SHORT, LONG)           → classified as long
 *   SIM_BTN_LONG_MS   >= BUTTON_LONG_PRESS_MS   → classified as very-long
 */
#define SIM_BTN_SHORT_MS         400   /**< Simulated short press (400 ms) */
#define SIM_BTN_MEDIUM_MS       1200   /**< Simulated medium/long press (1200 ms) */
#define SIM_BTN_LONG_MS         2500   /**< Simulated very-long press (2500 ms) */

/*---------------------------------------------------------------------------*
 * Terminal layout – geometry                                                *
 *---------------------------------------------------------------------------*/
#define TERM_ROWS                 45   /**< Total terminal row count */
#define TERM_COLS                110   /**< Total terminal column count (including borders) */
#define PANEL_ROWS   (TERM_ROWS - 6)   /**< Usable content rows per panel */

/** Panel column positions and widths. */
#define COL_L          2   /**< Left   panel: first column */
#define COL_L_W       36   /**< Left   panel: character width */
#define COL_M         40   /**< Middle panel: first column */
#define COL_M_W       30   /**< Middle panel: character width */
#define COL_R         72   /**< Right  panel: first column */
#define COL_R_W       38   /**< Right  panel: character width */

/** Panel row positions. */
#define ROW_STATUS     2                   /**< Status bar row */
#define ROW_HDR        4                   /**< Panel header row */
#define ROW_CONTENT    5                   /**< First content row */
#define ROW_INPUT      (TERM_ROWS - 1)     /**< Command input row */

/*---------------------------------------------------------------------------*
 * UI buffer sizes                                                           *
 *---------------------------------------------------------------------------*/
/** Lines retained in each panel's scroll-back buffer. */
#define UI_LINE_HISTORY          400

/** Maximum characters per display line (including null terminator margin). */
#define UI_LINE_WIDTH             96

/** Maximum wall pairs stored for display/visualisation. */
#define UI_MAX_WALLS             200

/** Command input buffer size (characters). */
#define UI_CMD_BUF_LEN           128

/** Maximum number of wall entries shown by the 'walls' command. */
#define UI_WALLS_DISPLAY_MAX      20

/** Grid-mark column interval for the ASCII wall map. */
#define UI_MAP_GRID_STEP          10

/** Delay (ms) inserted between steps in automated wall tests. */
#define SIM_TEST_STEP_DELAY_MS   100

/*---------------------------------------------------------------------------*
 * Logger buffer sizes                                                       *
 *---------------------------------------------------------------------------*/
/** Log filename string buffer length (e.g. "snake_sim_20260101_120000.log"). */
#define LOG_FILENAME_LEN          64

/** Timestamp string buffer: "HH:MM:SS.mmm\0" needs 13 bytes; 16 gives margin. */
#define LOG_TIMESTAMP_LEN         16

/** Decoded packet description string buffer. */
#define LOG_DECODED_LEN           80

/** Hex-dump string buffer (8 bytes × 3 chars + null). */
#define LOG_HEX_LEN               32

/** General-purpose log line buffer. */
#define LOG_LINE_LEN             128

/** Width of the separator line written in the log file header. */
#define LOG_SEPARATOR_LEN         80

/*---------------------------------------------------------------------------*
 * Sentinel values                                                           *
 *---------------------------------------------------------------------------*/
/**
 * Sentinel used to mark "not yet observed" in prev_state / prev_dbg
 * change-detection variables.  0xFF is safe because valid state values
 * are 0..4 and valid debug-mode values are 0..6.
 */
#define SIM_PREV_UNINIT          0xFFu

/*---------------------------------------------------------------------------*
 * UART packet header byte (simulator-side reference)                       *
 *---------------------------------------------------------------------------*/
/**
 * Start byte of every UART packet (mirrors UART_Packet_t.header).
 * Defined here for use in simulator files that build / inspect raw packets
 * without pulling in the full game protocol implementation.
 */
#define SIM_UART_PACKET_HEADER   0xAAu

#endif /* SIM_CONFIG_H */
