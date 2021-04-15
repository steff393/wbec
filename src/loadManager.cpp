#include <Arduino.h>
#include "loadManager.h"

uint8_t currTot = 180;
uint8_t chgStat[16];
uint8_t currMin[16];
uint8_t currMax[16];
uint8_t currLim[16];

void lm_setWbState(uint8_t id, uint8_t _chgStat, uint8_t _currMin, uint8_t _currMax) {
	chgStat[id-1] = _chgStat;
	currMin[id-1] = _currMin;
	currMax[id-1] = _currMax;
}

uint8_t lm_getWbLimit(uint8_t id) {
	return currLim[id-1];
}

bool hasRequest(uint8_t id) {
	return (chgStat[id] == 6 || chgStat[id] == 7);		// C1 or C2
}

void lm_updateWbLimits() {
	uint8_t cnt = 0;
	uint8_t lim = 0;

	// count boxes with load request
	for (int i = 0; i <= 15 ; i++) {
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
	for (int i = 0; i <= 15 ; i++) {
		if (hasRequest(i)) {
			// provide requested current, but limit to max. value
			if (lim <= currMax[i]) {
				currLim[i] = lim;
			} else {
				currLim[i] = currMax[i];
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