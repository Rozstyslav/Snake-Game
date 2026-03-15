/**
 * @file uart_protocol.c
 * @author Vasyl
 * @version 1.7
 * @date 2026-02-19
 * @brief UART protocol implementation for Snake game.
 *
 * Implements 8‑byte fixed‑length packet protocol with XOR checksum,
 * Little‑Endian multi‑byte fields, and ACK/NACK flow control.
 * Fully compliant with uart_protocol.h v1.6 and specification v1.7.
 *
 * CHANGES in v1.7:
 * - UART_SendScore() now includes snake_len in payload[0]
 * - Score moved to payload[1-2] (Little-Endian)
 *
 * @dependencies uart_protocol.h, main.h, game_config.h
 */
#include "game_config.h"
#include "uart_protocol.h"
#include "main.h"                 /* HAL functions, UART handle */


/*---------------------------------------------------------------------------*
 * Private variables (module context)                                        *
 *---------------------------------------------------------------------------*/
static UART_CommandHandler cmd_handler = NULL; /**< Registered command callback */
uint8_t uart_last_crc_error = 0;   /**< Set to 1 if last packet had CRC error */
uint8_t uart_packet_received = 0;  /**< Set to 1 when a valid packet (good CRC) is received */
static UART_HandleTypeDef *huart_protocol = NULL; /**< UART handle (set via init) */

/* Packet reception state machine */
static uint8_t rx_state = 0;      /**< 0 = waiting for 0xAA, 1 = receiving packet */
static uint8_t rx_buffer[UART_PACKET_SIZE]; /**< Accumulates one full packet */
static uint8_t rx_index = 0;      /**< Current index in rx_buffer */

/*---------------------------------------------------------------------------*
 * Public functions                                                          *
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialise the UART protocol module.
 * @param huart Pointer to HAL UART handle.
 */
void UART_Protocol_Init(UART_HandleTypeDef *huart)
{
    huart_protocol = huart;
}

/**
 * @brief Register a callback for received commands.
 * @param handler Function pointer to handle commands (cmd, payload).
 */
void UART_RegisterHandler(UART_CommandHandler handler)
{
    cmd_handler = handler;
}

/**
 * @brief Compute XOR checksum over a byte array.
 * @param data Pointer to data buffer.
 * @param length Number of bytes to process.
 * @return XOR of all bytes.
 */
uint8_t UART_CalculateChecksum(uint8_t *data, uint16_t length)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Send an 8‑byte packet over UART.
 * @param command Command byte (CMD_* from PC→STM32 or STM32→PC).
 * @param payload Pointer to up to 5 bytes of data. Unused bytes are zeroed.
 * @param len Number of valid payload bytes (0 ≤ len ≤ 5).
 */
void UART_SendPacket(uint8_t command, const uint8_t *payload, uint16_t len)
{
    if (huart_protocol == NULL) {
        return; /* Protocol not initialised */
    }

    UART_Packet_t packet = {
        .header = 0xAA,
        .command = command,
        .data = {0},
        .crc = 0
    };

    /* Copy payload (max 5 bytes, rest already zeroed) */
    for (uint16_t i = 0; i < len && i < 5; i++) {
        packet.data[i] = payload[i];
    }

    /* Compute XOR of first 7 bytes */
    packet.crc = UART_CalculateChecksum((uint8_t*)&packet, UART_PACKET_SIZE - 1);

    /* Transmit the 8‑byte packet (blocking) */
    HAL_UART_Transmit(huart_protocol, (uint8_t*)&packet, sizeof(packet), UART_PACKET_TIMEOUT);
}

/**
 * @brief Process one received byte (UART Rx state machine).
 * @param byte The byte received from UART.
 */
void UART_ProcessByte(uint8_t byte)
{
    if (huart_protocol == NULL) {
        return; /* Cannot send ACK/NACK – protocol not initialised */
    }

    /* State 0: waiting for start byte (0xAA) */
    if (rx_state == 0) {
        if (byte == 0xAA) {
            rx_buffer[0] = byte;
            rx_index = 1;
            rx_state = 1;
        }
        /* Ignore all other bytes */
        return;
    }

    /* State 1: accumulating packet */
    rx_buffer[rx_index++] = byte;

    /* Packet complete (UART_PACKET_SIZE bytes received) */
    if (rx_index >= UART_PACKET_SIZE) {
        uint8_t reply = NACK; /* Default reply */
        uint8_t cmd = rx_buffer[UART_CMD_POS];
        uint8_t calc_crc = UART_CalculateChecksum(rx_buffer, UART_PACKET_SIZE - 1);

        /* 1. Verify CRC */
        if (calc_crc == rx_buffer[UART_CRC_OFFSET]) {
            uart_packet_received = 1;
            uart_last_crc_error = 0;

            /* 2. Check if command is valid (PC → STM32 range) */
            if (cmd >= 0x01 && cmd <= 0x1F) {
                /* Known commands – may be extended later */
                switch (cmd) {
                    case CMD_SET_STATE:
                    case CMD_MOVE:
                    case CMD_CONFIG:
                    case CMD_DEBUG:
                        reply = ACK;
                        break;
                    default:
                        reply = NACK; /* Unknown command in valid range */
                        break;
                }
            } else {
                reply = NACK; /* Command outside PC→STM32 range */
            }
        } else {
            reply = NACK; /* CRC error */
            uart_last_crc_error = 1;
            uart_packet_received = 0;
        }

        /* 3. Send ACK or NACK (single byte) */
        HAL_UART_Transmit(huart_protocol, &reply, 1, UART_ACK_TIMEOUT);

        /* 4. Execute command only if ACK was sent and handler registered */
        if (reply == ACK && cmd_handler != NULL) {
            /* Pass command and payload (bytes 2-6) to the registered handler */
            cmd_handler(cmd, &rx_buffer[UART_PAYLOAD_OFFSET]);
        }

        /* 5. Reset state machine for next packet */
        rx_state = 0;
        rx_index = 0;
    }
}

/**
 * @brief Send current game score with snake length (CMD_SCORE).
 * @param snake_len Current snake length (1-100).
 * @param score Score value (0–65535).
 *
 * Packet payload format (Little-Endian):
 * - payload[0]: snake_len (1 byte)
 * - payload[1]: score LSB (low byte)
 * - payload[2]: score MSB (high byte)
 * - payload[3-4]: unused (0x00)
 */
void UART_SendScore(uint8_t snake_len, uint16_t score)
{
    uint8_t payload[5] = {0};
    payload[0] = snake_len;                      /* Snake length */
    payload[1] = (uint8_t)(score & 0xFF);        /* Score LSB */
    payload[2] = (uint8_t)((score >> 8) & 0xFF); /* Score MSB */
    UART_SendPacket(CMD_SCORE, payload, 3);
}

/**
 * @brief Send current game state (CMD_STATE).
 * @param state Current game state (0..4).
 */
void UART_SendState(GameState_t state)
{
    uint8_t payload[UART_PAYLOAD_SIZE] = {0};
    payload[0] = (uint8_t)state;
    UART_SendPacket(CMD_STATE, payload, 1);
}

/*---------------------------------------------------------------------------*
 * Static assertion: ensure packet structure has no padding                  *
 *---------------------------------------------------------------------------*/
_Static_assert(sizeof(UART_Packet_t) == UART_PACKET_SIZE,
               "UART_Packet_t must be exactly UART_PACKET_SIZE bytes");
