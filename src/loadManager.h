#ifndef LOADMANAGER_H
#define LOADMANAGER_H

extern void     lm_setup();
extern void     lm_loop();

extern uint8_t 	lm_getWbLimit(uint8_t id);
extern uint8_t 	lm_getLastRequest(uint8_t id);
extern void     lm_storeRequest(uint8_t id, uint8_t val);
extern void 		lm_currentReadSuccess(uint8_t id);

#endif /* LOADMANAGER_H */
