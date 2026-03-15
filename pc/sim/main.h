#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file main.h
 * @brief Simulator replacement for Core/Inc/main.h from CubeIDE.
 *
 * Pulls in the HAL stub (which itself includes sim_config.h) and
 * re-declares the same GPIO pin aliases as the original CubeIDE main.h
 * so that the game-engine sources compile unchanged.
 */

#include "hal_stub.h"
#include "game_config.h"

/* GPIO pin aliases – identical to the original CubeIDE main.h */
#define B1_Pin          GPIO_PIN_0
#define B1_GPIO_Port    GPIOA
#define LD4_Pin         GPIO_PIN_8
#define LD4_GPIO_Port   GPIOC
#define LD3_Pin         GPIO_PIN_9
#define LD3_GPIO_Port   GPIOC

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
