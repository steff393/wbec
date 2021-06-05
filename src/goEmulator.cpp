// Copyright (c) 2021 steff393

#include <Arduino.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "logger.h"
#include "mbComm.h"

#define CYCLE_TIME		   1000		// 1s

const uint8_t m = 4;

uint32_t energyI         = 0;
uint16_t chgStat_old     = 0;
uint32_t goE_lastCall    = 0;
uint8_t  alw_old = 1;
uint8_t  alw = 1;
uint8_t  amp = 0;
uint16_t dwo = 0;
uint8_t  pha = 0;
uint8_t  i   = 0;


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

	if (!goE_plugged(chgStat_old) && goE_plugged(content[i][1])) {
		// vehicle plugged --> store energy count
		energyI = (uint32_t) content[i][13] << 16 | (uint32_t)content[i][14];
	}
	chgStat_old = content[i][1];
	
	// update the amp value
	amp = content[i][53] / 10;
	
	// set the pha value
	pha = 0;
	if (content[i][6] > 200) { pha+=9; } 	// 0000 1001
	if (content[i][7] > 200) { pha+=18; } // 0001 0010
	if (content[i][8] > 200) { pha+=36; } // 0010 0100
	
	// implement the auto-switch-off function
	if (dwo != 0) {
		if (dwo * 100 < (((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) - energyI)) {
			// Defined energy for this load cycle was reached => stop loading
			alw = 0;
		}
	}

	// decide whether box shall be locked or not
	if (alw == 0 && alw_old == 1) {
		// lock the wallbox
		mb_writeReg(i, REG_REMOTE_LOCK, 0);
	}
	if (alw == 1 && alw_old == 0) {
		// unlock the wallbox
		mb_writeReg(i, REG_REMOTE_LOCK, 1);
	}
	alw_old = alw;
}


void goE_setPayload(String payload) {
	String cmd;
	uint16_t val = 0;
	cmd = payload.substring(0,3);					// first 4 chars, e.g. "amx="
	val = payload.substring(4).toInt();		// everything after "="
	if (cmd == "alw") {
		if (val == 1) {
			alw = 1;	// charging allowed
		} 
		if (val == 0) {
			alw = 0;	// charging  not allowed
		}
	}
	if (cmd == "amp" || cmd == "amx") {
		// go-e has 1A resolution, wbec has 0.1A resulotion
		val = val * 10;
		// set current
		if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
			mb_writeReg(i, REG_CURR_LIMIT, val);
			amp = val / 10;
		}
	}
	if (cmd == "dwo") {
		if (val <= 0xFFFFu) {
			dwo = val;
		}
	}
}

String goE_getStatus() {
	DynamicJsonDocument data(1024); 
	uint8_t i = 0;

	data["version"] = "B";
	switch(content[i][1]) {
		case  2:  data["car"] = "1"; data["err"] =  "0"; break;
		case  3:  data["car"] = "1"; data["err"] =  "0"; break;
		case  4:  data["car"] = "3"; data["err"] =  "0"; break;
		case  5:  data["car"] = "3"; data["err"] =  "0"; break;
		case  6:  data["car"] = "3"; data["err"] =  "0"; break;   // alternatively 4
		case  7:  data["car"] = "2"; data["err"] =  "0"; break;
		case  8:  data["car"] = "4"; data["err"] =  "0"; break;
		case  9:  data["car"] = "2"; data["err"] = "10"; break;		// not sure, if 9 is really an error...
		case 10:  data["car"] = "1"; data["err"] =  "0"; break;		// e.g. when remote locked the status will be set to F = 10 -> not clear if a vehicle is connected?!
		default:  data["car"] = "0"; data["err"] = "10"; break; 
	}
	data["alw"] = String(alw);
	data["amp"] = String(amp);
	data["stp"] = "0";
	data["pha"] = String(pha);
	data["tmp"] = String(content[i][5]);
	data["dws"] = String((((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) - energyI) * 360);
	data["dwo"] = String(dwo);
	data["uby"] = "0";
	data["eto"] = String(((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 100);
	data["nrg"][0] = content[i][6]; // L1
	data["nrg"][1] = content[i][7]; // L2
	data["nrg"][2] = content[i][8]; // L3
	data["nrg"][3] = 0;
	data["nrg"][4] = content[i][2]; // L1
	data["nrg"][5] = content[i][3]; // L2
	data["nrg"][6] = content[i][4]; // L3
	data["nrg"][7] = 0;
	data["nrg"][8] = 0;
	data["nrg"][9] = 0;
	data["nrg"][10] = 0;
	data["nrg"][11] = content[i][10] / 10;
	data["nrg"][12] = 0;
	data["nrg"][13] = 0;
	data["nrg"][14] = 0;
	data["nrg"][15] = 0;
	data["fwv"] = "40";
	data["sse"] = "000000";
	data["ama"] = String(content[i][15]);
	data["ust"] = "2";

	String response;
	serializeJson(data, response);
	log(m, response);
	return(response);
}