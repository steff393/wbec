// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ArduinoJson.h>
#include <globalConfig.h>
#include <logger.h>
#include <loadManager.h>
#include <mbComm.h>
#include <powerfox.h>
#include <webSocket.h>
#include <WebSocketsServer.h>

#define CYCLE_TIME	 1000	
#define JSON_LEN      256

static const uint8_t m = 10;

static WebSocketsServer webSocket = WebSocketsServer(81);
static uint32_t lastCall   = 0;
static uint8_t  id         = 0;


static void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
	if(type == WStype_TEXT) {
		LOG(m, "Payload %s", (char *)payload)
		if (length >= 9 && !strncmp((char *)payload, "currLim=", 8)) {
			char * pch;
			pch = strtok((char *)payload, "=");
			pch = strtok(NULL, "=");
			lm_storeRequest(id, atoi(pch));
		} else if (length >= 4 && !strncmp((char *)payload, "id=", 3)) {
			char * pch;
			pch = strtok((char *)payload, "=");
			pch = strtok(NULL, "=");
			id = atoi(pch);
		} else if (strstr_P((char *)payload, PSTR("PV_OFF"))) {
			pf_setMode(PV_OFF);
		} else if (strstr_P((char *)payload, PSTR("PV_ACTIVE"))) {
			pf_setMode(PV_ACTIVE);
		} else if (strstr_P((char *)payload, PSTR("PV_MIN_PV"))) {
			pf_setMode(PV_MIN_PV);
		}
	} 
}


void webSocket_setup() {
	// start the WebSocket connection
	webSocket.begin();
	webSocket.onEvent(webSocketEvent);
}


void webSocket_loop() {
	webSocket.loop();
	if ((millis() - lastCall < CYCLE_TIME)) {
		return;
	}
	lastCall = millis();

	StaticJsonDocument<JSON_LEN> data;
	data[F("id")]       = id;
	data[F("chgStat")]  = content[id][1];
	data[F("power")]    = content[id][10];
	data[F("energyI")]  = (float)((uint32_t) content[id][13] << 16 | (uint32_t)content[id][14]) / 1000.0;
	data[F("currLim")]  = (float)content[id][53]/10.0;
	data[F("watt")]     = pf_getWatt();
	data[F("pvMode")]   = pf_getMode();
	data[F("timeNow")]  = log_time();
	char response[JSON_LEN];
	serializeJson(data, response, JSON_LEN);
	webSocket.broadcastTXT(response);
}
