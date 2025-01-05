// Copyright (c) 2023 steff393
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <globalConfig.h>
#include <IPAddress.h>
#include <logger.h>
#include <pvAlgo.h>
#include <pvHttp.h>


const uint8_t m = 13;

#define MAX_API_LEN		 	    2048		// Max accepted length of API response

static IPAddress remote;   
static bool      pvHttpActive               = false;
static uint32_t  lastHandleCall             = 0;


void pvHttp_setup() {
	if (strcmp(cfgPvHttpIp, "") != 0) {
		if (remote.fromString(cfgPvHttpIp) && cfgPvHttpPath[0]=='/') {
			pvHttpActive = true;
		}
	}
}


void pvHttp_loop() {
	if ((millis() - lastHandleCall < (uint16_t)cfgPvCycleTime * 1000) ||     // avoid unnecessary frequent calls
			(pvHttpActive == false)) {
		return;
	}
	lastHandleCall = millis();

	String response;
	
	WiFiClient client;
	HTTPClient http;
	http.begin(client, cfgPvHttpIp, cfgPvHttpPort, cfgPvHttpPath);
	int16_t httpResponseCode = http.GET();
	if (httpResponseCode > 0) {
		response = http.getString();
		LOG(m, "HTTP Response code: %d, %s", httpResponseCode, response);
	}	else {
		LOG(m, "Error code: %d", httpResponseCode);
	}
	http.end();

	int32_t watt = 0;

	if (strcmp(cfgPvHttpJson, "") == 0) {
		watt = response.toInt();
	} else {
		// extract the value from a JSON string (only 1st occurence)
		// Example: {"Time":"2022-12-10T17:25:46","Main":{"power":-123,"from_grid":441.231,"to_grid":9578.253}}
		// cfgPvHttpJson = power\":                      |      |------>
		// the slash \ will escape the quote " sign
		char *pch = strstr(response.c_str(), cfgPvHttpJson); // search the index of cfgPvHttpJson, then add it's length
		if (pch != NULL) {
			pch += strlen(cfgPvHttpJson); 
			watt = atol(pch);
		}
	}
  LOG(m, "Watt=%d", watt)
	
	int32_t batt = 0;

	if (strcmp(cfgPvHttpJsonBatt, "") == 0) {
		// no search-string configured --> do nothing
		;
	} else {
		char *pch = strstr(response.c_str(), cfgPvHttpJsonBatt); // search the index of cfgPvHttpJsonBatt, then add it's length
		if (pch != NULL) {
			pch += strlen(cfgPvHttpJsonBatt); 
			batt = -atol(pch);  // battery power (pos. = discharging battery, neg. = charging battery)
			watt += batt;      // adding means, that watt will become smaller (=> availPower will become higher), when battery is charging 
		}
		LOG(m, "Batt=%d", batt)
	}
	pv_setWatt(watt);
}
