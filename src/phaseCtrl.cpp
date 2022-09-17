// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <logger.h>
#include <loadManager.h>
#include <mbComm.h>

const uint8_t m  = 6;
const uint8_t id = 0;

#define LIMIT_230V       200
#define LIMIT_0V   	      15
#define LIMIT_0A           0
#define CYCLE_TIME		   500		// 500 ms
#define VOLT_DEB_TIME  60000		// wait 60s until voltage is considered stable
#define AMPS_DEB_TIME  60000		// wait 60s until current is considered stable
#define MAX_WAIT_TIME 300000		// go back to INIT latest after 5 min in WAIT_0AMP --> abort switching


enum pcState_enum {INIT, NORMAL_1P, NORMAL_3P, WAIT_0AMP};

uint8_t  pcState              = INIT;
uint8_t  pcRequest            = 0;
uint16_t currentBackup        = 0;
uint32_t lastHandleCall       = 0;
uint32_t timerCheck1p         = 0;
uint32_t timerCheck3p         = 0;
uint32_t timerWait0Amp        = 0;
uint32_t timerWait0Amp_Entry  = 0;


uint8_t pc_checkVoltages() {
	uint32_t now = millis();
	if (content[id][6] > LIMIT_230V && 
			content[id][7] > LIMIT_230V &&
			content[id][8] > LIMIT_230V &&
			modbusResultCode[id] == 0) {
		if (now - timerCheck3p > VOLT_DEB_TIME) {
			return(3);
		}
	} else { timerCheck3p = now; }

	if (content[id][6] > LIMIT_230V &&
			content[id][7] < LIMIT_0V   &&
			content[id][8] < LIMIT_0V   &&
			modbusResultCode[id] == 0) {
		if (now - timerCheck1p > VOLT_DEB_TIME) {
			return(1);
		}
	} else { timerCheck1p = now; }
	return(0);
}


bool pc_check0Amp() {
	uint32_t now = millis();
	if (content[id][2] <= LIMIT_0A && 
			content[id][3] <= LIMIT_0A &&
			content[id][4] <= LIMIT_0A &&
			modbusResultCode[id] == 0) {
		if (now - timerWait0Amp > AMPS_DEB_TIME) {
			return(true);
		}
	} else { timerWait0Amp = now; }
	return(false);
}


uint8_t getRequestedPhases() {
	return(pcRequest);
	// read available power from e.g. openWB
	/* if (power > 4kW) {
		return(3);
	} else {
		return(1);
	}
	--> Add debouncing, e.g. 10 minutes ??
	*/ 
}


void httpCall(boolean state) {
	WiFiClient client;
	HTTPClient http;
	String serverPath;
	if (state) {
		serverPath = F("http://shelly-ip/relay/0?turn=on");
	} else {
		serverPath = F("http://shelly-ip/relay/0?turn=off");
	}
	log(m, F("url:") + serverPath);

	http.begin(client, serverPath);
	int16_t httpResponseCode = http.GET();
	if (httpResponseCode > 0) {
		log(m, F("HTTP Response code: ") + String(httpResponseCode) + ", " + http.getString());
	}
	else {
		log(m, F("Error code: ") + String(httpResponseCode));
	}
	http.end();
}


void trans_INIT() {
	log(m, F("--> INIT"));
	pcState = INIT;
	timerCheck1p = millis();
	timerCheck3p = millis();
}


void trans_NORMAL_1P() {
	log(m, F("--> NORMAL_1P"));
	pcState = NORMAL_1P;
	if (currentBackup > 0) {
		lm_storeRequest(id, currentBackup);
	}
}


void trans_NORMAL_3P() {
	log(m, F("--> NORMAL_3P"));
	pcState = NORMAL_3P;
	if (currentBackup > 0) {
		lm_storeRequest(id, currentBackup);
	}
}


void trans_WAIT_0AMP() {
	log(m, F("--> WAIT_0AMP"));
	pcState = WAIT_0AMP;
	timerWait0Amp       = millis();
	timerWait0Amp_Entry = millis();
}


void pc_handle() {
	if ((millis() - lastHandleCall < CYCLE_TIME) || (cfgCntWb > 1)) {
		// avoid unnecessary frequent calls and block the feature, when more than 1 wallbox is connected, due to timing reasons
		return;
	}
	lastHandleCall = millis();
	switch (pcState) {
		case INIT: 
			// Check how many phases are active
			switch (pc_checkVoltages()) {
				case 1: 
				  trans_NORMAL_1P();
					break;
				case 3: 
				  trans_NORMAL_3P();
					break;
				default: ; // remain
			}
			break;
		case NORMAL_1P:
		  if (getRequestedPhases() == 3) {
				lm_storeRequest(0, 0);
				trans_WAIT_0AMP();
			}
			break;
		case NORMAL_3P:
			if (getRequestedPhases() == 1) {
				lm_storeRequest(0, 0);
				trans_WAIT_0AMP();
			}
			break;
		case WAIT_0AMP:
			if (pc_check0Amp()) {
				if (getRequestedPhases() == 1) {
					log(m, F("Call Shelly OFF"));
					httpCall(false);
				}
				if (getRequestedPhases() == 3) {
					log(m, F("Call Shelly ON"));
					httpCall(true);
				}
				trans_INIT();
			}
			if (millis() - timerWait0Amp_Entry > MAX_WAIT_TIME) {
				// abort without switching
				trans_INIT();
			}
			break;
		default: 
			trans_INIT(); // should not happen
	}
}


void pc_requestPhase(uint8_t val) {
	if (val == 1 || val == 3) {
		pcRequest = val;
	}
}


uint8_t pc_getState() {
	return(pcState);
}

boolean pc_switchInProgress() {
	if (lastHandleCall == 0) {
		return(false);		// as long as feature is disabled, don't block current modifications!
	}
	// no modification of register 261 allowed, when phase switch is in progress
	if (pcState == INIT || pcState == WAIT_0AMP) {
		return(true);
	} else {
		return(false);
	}
}


void pc_backupRequest(uint16_t val) {
	currentBackup = val;
}
