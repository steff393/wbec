// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ArduinoJson.h>
#include <globalConfig.h>
#include <logger.h>
#include <powerfox.h>
#include <pvAlgo.h>
#include <umm_malloc/umm_heap_select.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>


const uint8_t m = 9;

#define MAX_API_LEN		 	    150		// Max accepted length of API response
#define OUTDATED         600000		// 10 min, after this time the value is considered outdated

static uint32_t lastHandleCall       = 0;
static boolean  powerfoxActive       = false;

HTTPClient                *http;
BearSSL::WiFiClientSecure *httpsClient;


void powerfox_setup() {
	// check config values
	if (strcmp(cfgFoxUser, "") && strcmp(cfgFoxPass, "") && strcmp(cfgFoxDevId, "") &&		// look for credentials (all need a value)
			(cfgCntWb == 1)) {		// more wallboxes need too much heap, e.g. for web server
		powerfoxActive = true;
	} else {
		powerfoxActive = false;
	}
}


void powerfox_loop() {
	if ((millis() - lastHandleCall < (uint16_t)cfgPvCycleTime * 1000)  ||      // avoid unnecessary frequent calls
			(powerfoxActive == false)) {
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
		int32_t  watt      = 0;  // power from powerfox API (neg. = 'Einspeisung', pos. = 'Bezug')
		timestamp =  doc[F("Timestamp")]        | 0;
		watt = (int) doc[F("Watt")].as<float>();
		LOG(m, "Timestamp=%d, Watt=%d", timestamp, watt)

		if (log_unixTime() - timestamp <= OUTDATED) {
			pv_setWatt(watt);
		}
	}
	HeapSelectDram ephemeral;
  Serial.printf("DRAM free: %6d bytes\r\n", ESP.getFreeHeap());
}
