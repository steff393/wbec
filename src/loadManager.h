#ifndef LOADMANAGER_H
#define LOADMANAGER_H

extern void 		lm_setWbState(uint8_t id, uint8_t _chgStat, uint8_t _currMin, uint8_t _currMax);
extern uint8_t 	lm_getWbLimit(uint8_t id);
extern void 		lm_updateWbLimits();

#endif /* LOADMANAGER_H */
