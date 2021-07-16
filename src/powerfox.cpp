// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ArduinoJson.h>
#include "globalConfig.h"
#include "logger.h"

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>


const uint8_t m = 9;

#define CYCLE_TIME		   20000		// 60 s	=> 1440 calls per day (fair-use limit: 8000 per day)
#define MAX_API_LEN				 150		// Max accepted length of API response
#define MIN_HEAP_NEEDED  15000		// This heap is minimum necessary, otherwise ESP will crash during HTTPS / TLS connection
#define OUTDATED           600		// 10 min, after this time the value is considered outdated
#define WATT_MIN       -100000		// 100kW Feed-in
#define WATT_MAX        100000		// 100kW Consumption

static uint32_t lastHandleCall       = 0;
static uint32_t timestamp            = 0;
static int32_t  watt                 = 0;

HTTPClient                *http;
BearSSL::WiFiClientSecure *httpsClient;


void powerfox_setup() {
	
}


void powerfox_loop() {
	if (((millis() - lastHandleCall < CYCLE_TIME) && lastHandleCall != 0) || 
			!strcmp(cfgFoxUser, "") || !strcmp(cfgFoxPass, "") || !strcmp(cfgFoxDevId, "")) {
		// avoid unnecessary frequent calls
		return;
	}
	lastHandleCall = millis();

	Serial.print(F("Heap before new: ")); Serial.println(ESP.getFreeHeap());
	if (ESP.getFreeHeap() < MIN_HEAP_NEEDED) {
		LOG(m, "Not enough Heap", "")
		return;
	}
	http = new HTTPClient();
	httpsClient = new BearSSL::WiFiClientSecure();
	char response[MAX_API_LEN];
	Serial.print(F("Heap after new : ")); Serial.println(ESP.getFreeHeap());

	httpsClient->setInsecure();
	httpsClient->setBufferSizes(512,512);    // must be between 512 and 16384
	http->begin(*httpsClient, F("https://backend.powerfox.energy/api/2.0/my/") + String(cfgFoxDevId) + F("/current"));

	http->setAuthorization(cfgFoxUser, cfgFoxPass);
	http->setReuse(false);
	uint32_t tm = millis();
	http->GET();
	Serial.print(F("Duration of GET: ")); Serial.println(millis() - tm);
	
	while (httpsClient->connected() || httpsClient->available()) {
		if (httpsClient->available()) {
			httpsClient->read((uint8_t*)response, MAX_API_LEN-1);
		}
	}
	Serial.println(response);
	delete http;
  delete httpsClient;
	Serial.print(F("Heap after del : ")); Serial.println(ESP.getFreeHeap());

	DynamicJsonDocument doc(256);
	// Parse JSON object
	DeserializationError error = deserializeJson(doc, response);
	if (error) {
		LOG(m, "deserializeJson() failed: %s", error.f_str())
		return;
	}
	timestamp =  doc[F("Timestamp")]        | 0;
	watt = (int) doc[F("Watt")].as<float>();
	LOG(m, "Timestamp=%d, Watt=%d", timestamp, watt)
  
}

int32_t pf_getWatt() {
	if ((log_unixTime() - timestamp > OUTDATED) || (watt < WATT_MIN) || (watt > WATT_MAX)) {
		return(0);
	} else {
		return(watt);
	}
}