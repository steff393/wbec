// Copyright (c) 2021 steff393

#include <globalConfig.h>

#ifndef MBCOMM_H
#define MBCOMM_H

extern void mb_getAscii(uint8_t id, uint8_t from, uint8_t len, char *result);
extern void mb_setup();
extern void mb_loop();
extern void mb_writeReg(uint8_t id, uint16_t reg, uint16_t val);
extern bool mb_isCarConnected(uint8_t id);
extern uint16_t mb_version(uint8_t id);
extern uint16_t mb_status(uint8_t id);
extern uint16_t mb_temperature(uint8_t id);
extern uint16_t mb_extLock(uint8_t id);
extern uint16_t mb_power(uint8_t id);
extern uint16_t mb_amperageLimit(uint8_t id);
extern uint16_t mb_amperageMaximum(uint8_t id);
extern uint16_t mb_amperageMinimum(uint8_t id);
extern uint16_t mb_chargingRequested(uint8_t id);
extern uint32_t mb_energyTotal(uint8_t id);
extern uint32_t mb_energyPowerOn(uint8_t id);
extern const std::array<uint16_t,3>& mb_voltages(uint8_t id);
extern const std::array<uint16_t,3>& mb_amperages(uint8_t id);
extern uint16_t mb_watchdogTimeout(uint8_t id);
extern uint16_t mb_standby(uint8_t id);
extern uint16_t mb_remLock(uint8_t id);
extern uint16_t mb_amperageFailsafe(uint8_t id);

extern uint32_t  modbusLastTime;
extern uint8_t   modbusResultCode[WB_CNT];


#endif /* MBCOMM_H */
