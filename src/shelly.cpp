// Copyright (c) 2022 steff393, MIT license
#include <Arduino.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <globalConfig.h>
#include <IPAddress.h>
#include <logger.h>
#include <pvAlgo.h>
#include <shelly.h>

const uint8_t m = 12;

#define MAX_API_LEN		 	    2048		// Max accepted length of API response

static IPAddress remote;   // Address of Modbus Slave device

static bool      shellyActive               = false;
static uint32_t  lastHandleCall             = 0;


void shelly_setup() {
	if (strcmp(cfgShellyIp, "") != 0) {
		if (remote.fromString(cfgShellyIp)) {
			shellyActive = true;
		}
	}
}


void shelly_loop() {
	if ((millis() - lastHandleCall < (uint16_t)cfgPvCycleTime * 1000) ||     // avoid unnecessary frequent calls
			(shellyActive == false)) {
		return;
	}
	lastHandleCall = millis();

	WiFiClient client;
	HTTPClient http;
	char serverPath[35];
	String response;
	sprintf(serverPath, "http://%s/status", cfgShellyIp);
	LOG(m, "url: %s", serverPath);

	http.begin(client, serverPath);
	int16_t httpResponseCode = http.GET();
	if (httpResponseCode > 0) {
		response = http.getString();
		LOG(m, "HTTP Response code: %d, %s", httpResponseCode, response);
	}
	else {
		LOG(m, "Error code: %d", httpResponseCode);
	}
	http.end();

	StaticJsonDocument<MAX_API_LEN> doc;
	// Parse JSON object
	DeserializationError error = deserializeJson(doc, response);
	if (error) {
		LOG(m, "deserializeJson() failed: %s", error.f_str())
		return;
	} 

	uint32_t timestamp = 0;
	int32_t  watt      = 0;  // (neg. = 'Einspeisung', pos. = 'Bezug')
	timestamp =  doc[F("unixtime")]        | 0;
	watt = (int) doc[F("total_power")].as<float>();
	LOG(m, "Timestamp=%d, Watt=%d", timestamp, watt)

	pv_setWatt(watt);
}
