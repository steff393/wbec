// Copyright (c) 2022 andreas.miketta, steff393, MIT license
// based on https://github.com/AMiketta/wbec
#ifndef INVERTER_H
#define INVERTER_H

#define INVERTER_JSON_LEN       256

extern void     inverter_setup();
extern void     inverter_loop();
extern String   inverter_getStatus();
extern int16_t  inverter_getPwrInv();
extern int16_t  inverter_getPwrMet();

#endif /* INVERTER_H */
