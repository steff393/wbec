// Copyright (c) 2021 steff393, MIT license

#include <algorithm>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <globalConfig.h>
#include <LittleFS.h>
#include <logger.h>
#include <loadManager.h>
#include <mbComm.h>
#include <pvAlgo.h>
#include <RTCVars.h>


const uint8_t m = 11;

#define WATT_MIN        -100000		// 100kW Feed-in
#define WATT_MAX         100000		// 100kW Consumption
#define BOXID                 0		// only 1 box supported

RTCVars rtc;                               // used to memorize a few global variables over reset (not for cold boot / power on reset)

static uint32_t  lastCall             = 0;
static uint32_t  lastActivation       = 0;  // timestamp of the recent switch-on (#71), to avoid to frequent on/off
static int32_t   watt                 = 0;  // power from powerfox API (neg. = 'Einspeisung', pos. = 'Bezug')
static int32_t   availPowerPrev       = 0;  // availPower from previous cycle
static pvMode_t  pvMode               = PV_OFF;


void pvAlgo() {
	int32_t availPower = 0;

	uint16_t targetCurr = 0;
	uint8_t actualCurr = mb_amperageLimit(BOXID);

	if (mb_isCarConnected(BOXID)) {

		// available power for charging is 'Einspeisung + akt. Ladeleistung' = -watt + mb_power(BOXID)
		// negative 'watt' means 'Einspeisung'
		availPower = (int16_t)(mb_power(BOXID) - watt - cfgPvOffset);
		
		// Simple filter (average of this and previous value)
		availPower = (availPowerPrev + availPower) / 2;
		availPowerPrev = availPower;
		
		// Calculate the new target current
		if (availPower > 0 && cfgPvPhFactor != 0) {
			targetCurr = (uint16_t) (availPower / (int32_t) cfgPvPhFactor); 
		}
		LOG(m, "Target current: %.1fA", (float)targetCurr/10.0)
		// Hysteresis
		if ((actualCurr == 0 && targetCurr < cfgPvLimStart) ||
				(actualCurr != 0 && targetCurr < cfgPvLimStop)) {
			targetCurr = 0;

			// MIN+PV, don't switch off, but ...
			if ((pvMode == PV_MIN_PV) ||
			    (cfgPvMinTime != 0 && lastActivation != 0 && (millis() - lastActivation < ((uint32_t)cfgPvMinTime) * 60 * 1000))) {   // also if MinTime not elapsed (#71)
				targetCurr = mb_amperageMinimum(BOXID); // ... set minimal current configured in box
			}
		}

		// Saturation to 0 or 6..16A
		targetCurr = std::clamp(targetCurr, (uint16_t)CURR_ABS_MIN, (uint16_t)CURR_ABS_MAX);

		if (actualCurr == 0 && targetCurr >= CURR_ABS_MIN) {
			// switch on => remember timestamp for cfgPvMinTime (#71)
			lastActivation = millis();
		}
	} else {
		// no car connected
		targetCurr = 0;
		availPowerPrev = 0;
	}
	Serial.print("Watt="); Serial.print(watt); Serial.print(", availPower="); Serial.print(availPower); Serial.print(", targetCurr="); Serial.println(targetCurr);


	FSInfo fs_info;   
	LittleFS.info(fs_info);
	uint32_t time = log_unixTime();
	if ((time < 2085000000UL) &&                                // 26.01.2036 --> sometimes there are large values (e.g. 2085985724) which are wrong -> ignore them
			(fs_info.totalBytes - fs_info.usedBytes > 512000)) {    // 500kB should remain free
		File logFile = LittleFS.open(F("/pv.txt"), "a"); // Write the time and the temperature to the csv file
		logFile.print(time);
		logFile.print(";");
		logFile.print(watt);
		logFile.print(";");
		logFile.print(mb_power(BOXID));
		logFile.print(";");
		logFile.print(actualCurr);
		logFile.print(";");
		logFile.println(targetCurr);
		logFile.close();
	}

	if ((targetCurr != actualCurr)) {														// update the value not too often 
		lm_storeRequest(BOXID, targetCurr);
	}
}


void pv_setup() {
	// check config values
	if (cfgPvActive == 0) {
		pvMode = PV_DISABLED;
	} else {
		rtc.registerVar((char *)&pvMode);
		rtc.registerVar(&availPowerPrev);
		rtc.loadFromRTC();             // we load the values from rtc memory back into the registered variables
	}
}


void pv_loop() {
	if ((millis() - lastCall < (uint16_t)cfgPvCycleTime * 1000)  ||      // avoid unnecessary frequent calls
			(pvMode == PV_DISABLED)) {
		return;
	}
	lastCall = millis();

	// Call algo
	if (pvMode > PV_OFF) {  // PV algo active 
		pvAlgo();
	} else {
		availPowerPrev = 0;
	}
	rtc.saveToRTC();   // memorize over reset
}


int32_t pv_getWatt() {
	return(watt);
}


void pv_setWatt(int32_t val) {
	if ((val >= WATT_MIN) && (val <= WATT_MAX)) {
		if (cfgPvInvert) {
			watt = -val;  // possibility to invert the value (#61)
		} else {
			watt = val;
		}
	}
}


pvMode_t pv_getMode() {
	return(pvMode);
}


void pv_setMode(pvMode_t val) {
	pvMode = val;
	rtc.saveToRTC();     // memorize over reset
	lastCall = 0;  // make sure to call pv_Algo() in the next pv_loop() call
}
