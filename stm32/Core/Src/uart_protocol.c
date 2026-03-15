/**
 * @file uart_protocol.c
 * @author Vasyl
 * @version 1.6
 * @date 2026-02-17
 * @brief UART protocol implementation for Snake game.
 *
 * Implements an 8-byte fixed-length packet protocol with XOR checksum,
 * Little-Endian multi-byte fields, and ACK/NACK flow control.
 * Fully compliant with uart_protocol.h v1.5 and specification v1.6.
 *
 * @dependencies uart_protocol.h, main.h, game_config.h
 */

#include "game_config.h"
#include "uart_protocol.h"
#include "main.h"   /* HAL functions, UART handle */

 /*---------------------------------------------------------------------------*
  * Private variables (module context)                                        *
  *---------------------------------------------------------------------------*/
static UART_CommandHandler    cmd_handler = NULL;  /**< Registered command callback */
uint8_t uart_last_crc_error = 0u;  /**< Set to 1 when the last packet had a CRC error */
uint8_t uart_packet_received = 0u;  /**< Set to 1 when a valid packet (good CRC) is received */
static UART_HandleTypeDef* huart_protocol = NULL;  /**< UART handle (set via init) */

/* Packet reception state machine */
static uint8_t rx_state = 0u;                        /**< 0 = waiting for 0xAA header */
static uint8_t rx_buffer[UART_PACKET_SIZE];            /**< Accumulates one full packet */
static uint8_t rx_index = 0u;                        /**< Current write index in rx_buffer */

/*---------------------------------------------------------------------------*
 * Public functions                                                          *
 *---------------------------------------------------------------------------*/

 /**
  * @brief Initialise the UART protocol module.
  * @param huart Pointer to the HAL UART handle.
  */
void UART_Protocol_Init(UART_HandleTypeDef* huart)
{
    huart_protocol = huart;
}

/**
 * @brief Register a callback for received commands.
 * @param handler Function pointer: void handler(uint8_t cmd, uint8_t *payload).
 */
void UART_RegisterHandler(UART_CommandHandler handler)
{
    cmd_handler = handler;
}

/**
 * @brief Compute XOR checksum over a byte array.
 * @param data   Pointer to the data buffer.
 * @param length Number of bytes to process.
 * @return XOR of all bytes in the buffer.
 */
uint8_t UART_CalculateChecksum(uint8_t* data, uint16_t length)
{
    uint8_t  checksum = 0u;
    uint16_t i;
    for (i = 0u; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Send an 8-byte packet over UART.
 * @param command Command byte (CMD_* from PC->STM32 or STM32->PC).
 * @param payload Pointer to up to 5 bytes of data. Unused bytes are zero-padded.
 * @param len     Number of valid payload bytes (0 <= len <= 5).
 */
void UART_SendPacket(uint8_t command, const uint8_t* payload, uint16_t len)
{
    /* Variable declaration at block start for C89 / MSVC compatibility. */
    UART_Packet_t packet;
    uint16_t      i;

    if (huart_protocol == NULL) {
        return;   /* Protocol not initialised; cannot transmit. */
    }

    packet.header = 0xAAu;
    packet.command = command;
    packet.data[0] = packet.data[1] = packet.data[2] =
        packet.data[3] = packet.data[4] = 0u;
    packet.crc = 0u;

    /* Copy payload bytes (max UART_PAYLOAD_SIZE; remainder stays zero). */
    for (i = 0u; i < len && i < UART_PAYLOAD_SIZE; i++) {
        packet.data[i] = payload[i];
    }

    /* Compute XOR checksum over the first 7 bytes. */
    packet.crc = UART_CalculateChecksum((uint8_t*)&packet, UART_PACKET_SIZE - 1u);

    /* Transmit the full 8-byte packet (blocking). */
    HAL_UART_Transmit(huart_protocol, (uint8_t*)&packet, sizeof(packet), UART_PACKET_TIMEOUT);
}

/**
 * @brief Process one received byte (UART Rx state machine).
 * @param byte The byte received from UART.
 *
 * This FSM detects the 0xAA header, collects a full 8-byte packet,
 * verifies the CRC, sends ACK or NACK, and dispatches valid commands.
 * Must be called from the UART Rx callback for every incoming byte.
 */
void UART_ProcessByte(uint8_t byte)
{
    if (huart_protocol == NULL) {
        return;   /* Cannot send ACK/NACK – protocol not initialised. */
    }

    /* State 0: waiting for the start byte (0xAA). */
    if (rx_state == 0u) {
        if (byte == 0xAAu) {
            rx_buffer[0] = byte;
            rx_index = 1u;
            rx_state = 1u;
        }
        /* All other bytes are ignored (inter-packet noise). */
        return;
    }

    /* State 1: accumulating packet bytes. */
    rx_buffer[rx_index++] = byte;

    /* Packet complete once UART_PACKET_SIZE bytes have been received. */
    if (rx_index >= UART_PACKET_SIZE) {
        uint8_t reply = NACK;   /* default reply */
        uint8_t cmd = rx_buffer[UART_CMD_POS];
        uint8_t calc_crc = UART_CalculateChecksum(rx_buffer, UART_PACKET_SIZE - 1u);

        /* 1. Verify CRC. */
        if (calc_crc == rx_buffer[UART_CRC_OFFSET]) {
            uart_packet_received = 1u;
            uart_last_crc_error = 0u;

            /* 2. Check whether the command belongs to the PC->STM32 range. */
            if (cmd >= 0x01u && cmd <= 0x1Fu) {
                switch (cmd) {
                case CMD_SET_STATE:
                case CMD_MOVE:
                case CMD_CONFIG:
                case CMD_DEBUG:
                    reply = ACK;
                    break;
                default:
                    reply = NACK;   /* Command is in range but unknown. */
                    break;
                }
            }
            else {
                reply = NACK;       /* Command is outside the PC->STM32 range. */
            }
        }
        else {
            reply = NACK;
            uart_last_crc_error = 1u;
            uart_packet_received = 0u;
        }

        /* 3. Send ACK or NACK (single byte). */
        HAL_UART_Transmit(huart_protocol, &reply, 1u, UART_ACK_TIMEOUT);

        /* 4. Dispatch command only when ACK was sent and a handler is registered. */
        if (reply == ACK && cmd_handler != NULL) {
            cmd_handler(cmd, &rx_buffer[UART_PAYLOAD_OFFSET]);
        }

        /* 5. Reset state machine for the next packet. */
        rx_state = 0u;
        rx_index = 0u;
    }
}

/**
 * @brief Send the current game score (CMD_SCORE).
 * @param length Current snake length (1 byte).
 * @param score  Score value (0-65535), transmitted Little-Endian over 2 bytes.
 */
void UART_SendScore(uint8_t length, uint16_t score)
{
    uint8_t payload[UART_PAYLOAD_SIZE] = { 0u };
    payload[0] = length;                             /* snake length */
    payload[1] = (uint8_t)(score & 0xFFu);           /* score low byte  */
    payload[2] = (uint8_t)((score >> 8u) & 0xFFu);  /* score high byte */
    /* payload[3] and payload[4] remain 0. */
    UART_SendPacket(CMD_SCORE, payload, 3u);
}

/**
 * @brief Send the current game state (CMD_STATE).
 * @param state Current game state (0..4).
 */
void UART_SendState(GameState_t state)
{
    uint8_t payload[UART_PAYLOAD_SIZE] = { 0u };
    payload[0] = (uint8_t)state;
    UART_SendPacket(CMD_STATE, payload, 1u);
}

/*---------------------------------------------------------------------------*
 * Static assertion: ensure the packet structure has no padding              *
 *---------------------------------------------------------------------------*/
_Static_assert(sizeof(UART_Packet_t) == UART_PACKET_SIZE,
    "UART_Packet_t must be exactly UART_PACKET_SIZE bytes");