#ifndef MBCOMM_H
#define MBCOMM_H

extern void mb_setup();
extern void mb_handle();
extern void mb_writeReg(uint16_t reg, uint16_t val);

extern uint32_t  modbusLastTime;
extern uint32_t  modbusCycleTime;
extern uint8_t   modbusResultCode;
extern uint16_t  content[WB_CNT][55];

#endif /* MBCOMM_H */
