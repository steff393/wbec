// Copyright (c) 2021 steff393, MIT license

#ifndef POWERFOX_H
#define POWERFOX_H

extern void powerfox_setup();
extern void powerfox_loop();
extern int32_t pf_getWatt();
extern uint8_t pf_getMode();
extern void    pf_setMode(uint8_t val);

enum pvMode_enum {PV_OFF, PV_ACTIVE, PV_MIN_PV};

#endif /* POWERFOX_H */
