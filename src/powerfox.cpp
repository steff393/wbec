// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ArduinoJson.h>
#include <globalConfig.h>
#include <LittleFS.h>
#include <logger.h>
#include <loadManager.h>
#include <mbComm.h>
#include <powerfox.h>
#include <rtcvars.h>
#include <umm_malloc/umm_heap_select.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>


const uint8_t m = 9;

#define MAX_API_LEN		 	    150		// Max accepted length of API response
#define OUTDATED         600000		// 10 min, after this time the value is considered outdated
#define WATT_MIN        -100000		// 100kW Feed-in
#define WATT_MAX         100000		// 100kW Consumption
#define BOXID                 0		// only 1 box supported

RTCVars rtc;                               // used to memorize a few global variables over reset (not for cold boot / power on reset)
static uint32_t lastHandleCall       = 0;
static int32_t  watt                 = 0;  // power from powerfox API (neg. = 'Einspeisung', pos. = 'Bezug')
static int32_t  availPowerPrev       = 0;  // availPower from previous cycle
static uint8_t  pvMode               = PV_OFF;

HTTPClient                *http;
BearSSL::WiFiClientSecure *httpsClient;


void pfoxAlgo() {
	int32_t availPower = 0;

	uint8_t targetCurr = 0;
	uint8_t actualCurr = content[BOXID][53];

	if (content[BOXID][1] >= 4 && content[BOXID][1] <= 7) {   // Car is connected

		// available power for charging is 'Einspeisung + akt. Ladeleistung' = -watt + content[0][10]
		// negative 'watt' means 'Einspeisung'
		availPower = (int16_t)(content[BOXID][10] - watt - cfgPvOffset);
		
		// Simple filter (average of this and previous value)
		availPower = (availPowerPrev + availPower) / 2;
		availPowerPrev = availPower;
		
		// Calculate the new target current
		if (availPower > 0 && cfgPvPhFactor != 0) {
			targetCurr = availPower / cfgPvPhFactor;
		}
		LOG(m, "Target current: %.1fA", (float)targetCurr/10.0)
		// Hysteresis
		if ((actualCurr == 0 && targetCurr < cfgPvLimStart) ||
				(actualCurr != 0 && targetCurr < cfgPvLimStop)) {
			targetCurr = 0;
		}

		if (pvMode == PV_MIN_PV) {
			targetCurr = content[BOXID][16]; // set minimal current configured in box
		}

		// Saturation to 0 or 6..16A
		if (targetCurr != 0) {
			if (targetCurr < CURR_ABS_MIN) {
				targetCurr = CURR_ABS_MIN;
			} else if (targetCurr > CURR_ABS_MAX) {
				targetCurr = CURR_ABS_MAX;
			} 
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
		File logFile = LittleFS.open(F("/pfox.txt"), "a"); // Write the time and the temperature to the csv file
		logFile.print(time);
		logFile.print(";");
		logFile.print(watt);
		logFile.print(";");
		logFile.print(content[BOXID][10]);
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


void powerfox_setup() {
	// check config values
	if (!strcmp(cfgFoxUser, "") || !strcmp(cfgFoxPass, "") || !strcmp(cfgFoxDevId, "") ||		// look for credentials (all need a value)
			(cfgCntWb > 1)) {		// more wallboxes need too much heap, e.g. for web server
		pvMode = PV_DISABLED;
	} else {
		rtc.registerVar(&pvMode);
		rtc.registerVar(&availPowerPrev);
		rtc.loadFromRTC();             // we load the values from rtc memory back into the registered variables
	}
}


void powerfox_loop() {
	if ((millis() - lastHandleCall < (uint16_t)cfgPvCycleTime * 1000)  ||      // avoid unnecessary frequent calls
			(pvMode == PV_DISABLED)) {
		return;
	}
	lastHandleCall = millis();

	Serial.print(F("Heap before new: ")); Serial.println(ESP.getFreeHeap());
	{
		HeapSelectIram ephemeral;
		Serial.printf("IRAM free: %6d bytes\r\n", ESP.getFreeHeap());
		http = new HTTPClient();
		httpsClient = new BearSSL::WiFiClientSecure();
		
		Serial.print(F("Heap after new : ")); Serial.println(ESP.getFreeHeap());

		httpsClient->setInsecure();
		httpsClient->setBufferSizes(512,512);    // must be between 512 and 16384
		http->begin(*httpsClient, F("https://backend.powerfox.energy/api/2.0/my/") + String(cfgFoxDevId) + F("/current"));

		http->setAuthorization(cfgFoxUser, cfgFoxPass);
		http->setReuse(false);
		uint32_t tm = millis();
		http->GET();
		Serial.print(F("Duration of GET: ")); Serial.println(millis() - tm);
		
		char response[MAX_API_LEN];
		while (httpsClient->connected() || httpsClient->available()) {
			if (httpsClient->available()) {
				httpsClient->read((uint8_t*)response, MAX_API_LEN-1);
			}
		}
		Serial.println(response);
		Serial.print(F("Heap befor del : ")); Serial.println(ESP.getFreeHeap());
		delete http;
		delete httpsClient;
		Serial.print(F("Heap after del : ")); Serial.println(ESP.getFreeHeap());

		StaticJsonDocument<256> doc;
		// Parse JSON object
		DeserializationError error = deserializeJson(doc, response);
		if (error) {
			LOG(m, "deserializeJson() failed: %s", error.f_str())
			return;
		} 

		uint32_t timestamp = 0;
		timestamp =  doc[F("Timestamp")]        | 0;
		watt = (int) doc[F("Watt")].as<float>();
		LOG(m, "Timestamp=%d, Watt=%d", timestamp, watt)
		
		if ((log_unixTime() - timestamp > OUTDATED) || (watt < WATT_MIN) || (watt > WATT_MAX)) {
			Serial.print("unixtime:"); Serial.println(log_unixTime());
			watt = 0;
			availPowerPrev = 0;
		}
	}
	HeapSelectDram ephemeral;
  Serial.printf("DRAM free: %6d bytes\r\n", ESP.getFreeHeap());

	// Call algo
	if (pvMode > PV_OFF) {  // PowerFox Control active 
		pfoxAlgo();
	} else {
		availPowerPrev = 0;
	}
	rtc.saveToRTC();   // memorize over reset
}

int32_t pf_getWatt() {
	return(watt);
}

uint8_t pf_getMode() {
	return(pvMode);
}

void pf_setMode(uint8_t val) {
	pvMode = val;
	rtc.saveToRTC();   // memorize over reset
}