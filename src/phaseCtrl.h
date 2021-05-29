// Copyright (c) 2021 steff393, MIT license

#ifndef PHASECTRL_H
#define PHASECTRL_H

extern void    pc_handle();
extern void    pc_requestPhase(uint8_t val);
extern uint8_t pc_getState();
extern boolean pc_switchInProgress();
extern void    pc_backupRequest(uint16_t val);

#endif /* PHASECTRL_H */
