// Copyright (c) 2021 steff393, MIT license

#ifndef PVALGO_H
#define PVALGO_H

#include <Arduino.h>


typedef enum {
	PV_DISABLED   = 0, 
	PV_OFF        = 1, 
	PV_ACTIVE     = 2, 
	PV_MIN_PV     = 3
} pvMode_t;


extern void     pv_setup();
extern void     pv_loop();
extern int32_t  pv_getWatt();
extern void     pv_setWatt(int32_t val);
extern pvMode_t pv_getMode();
extern void     pv_setMode(pvMode_t val);
extern uint8_t  pv_getWbId();
extern void     pv_setWbId(uint8_t val);

#endif /* PVALGO_H */
