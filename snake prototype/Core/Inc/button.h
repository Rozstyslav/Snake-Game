#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

/**
 * @brief Initialize button module.
 */
void Button_Init(void);

/**
 * @brief Process button state machine. Call this periodically from main loop.
 */
void Button_Process(void);

#endif
