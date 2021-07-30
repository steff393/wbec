// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ArduinoJson.h>
#include "globalConfig.h"
#include <LittleFS.h>
#include "logger.h"
#include "mbComm.h"
#include <rtcvars.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>


const uint8_t m = 9;

#define CYCLE_TIME		    30000		// 30 s	=> 2880 calls per day (fair-use limit: 8000 per day)
#define UPDATE_TIME       60000   // 60s
#define MAX_API_LEN		 	    150		// Max accepted length of API response
#define MIN_HEAP_NEEDED   15000		// This heap is minimum necessary, otherwise ESP might crash during HTTPS / TLS connection
#define OUTDATED         600000		// 10 min, after this time the value is considered outdated
#define WATT_MIN        -100000		// 100kW Feed-in
#define WATT_MAX         100000		// 100kW Consumption
#define BOXID                 0		// only 1 box supported
#define CURR_START_MIN       61   // 6,1A
#define CURR_STOP_MIN        50   // 5,0A (not possible, but will be kept at 6A)
#define CURRENT_POWER_FACTOR 69		// 1A equals 690 W @ 3phases  => 0,1A equals 69W       (@ 1phase: 23)

RTCVars rtc;                               // used to memorize a few global variables over reset (not for cold boot / power on reset)
static uint32_t lastHandleCall       = 0;
static int32_t  watt                 = 0;  // power from powerfox API (neg. = 'Einspeisung', pos. = 'Bezug')
static int32_t  availPowerPrev       = 0;  // availPower from previous cycle
static uint32_t lastUpdate           = 0;  // last update via Modbus to box
static uint8_t  enabled              = 0;

HTTPClient                *http;
BearSSL::WiFiClientSecure *httpsClient;


void pfoxAlgo() {
	int32_t availPower = 0;

	uint8_t targetCurr = 0;
	uint8_t actualCurr = content[BOXID][53];

	if (true /*content[BOXID][1] >= 4 && content[BOXID][1] <= 7*/) {   // Car is connected

		// available power for charging is 'Einspeisung + akt. Ladeleistung' = -watt + content[0][10]
		// negative 'watt' means 'Einspeisung'
		availPower = (int16_t)(content[BOXID][10] - watt);
		
		// Simple filter (average of this and previous value)
		availPower = (availPowerPrev + availPower) / 2;
		availPowerPrev = availPower;
		
		// Calculate the new target current
		if (availPower > 0) {
			targetCurr = availPower / CURRENT_POWER_FACTOR;
		}
		LOG(m, "Target current: %.1fA", (float)targetCurr/10.0)
		// Hysteresis
		if ((actualCurr == 0 && targetCurr < CURR_START_MIN) ||
				(actualCurr != 0 && targetCurr < CURR_STOP_MIN)) {
			targetCurr = 0;
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

	if ((targetCurr != actualCurr) /*&& (millis() - lastUpdate > UPDATE_TIME)*/) {	// update the value not too often 
		mb_writeReg(BOXID, REG_CURR_LIMIT, targetCurr);
		//lastUpdate = millis();
	}
}


void powerfox_setup() {
	rtc.registerVar(&enabled);
	rtc.registerVar(&availPowerPrev);
	rtc.loadFromRTC();             // we load the values from rtc memory back into the registered variables
}


void powerfox_loop() {
	if ((millis() - lastHandleCall < CYCLE_TIME)  || 								                        // avoid unnecessary frequent calls
			!strcmp(cfgFoxUser, "") || !strcmp(cfgFoxPass, "") || !strcmp(cfgFoxDevId, "") ||		// look for credentials (all need a value)
			(cfgCntWb > 1)) {		// more wallboxes need too much heap, e.g. for web server
		return;
	}
	lastHandleCall = millis();

	Serial.print(F("Heap before new: ")); Serial.println(ESP.getFreeHeap());
	if (ESP.getFreeHeap() < MIN_HEAP_NEEDED) {
		LOG(m, "Heap is low!", "")
	}
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

	// Call algo
	if (enabled) {  // PowerFox Control active 
		pfoxAlgo();
	} else {
		availPowerPrev = 0;
	}
	rtc.saveToRTC();   // memorize over reset
}

int32_t pf_getWatt() {
	return(watt);
}

boolean pf_getEnabled() {
	return(enabled);
}

void pf_setEnabled(boolean act) {
	enabled = act;
	rtc.saveToRTC();   // memorize over reset
}