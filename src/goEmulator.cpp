// Copyright (c) 2021 steff393

#include <Arduino.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "mbComm.h"

uint32_t energyI = 0;
uint8_t  chgStat_old = 0;
uint8_t  alw = 0;
uint8_t  amp = 0;
uint8_t  i = 0;


void goE_handle() {
	if (chgStat_old != 7 && content[i][1] == 7) {
		// charging started --> store energy count
		energyI = (uint32_t) content[i][13] << 16 | (uint32_t)content[i][14];
	}
	// update the amp value
	amp = content[i][53] / 10;
}


void goE_setPayload(String payload) {
	String cmd;
	uint16_t val = 0;
	cmd = payload.substring(0,3);					// first 4 chars, e.g. "amx="
	val = payload.substring(4).toInt();		// everything after "="
	if (cmd == "alw") {
		if (val == 1) {
			// charging allowed
			alw = 1;
		} else {
			// reduce current to 0A
			mb_writeReg(i, REG_CURR_LIMIT, 0);
			amp = 0;
			alw = 0;
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
}

String goE_getStatus() {
	DynamicJsonDocument data(1024); 
	uint8_t i = 0;

	switch(content[i][1]) {
		case 2:  data["car"] = "1"; break;
		case 3:  data["car"] = "1"; break;
		case 4:  data["car"] = "3"; break;
		case 5:  data["car"] = "3"; break;
		case 6:  data["car"] = "3"; break;   // alternatively 4
		case 7:  data["car"] = "2"; break;
		default: data["car"] = "0"; data["err"] = "10"; break; 
	}
	data["alw"] = String(alw);
	data["amp"] = String(amp);
	data["err"] = "0";
	data["stp"] = "0";
	data["tmp"] = String(content[i][5]);
	data["dws"] = String(((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) - energyI);
	data["ubi"] = "0";
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

	String response;
	serializeJson(data, response);
	Serial.println(response);
	return(response);
}