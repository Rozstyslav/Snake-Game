/**
 * @file hal_stub.c
 * @brief HAL function implementations for the simulator on Windows / Linux.
 *
 * Provides GPIO state tracking, a UART transmit accumulation buffer,
 * and platform-specific timing primitives using Win32 or POSIX APIs.
 */

#define _POSIX_C_SOURCE 200809L
#include "hal_stub.h"   /* also pulls in sim_config.h */
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

/* ── GPIO state ──────────────────────────────────────────────────────── */
GPIO_PinState gpio_state[3] = {
    GPIO_PIN_RESET,   /* LED_GREEN: off at startup */
    GPIO_PIN_RESET,   /* LED_BLUE:  off at startup */
    GPIO_PIN_RESET    /* BUTTON:    released at startup */
};

/* Simulated button line state; released (LOW) by default. */
GPIO_PinState sim_button_state = GPIO_PIN_RESET;

/* ── UART transmit buffer ─────────────────────────────────────────────
 * SIM_TX_BUF_SIZE comes from sim_config.h (via hal_stub.h).
 * The local #define that used to duplicate this value has been removed.  */
uint8_t  tx_buf[SIM_TX_BUF_SIZE];
uint16_t tx_buf_len = 0;

/* ── HAL_GetTick ─────────────────────────────────────────────────────── */
uint32_t HAL_GetTick(void)
{
#ifdef _WIN32
    return (uint32_t)GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
#endif
}

/* ── HAL_Delay ───────────────────────────────────────────────────────── */
void HAL_Delay(uint32_t ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts = { (time_t)(ms / 1000u),
                           (long)((ms % 1000u) * 1000000L) };
    nanosleep(&ts, NULL);
#endif
}

HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }

/* ── GPIO WritePin ───────────────────────────────────────────────────── */
void HAL_GPIO_WritePin(void *GPIOx, uint16_t pin, GPIO_PinState state)
{
    if (GPIOx == GPIOC) {
        if (pin == GPIO_PIN_9) gpio_state[GPIO_IDX_LED_GREEN] = state;
        if (pin == GPIO_PIN_8) gpio_state[GPIO_IDX_LED_BLUE]  = state;
    }
}

/* ── GPIO ReadPin ────────────────────────────────────────────────────── */
GPIO_PinState HAL_GPIO_ReadPin(void *GPIOx, uint16_t pin)
{
    if (GPIOx == GPIOA && pin == GPIO_PIN_0)
        return sim_button_state;
    return GPIO_PIN_RESET;
}

/* ── UART Transmit ────────────────────────────────────────────────────
 * Appends transmitted bytes to tx_buf so that UI_FlushTxPackets() can
 * decode and display them.  Silently drops data if the buffer is full. */
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart,
                                    const uint8_t *pData, uint16_t Size,
                                    uint32_t Timeout)
{
    (void)huart;
    (void)Timeout;

    if (tx_buf_len + Size <= SIM_TX_BUF_SIZE) {
        memcpy(tx_buf + tx_buf_len, pData, Size);
        tx_buf_len += Size;
    }
    /* Buffer overflow: data is silently dropped.
     * If this becomes a problem, increase SIM_TX_BUF_SIZE in sim_config.h. */
    return HAL_OK;
}

/* ── UART Receive_IT ──────────────────────────────────────────────────
 * No-op in the simulator: bytes are injected directly by UI_InjectPacket()
 * via HAL_UART_RxCpltCallback() rather than through DMA/interrupt.       */
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart,
                                      uint8_t *pData, uint16_t Size)
{
    (void)huart;
    (void)pData;
    (void)Size;
    return HAL_OK;
}
