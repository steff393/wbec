// Copyright (c) 2021 steff393, MIT license

#ifndef POWERFOX_H
#define POWERFOX_H

extern void powerfox_setup();
extern void powerfox_loop();
extern int32_t pf_getWatt();
extern boolean pf_getEnabled();
extern void    pf_setEnabled(boolean act);

#endif /* POWERFOX_H */
