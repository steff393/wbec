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

static uint32_t lastHandleCall       = 0;

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
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(error.f_str());
		return;
	}
	log(m, String(F("Response: ") + String(doc["Watt"].as<float>())));
}

