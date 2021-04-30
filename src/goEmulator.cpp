// Copyright (c) 2021 steff393

#include <Arduino.h>

#include "mbComm.h"

uint32_t energyI = 0;
uint8_t  chgStat_old = 0;
uint8_t  i = 0;

void goE_handle() {
	if (chgStat_old != 7 && content[i][1] == 7)
		// charging started --> store energy count
		energyI = (uint32_t) content[i][13] << 16 | (uint32_t)content[i][14];
}


uint32_t goE_getDws() {
	return(((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) - energyI);
}
