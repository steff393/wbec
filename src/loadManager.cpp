// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <globalConfig.h>
#include <loadManager.h>
#include <mbComm.h>

#define CYCLE_TIME		             500	// ms


static uint32_t lastCall           = 0;

static uint8_t  currLim[WB_CNT];
static uint8_t  lastReq[WB_CNT];

/*
static uint16_t sumRead            = 0;
static uint16_t sumReq             = 0;
static boolean  allBoxesReceived   = false;
*/


static uint8_t saturate1(uint8_t val, uint16_t limit) {
	if (val > limit) {
		return(limit);
	} else {
		return(val);
	}
}


static uint8_t saturate2(uint8_t val, uint16_t limit1, uint16_t limit2) {
	uint16_t limit;
	if (limit1 < limit2) {
		limit = limit1;
	} else {
		limit = limit2;
	}
	return(saturate1(val, limit));
}



static void lm_updateWbLimits() {
	// This is the central load management function.
	// It shall calculate the currLim[] value for every wallbox
	//
	// Input:
	// - mb_chargingRequested(id)      Car connected with charging request
	// - lastReq[id]                   Last requested current limit from any of the 'applications' on higher level
	// - mb_amperageMaximum(id) Maximum current configured in box (switch S1)
	// - mb_amperageLimit(id) Current limit which is wallbox
	// Output:
	// - currLim[id]              Current limit which shall be in the wallbox


	// Simple stupid method, only for 2 wallboxes with 50%/50%
	uint16_t requestMap = mb_chargingRequested(0) + mb_chargingRequested(1);

	switch(requestMap) {
		case 0: {   // no charging request
			currLim[0] = 0;
			currLim[1] = 0;
			break;
		}
		case 1: {   // charging request only on box 0
			currLim[0] = saturate2(lastReq[0], mb_amperageMaximum(0), cfgTotalCurrMax);
			currLim[1] = 0;
			break;
		}
		case 2: {   // charging request only on box 1
			currLim[0] = 0;
			currLim[1] = saturate2(lastReq[1], mb_amperageMaximum(1), cfgTotalCurrMax);
			break;
		}
		case 3: {   // charging request on both boxes 
			currLim[0] = saturate2(lastReq[0], mb_amperageMaximum(0), cfgTotalCurrMax / 2);
			currLim[1] = saturate2(lastReq[1], mb_amperageMaximum(1), cfgTotalCurrMax / 2);
			break;
		}
		default: { ; }  // shouldn't happen
	}

	/*
	// Complex, flexible methods

	uint16_t cnt = 0;
	uint16_t limit = 0;
	uint16_t remaining = cfgTotalCurrMax;

	sumRead = 0;
	sumReq  = 0;

	// Method A
	// -----------------------------------------------------------------

	// count boxes with charge request
	for (uint8_t id = 0; id < cfgCntWb ; id++) {
		if (mb_chargingRequested(id)) {
			cnt++;
		}
	}

	// calculate 'fair' limit (=allowed current / nr. of boxes)
	if (cnt != 0) {
		limit = remaining / cnt;
	}

	// every box with charge request and request < 'fair' limit gets its request
	for (uint8_t id = 0; id < cfgCntWb ; id++) {
		if (mb_chargingRequested(id) && lastReq[id] <= limit) {
			currLim[id] = saturate2(lastReq[id], remaining, mb_amperageMaximum(id));
			remaining -= currLim[id]; // can't become negative, as currLim is always <= remaining
			cnt--;
		}
	}

	// calculate 'fair' limit (=allowed current / nr. of boxes)
	if (cnt != 0) {
		limit = remaining / cnt;
	}

	// every box with charge request and request > 'fair' limit gets its request
	for (uint8_t id = 0; id < cfgCntWb ; id++) {
		if (mb_chargingRequested(id)) {
			currLim[id] = saturate2(lastReq[id], limit, mb_amperageMaximum(id));
			remaining -= currLim[id]; // can't become negative, as currLim is always <= remaining
		}
	}

	// TODO...



	// Method B
	// -----------------------------------------------------------------

	// count boxes with load request
	for (uint8_t id = 0; id < cfgCntWb ; id++) {
		if (mb_chargingRequested(id)) {
			cnt++;
		}
		// avoid limits < 6A
		if (cnt > (cfgTotalCurrMax / CURR_ABS_MIN)) {
			cnt--;
			break;
		}
		sumRead += mb_amperageLimit(id);
		sumReq  += lastReq[id];
	}

	if (sumReq <= cfgTotalCurrMax) {
		// no limitation needed, every box gets its request
		for (uint8_t id = 0; id < cfgCntWb ; id++) {
			currLim[id] = lastReq[id];
		}
	} else {
		// more requests than allowed, splitting is necessary:
		// prio 1: Boxes with a car that wants charging
		// prio 2: all other boxes
		for (uint8_t id = 0; id < cfgCntWb ; id++) {
			// ...
			// TODO
			// ...
		}
	}
	// TODO...
	*/
}


void lm_setup() {
	for (uint8_t id = 0; id < WB_CNT ; id++) {
		currLim[id] = 255;     // 255 is the marker, that no valid value received yet
		lastReq[id] = 0;
	}
	if (cfgTotalCurrMax != 0) {
		cfgStandby = 4;        // disable standby when using load management
	}
}


void lm_loop() {
	if ((millis() - lastCall < CYCLE_TIME) || (cfgTotalCurrMax == 0) || (cfgCntWb != 2)) {
		// avoid unnecessary frequent calls
		return;
	}
	lastCall = millis();

	/*
	// load management only starts when all boxes have been received once
	if (allBoxesReceived == false) {
		for (uint8_t id = 0; id < cfgCntWb ; id++) {
			if (currLim[id] == 255) { return;	} 
		}
		allBoxesReceived = true;
	}
	*/

	lm_updateWbLimits();
	
	for (uint8_t id = 0; id < cfgCntWb ; id++) {
		if (mb_amperageLimit(id) != currLim[id]) {
			// when the value from box differs to wanted value then write current via modbus
			mb_writeReg(id, REG_CURR_LIMIT, currLim[id]);
		}
	}
}


uint8_t lm_getWbLimit(uint8_t id) {
	return currLim[id];
}


uint8_t lm_getLastRequest(uint8_t id) {
	return lastReq[id];
}


void lm_storeRequest(uint8_t id, uint8_t val) {
	lastReq[id] = val;
	// when there is still buffer OR request is lowered OR load management inactive
	if (/*(sumRead + val < cfgTotalCurrMax) ||*/ (val < mb_amperageLimit(id)) || (cfgTotalCurrMax == 0)) {
		// direct write is possible
		mb_writeReg(id, REG_CURR_LIMIT, val);
	}
}


void lm_currentReadSuccess(uint8_t id) {
	// takeover the value from wallbox as long as not all boxes are received
	//if (allBoxesReceived == false) {
	//	currLim[id] = mb_amperageLimit(id);
	//} // else do nothing
}
