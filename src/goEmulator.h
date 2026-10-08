// Copyright (c) 2021 steff393

#ifndef GOEMULATOR_H
#define GOEMULATOR_H

#include <ArduinoJson.h>

extern void 		goE_handle();
extern void     goE_setPayload(String payload, uint8_t id);
extern void			goE_getStatus(uint8_t id, boolean fromApp, JsonDocument &data);
extern uint32_t goE_getEnergySincePlugged(uint8_t id); 

#endif /* GOEMULATOR_H */