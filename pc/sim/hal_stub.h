#ifndef HAL_STUB_H
#define HAL_STUB_H

/**
 * @file hal_stub.h
 * @brief HAL stub replacing stm32f0xx_hal.h for Windows / MSVC builds.
 *
 * Provides the minimal subset of the STM32 HAL API needed by the game
 * engine and protocol layer, implemented using standard C and OS primitives.
 */

#include "sim_config.h"   /* SIM_TX_BUF_SIZE and other simulator constants */

/* ── MSVC compatibility (must appear before any HAL types) ───────────── */
#ifdef _MSC_VER
  /* MSVC does not understand GCC __attribute__ – silently discard it. */
  #define __attribute__(x)
  /* Packed structs under MSVC are handled via #pragma pack in uart_protocol.h. */
  /* Map _Static_assert to the built-in MSVC static_assert. */
  #ifndef _Static_assert
    #define _Static_assert static_assert
  #endif
#endif

/* ── Base HAL types ──────────────────────────────────────────────────── */
#include <stdint.h>
#include <stddef.h>

typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;

#define GPIO_PIN_0   ((uint16_t)0x0001)
#define GPIO_PIN_8   ((uint16_t)0x0100)
#define GPIO_PIN_9   ((uint16_t)0x0200)
#define GPIO_PIN_13  ((uint16_t)0x2000)
#define GPIO_PIN_14  ((uint16_t)0x4000)

/* ── Simulated GPIO ports (arbitrary non-NULL sentinel addresses) ─────── */
#define GPIOA  ((void *)0xA0)
#define GPIOC  ((void *)0xC0)

/* ── UART handle ─────────────────────────────────────────────────────── */
typedef struct __UART_HandleTypeDef {
    void *Instance;
} UART_HandleTypeDef;

/* ── HAL API ─────────────────────────────────────────────────────────── */
uint32_t          HAL_GetTick(void);
void              HAL_Delay(uint32_t ms);
HAL_StatusTypeDef HAL_Init(void);

void              HAL_GPIO_WritePin(void *GPIOx, uint16_t pin, GPIO_PinState state);
GPIO_PinState     HAL_GPIO_ReadPin(void *GPIOx, uint16_t pin);

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart,
                                    const uint8_t *pData, uint16_t Size,
                                    uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart,
                                      uint8_t *pData, uint16_t Size);

/* ── GPIO state array – accessible from ui.c ────────────────────────── */
#define GPIO_IDX_LED_GREEN  0
#define GPIO_IDX_LED_BLUE   1
#define GPIO_IDX_BUTTON     2

extern GPIO_PinState gpio_state[3];

/* ── UART transmit packet buffer ─────────────────────────────────────── */
/* SIM_TX_BUF_SIZE is defined in sim_config.h (included above). */
extern uint8_t  tx_buf[SIM_TX_BUF_SIZE];
extern uint16_t tx_buf_len;

/* ── Simulated button state ──────────────────────────────────────────── */
extern GPIO_PinState sim_button_state;

/* ── UART RX complete callback (implemented in sim_main.c) ───────────── */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* HAL_STUB_H */
