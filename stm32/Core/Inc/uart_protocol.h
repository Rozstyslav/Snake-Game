/**
 * @file uart_protocol.h
 * @author Vasyl
 * @version 1.5
 * @date 2026-02-16
 * @brief UART protocol definitions for Snake game.
 *
 * This header defines the communication protocol between STM32 (Game Engine)
 * and PC (Display/Controller). It is fully compliant with protocol specification
 * v1.5 (uart_protocol.md).
 *
 * @dependencies uart_protocol.md v1.6
 * @note All multi-byte fields are transmitted in Little-Endian order.
 */

#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>

 /*---------------------------------------------------------------------------*
  * Packet size definition                                                   *
  *---------------------------------------------------------------------------*/
#define UART_PACKET_SIZE        8   /**< Total packet size in bytes */
#define UART_PAYLOAD_SIZE       5   /**< Number of payload bytes in packet */
#define UART_CRC_OFFSET         7   /**< Index of CRC byte (0‑based) */
#define UART_HEADER_POS         0   /**< Header byte position */
#define UART_CMD_POS            1   /**< Command byte position */
#define UART_PAYLOAD_OFFSET     2   /**< Index of first payload byte (after header and command) */

  /*---------------------------------------------------------------------------*
   * Forward declaration of HAL UART handle (avoids including full HAL header) *
   *---------------------------------------------------------------------------*/
typedef struct __UART_HandleTypeDef UART_HandleTypeDef;

/*---------------------------------------------------------------------------*
 * Commands: PC -> STM32 (range 0x01 - 0x1F)                                 *
 *---------------------------------------------------------------------------*/
#define CMD_SET_STATE   0x01    /**< Set game state (payload[0] = new state 0..4) */
#define CMD_MOVE        0x02    /**< Change direction: 0=Up,1=Down,2=Left,3=Right */
#define CMD_CONFIG      0x03    /**< Configure game: payload[0]=setting_id, payload[1]=value */
#define CMD_DEBUG       0x10    /**< Debug mode: 0x01 = Chaos, 0x00 = Normal */

 /*---------------------------------------------------------------------------*
  * CMD_CONFIG (0x03) setting IDs and values                                  *
  *---------------------------------------------------------------------------*/

  /* Setting IDs (payload[0]) */
#define CONFIG_SPEED     0x00    /**< Game speed setting */
#define CONFIG_WALLS     0x01    /**< Wall difficulty setting */
#define CONFIG_BOUNDARY  0x02    /**< Boundary behavior setting */

/* Speed values (payload[1] when setting_id = CONFIG_SPEED) */
#define SPEED_SLOW       0x00    /**< 400ms per step */
#define SPEED_MEDIUM     0x01    /**< 200ms per step (default) */
#define SPEED_FAST       0x02    /**< 100ms per step */

/* Wall difficulty values (payload[1] when setting_id = CONFIG_WALLS) */
#define WALLS_NONE       0x00    /**< No walls (default) */
#define WALLS_EASY       0x01    /**< Walls every ~50 steps */
#define WALLS_HARD       0x02    /**< Walls every ~20 steps */

/* Boundary behavior values (payload[1] when setting_id = CONFIG_BOUNDARY) */
#define BOUNDARY_WRAP    0x00    /**< Wrap around edges (toroidal, default) */
#define BOUNDARY_GAMEOVER 0x01   /**< Game over on boundary hit */

/*---------------------------------------------------------------------------*
 * Commands: STM32 -> PC (range 0x81 - 0x9F)                                 *
 *---------------------------------------------------------------------------*/
#define CMD_STATE       0x81    /**< Report current game state (0..4) */
#define CMD_DELTA       0x82    /**< Game update: head (x,y), new food (x,y), wall flag */
#define CMD_SCORE       0x83    /**< Current score (Little-Endian, 2 bytes) */

 /*---------------------------------------------------------------------------*
  * Flow control                                                              *
  *---------------------------------------------------------------------------*/
#define ACK             0x06    /**< Packet accepted, CRC valid, known command */
#define NACK            0x15    /**< Packet rejected – CRC error or unknown command */

  /*---------------------------------------------------------------------------*
   * Game states (used in CMD_STATE and current_state)                         *
   *---------------------------------------------------------------------------*/
typedef enum {
    STATE_MENU = 0,        /**< Main menu, waiting for START */
    STATE_PLAYING = 1,        /**< Game is active */
    STATE_PAUSE = 2,        /**< Game paused */
    STATE_GAMEOVER = 3,        /**< Game over, waiting for return to menu */
    STATE_SLEEP = 4         /**< Low-power sleep mode */
} GameState_t;

/*---------------------------------------------------------------------------*
 * 8‑byte fixed packet structure (must be packed)                           *
 *---------------------------------------------------------------------------*/
 /**
  * @brief UART packet format (total 8 bytes, no alignment padding).
  *
  * Byte order:
  * 0: Header (0xAA)
  * 1: Command
  * 2-6: Payload (5 bytes, unused bytes = 0x00)
  * 7: CRC (XOR of bytes 0..6)
  */
  /* Cross-platform packed struct */
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
typedef struct
#ifndef _MSC_VER
__attribute__((__packed__))
#endif
{
    uint8_t header;             /**< Always 0xAA */
    uint8_t command;            /**< Command code (CMD_*) */
    uint8_t data[5];            /**< Payload data */
    uint8_t crc;                /**< XOR checksum of previous 7 bytes */
} UART_Packet_t;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

/*---------------------------------------------------------------------------*
 * Payload structure for CMD_DELTA (0x82)                                    *
 *---------------------------------------------------------------------------*/
 /**
  * @brief Delta update payload format (5 bytes):
  * - Byte 0: New head X1 coordinate
  * - Byte 1: New head Y1 coordinate
  * - Byte 2: New food or last tail piece to delete X2 coordinate
  * - Byte 3: New food or last tail piece to delete Y2 coordinate
  * - Byte 4: Wall flag (0 = normal update, 1 = place wall at both x1, y1; x2, y2)
  */
  /* (No separate structure needed, just documentation) */

  /*---------------------------------------------------------------------------*
   * Public function prototypes                                                *
   *---------------------------------------------------------------------------*/

   /**
    * @brief Initialise the UART protocol module.
    * @param huart Pointer to HAL UART handle (e.g. &huart1).
    * @note Must be called once before any other protocol functions.
    */
void UART_Protocol_Init(UART_HandleTypeDef* huart);

/**
 * @brief Compute XOR checksum over a byte array.
 * @param data Pointer to the data buffer.
 * @param length Number of bytes to process.
 * @return XOR of all bytes in the buffer.
 */
uint8_t UART_CalculateChecksum(uint8_t* data, uint16_t length);

/**
 * @brief Send an 8‑byte packet over UART.
 * @param command Command byte (CMD_* from PC→STM32 or STM32→PC).
 * @param payload Pointer to up to 5 bytes of data. If less than 5, remaining
 *                bytes are padded with 0x00.
 * @param len Number of valid payload bytes (0 ≤ len ≤ 5).
 */
void UART_SendPacket(uint8_t command, const uint8_t* payload, uint16_t len);

/**
 * @brief Process one received byte (to be called from UART Rx callback).
 * @param byte The byte received from UART.
 *
 * This function implements a finite state machine that detects the 0xAA header,
 * collects 8‑byte packets, verifies CRC, sends ACK/NACK, and dispatches valid
 * commands. The application must call this function each time a byte arrives.
 */
void UART_ProcessByte(uint8_t byte);

/**
 * @brief Send current game score (CMD_SCORE).
 * @param score Score value (0–65535).
 * @note Score is transmitted as Little-Endian over two bytes.
 */
void UART_SendScore(uint8_t length, uint16_t score);

/**
 * @brief Send game state update (CMD_STATE).
 * @param state Current game state (0..4).
 */
void UART_SendState(GameState_t state);

/**
 * @brief Register a callback for received commands.
 * @param handler Function pointer to handle commands (cmd, payload).
 *        Payload points to 5 bytes of data from the packet.
 */
typedef void (*UART_CommandHandler)(uint8_t cmd, uint8_t* payload);
void UART_RegisterHandler(UART_CommandHandler handler);

#endif /* UART_PROTOCOL_H */