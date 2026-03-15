#ifndef UI_H
#define UI_H

#include <stdint.h>

void UI_Init(void);
void UI_RenderStatus(void);
void UI_FlushTxPackets(void);
void UI_ProcessInput(void);
void UI_InjectPacket(uint8_t cmd, uint8_t p0, uint8_t p1,
    uint8_t p2, uint8_t p3, uint8_t p4);
void UI_UpdateButton(void);

#endif