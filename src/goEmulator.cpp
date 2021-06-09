// Copyright (c) 2021 steff393

#include <Arduino.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "logger.h"
#include "mbComm.h"

#define CYCLE_TIME		   1000		// 1s

const uint8_t m = 4;

typedef struct goE_struct {
	uint32_t energyI         	= 0;
	uint16_t chgStat_old     	= 0;
	uint16_t dwo 							= 0;
	uint8_t  amp 							= 0;		// unit 0.1A (like reg. 261 of Energy Control)
	uint8_t  alw 							= 0;
} goE_t;

goE_t    box[WB_CNT];
uint32_t goE_lastCall    = 0;



boolean goE_plugged(uint16_t chgStat) {
	if (chgStat >= 4 && chgStat <= 7) {
		return(true);
	} else {
		return(false);
	}
}


void goE_handle() {
	if (millis() - goE_lastCall < CYCLE_TIME) {
		// avoid unnecessary frequent calls
		return;
	}

	for (uint8_t id = 0; id < cfgCntWb; id++) {
		if ((!goE_plugged(box[id].chgStat_old) && goE_plugged(content[id][1]))  || (box[id].energyI == 0)) {
			// vehicle plugged --> store energy count  (and also, when energyI == 0 because this indicates that there was no update after init)
			box[id].energyI = (uint32_t) content[id][13] << 16 | (uint32_t)content[id][14];
		}
		box[id].chgStat_old = content[id][1];

		// update alw & amp based on value from wallbox
		if (content[id][53] == 0) {
			box[id].alw = 0;
		} else {
			box[id].alw = 1;
			box[id].amp = content[id][53];
		}
		
		// implement the auto-switch-off function
		if (box[id].dwo != 0) {
			if (box[id].dwo * 100 < (((uint32_t) content[id][13] << 16 | (uint32_t)content[id][14]) - box[id].energyI)) {
				// Defined energy for this load cycle was reached => stop loading
				box[id].alw = 0;
				box[id].dwo = 0;
				mb_writeReg(id, REG_CURR_LIMIT, 0);
			}
		}
	}
}


void goE_setPayload(String payload, uint8_t id) {
	String cmd;
	uint16_t val = 0;
	cmd = payload.substring(0,3);					// first 4 chars, e.g. "amx="
	val = payload.substring(4).toInt();		// everything after "="
	if (cmd == "alw") {
		if (val == 1) {
			// charging allowed
			box[id].alw = 1;
			mb_writeReg(id, REG_CURR_LIMIT, box[id].amp);
		} 
		if (val == 0) {
			// charging  not allowed
			box[id].alw = 0;
			mb_writeReg(id, REG_CURR_LIMIT, 0);
		}
	}
	if (cmd == "amp" || cmd == "amx") {
		// go-e has 1A resolution, wbec has 0.1A resulotion
		val = val * 10;
		// set current
		if (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX) {	// values are between 6..32A according to API, 0 is not allowed
			box[id].amp = val;
			mb_writeReg(id, REG_CURR_LIMIT, box[id].amp);
		}
	}
	if (cmd == "dwo") {
		if (val <= 0xFFFFu) {
			box[id].dwo = val;
		}
	}
}

String goE_getStatus(uint8_t id, boolean fromApp) {
	DynamicJsonDocument data(1024); 

	if (fromApp) {
		data["oem"]="wbec";
		data["typ"]="Heidelberg Energy Control";
	}
	data["version"] = "B";
	switch(content[id][1]) {
		case  2:  data["car"] = "1"; data["err"] =  "0"; break;
		case  3:  data["car"] = "1"; data["err"] =  "0"; break;
		case  4:  data["car"] = "4"; data["err"] =  "0"; break;
		case  5:  data["car"] = "4"; data["err"] =  "0"; break;
		case  6:  data["car"] = "2"; data["err"] =  "0"; break;
		case  7:  data["car"] = "2"; data["err"] =  "0"; break;
		case  8:  data["car"] = "2"; data["err"] =  "0"; break;
		case  9:  data["car"] = "2"; data["err"] = "10"; break;		// not sure, if 9 is really an error...
		case 10:  data["car"] = "1"; data["err"] =  "0"; break;		// e.g. when remote locked the status will be set to F = 10 -> not clear if a vehicle is connected?!
		default:  data["car"] = "1"; data["err"] = "10"; break; 
	}
	data["alw"] = String(box[id].alw);
	data["amp"] = String(box[id].amp / 10);
	data["amx"] = String(box[id].amp / 10);
	data["stp"] = "0";
	uint8_t pha = 0;
	if (content[id][6] > 200) { pha+=9; } 	// 0000 1001
	if (content[id][7] > 200) { pha+=18; } // 0001 0010
	if (content[id][8] > 200) { pha+=36; } // 0010 0100
	data["pha"] = String(pha);
	data["tmp"] = String(content[id][5]);
	data["dws"] = String((((uint32_t) content[id][13] << 16 | (uint32_t)content[id][14]) - box[id].energyI) * 360);
	data["dwo"] = String(box[id].dwo);
	data["uby"] = "0";
	data["eto"] = String(((uint32_t) content[id][13] << 16 | (uint32_t)content[id][14]) / 100);
	data["nrg"][0] = content[id][6]; // L1
	data["nrg"][1] = content[id][7]; // L2
	data["nrg"][2] = content[id][8]; // L3
	data["nrg"][3] = 0;
	data["nrg"][4] = content[id][2]; // L1
	data["nrg"][5] = content[id][3]; // L2
	data["nrg"][6] = content[id][4]; // L3
	data["nrg"][7] = 0;
	data["nrg"][8] = 0;
	data["nrg"][9] = 0;
	data["nrg"][10] = 0;
	data["nrg"][11] = content[id][10] / 10;
	data["nrg"][12] = 0;
	data["nrg"][13] = 0;
	data["nrg"][14] = 0;
	data["nrg"][15] = 0;
	data["fwv"] = "40";
	data["sse"] = mb_getAscii(id, 27,3);
	data["ama"] = String(content[id][15]);
	data["ust"] = "2";

	String response;
	serializeJson(data, response);
	log(m, response);
	return(response);
}