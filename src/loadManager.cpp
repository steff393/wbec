#include <Arduino.h>
#include "globalConfig.h"
#include "loadManager.h"
#include "mbComm.h"

uint8_t currTot = 180;
uint8_t currLim[16];


uint8_t lm_getWbLimit(uint8_t id) {
	return currLim[id];
}

bool hasRequest(uint8_t id) {
	// check charging state
	return (content[id][1] == 6 || content[id][1] == 7);		// C1 or C2
}

void lm_updateWbLimits() {
	uint8_t cnt = 0;
	uint8_t lim = 0;

	// count boxes with load request
	for (int i = 0; i <= cfgCntWb ; i++) {
		if (hasRequest(i)) {
			cnt++;
		}
		// avoid limits < 6A
		if (cnt > (currTot / 60)) {
			cnt--;
			break;
		}
	}
	// split the available current 
	if (cnt != 0) {
		lim = currTot / cnt;
	}
	// assign limit to boxes
	for (int i = 0; i <= cfgCntWb ; i++) {
		if (hasRequest(i)) {
			// provide requested current, but limit to max. value
			if (lim <= content[i][15]) {
				currLim[i] = lim;
			} else {
				currLim[i] = content[i][15];
			}
			cnt--;
		} else {
			currLim[i] = 0;
		}
		if (cnt == 0) {
			// limit reached, bad luck for remaining boxes...
			break;
		}
	}
}
