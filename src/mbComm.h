// Copyright (c) 2021 steff393

#include <globalConfig.h>

#ifndef MBCOMM_H
#define MBCOMM_H

extern void mb_getAscii(uint8_t id, uint8_t from, uint8_t len, char *result);
extern void mb_setup();
extern void mb_loop();
extern void mb_writeReg(uint8_t id, uint16_t reg, uint16_t val);

extern uint16_t  content[WB_CNT][55];
extern uint32_t  modbusLastTime;
extern uint8_t   modbusResultCode[WB_CNT];


#endif /* MBCOMM_H */
