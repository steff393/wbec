// Copyright (c) 2022 andreas.miketta, steff393, MIT license
// based on https://github.com/AMiketta/wbec
#ifndef SOLAREDGE_H
#define SOLAREDGE_H

#define INVERTER_JSON_LEN       256

extern void     inverter_setup();
extern void     inverter_loop();
extern String   inverter_getStatus();

#endif /* SOLAREDGE_H */
