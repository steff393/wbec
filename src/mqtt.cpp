// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "globalConfig.h"
#include "logger.h"
#include "mbComm.h"
#include <PubSubClient.h>

const uint8_t m = 2;

WiFiClient espClient;
PubSubClient client(espClient);
uint32_t 	lastMsg = 0;
uint32_t 	lastReconnect = 0;


void callback(char* topic, byte* payload, uint8_t length)
{
	// handle received message
	char buffer[length+1];	// +1 for string termination
	for (uint8_t i = 0; i < length; i++) {
		buffer[i] = (char)payload[i];		
	}
	buffer[length] = '\0';			// add string termination
	LOGN(m, "Received: %s, Payload: %s", topic, buffer)

	//if (topic.startsWith(F("openWB/lp/")) && topic.endsWith(F("/AConfigured"))) {
	if (strstr_P(topic, PSTR("openWB/lp/")) && strstr_P(topic, PSTR("/AConfigured"))) {
		uint16_t val = atoi(buffer);
		uint8_t lp  = topic[10] - '0'; 	// loadpoint nr.
		uint8_t i;
		// search, which index fits to loadpoint, first element will be selected
		for (i = 0; i < cfgCntWb; i++) {
			if (cfgMqttLp[i] == lp) {break;}
		}
		if (cfgMqttLp[i] == lp) {
			// openWB has 1A resolution, wbec has 0.1A resulotion
			val = val * 10;
			// set current
			if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
				LOG(0, ", Write to box: %d Value: %d", i, val)
				mb_writeReg(i, REG_CURR_LIMIT, val);
			}
		} else {
			LOG(0, ", no box assigned", "");
		}
	}
}


void mqtt_begin() {
	if (strcmp(cfgMqttIp, "") != 0) {
  	client.setServer(cfgMqttIp, 1883);
		client.setCallback(callback);
	}
}

void reconnect() {
	LOGN(m, "Attempting MQTT connection...", "");
	// Create a random client ID
	char clientId[10];
	snprintf_P(clientId, sizeof(clientId), PSTR("wbec-%d"), (uint8_t)random(255));

	// Attempt to connect
	boolean con = false;
	if (strcmp(cfgMqttUser, "") != 0 && strcmp(cfgMqttPass, "") != 0) {
		con = client.connect(clientId, cfgMqttUser, cfgMqttPass);
	} else {
		con = client.connect(clientId);
	}
	if (con)
	{
		LOG(0, "connected", "");
		//once connected to MQTT broker, subscribe command if any
		for (uint8_t i = 0; i < cfgCntWb; i++) {
			char topic[40];
			if (cfgMqttLp[i] != 0) {
				snprintf_P(topic, sizeof(topic), PSTR("openWB/lp/%d/AConfigured"), cfgMqttLp[i]);
				client.subscribe(topic);
			}
		}
	} else {
		LOG(m, "failed, rc=%d try again in 5 seconds", client.state())
	}
}

void mqtt_handle() {
	if (strcmp(cfgMqttIp, "") != 0) {
		uint32_t now = millis();

		if (!client.connected()) {
			if (now - lastReconnect > 5000 || lastReconnect == 0) {
				reconnect();
				lastReconnect = now;
			}
		}

		client.loop();
	}
}


void mqtt_publish(uint8_t i) {
	if (strcmp(cfgMqttIp, "") == 0 || cfgMqttLp[i] == 0) {
		return;	// do nothing, when Mqtt is not configured, or box has no loadpoint assigned
	}
	
	uint8_t ps = 0;
	uint8_t cs = 0;

	switch(content[i][1]) {
		case 2:  ps = 0; cs = 0; break;
		case 3:  ps = 0; cs = 0; break;
		case 4:  ps = 1; cs = 0; break;
		case 5:  ps = 1; cs = 0; break;
		case 6:  ps = 1; cs = 0; break;
		case 7:  ps = 1; cs = 1; break;
		default: ps = 0; cs = 0; break; 
	}

	// publish the contents of box i
	char header[20];
	char topic[40];
	char value[15];
	snprintf_P(header, sizeof(header), PSTR("openWB/set/lp/%d"), cfgMqttLp[i]);
	boolean retain = true;
	
	snprintf_P(topic, sizeof(topic), PSTR("%s/plugStat"), header);
	snprintf_P(value, sizeof(value), PSTR("%d"), ps);
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/chargeStat"), header);
	snprintf_P(value, sizeof(value), PSTR("%d"), cs);
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/W"), header);
	snprintf_P(value, sizeof(value), PSTR("%d"), content[i][10]);
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/kWhCounter"), header);
	snprintf_P(value, sizeof(value), PSTR("%.3f"), (float)((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 1000.0);
	client.publish(topic, value, retain);

	for (uint8_t ph = 1; ph <= 3; ph++) {
		snprintf_P(topic, sizeof(topic), PSTR("%s/VPhase%d"), header, ph);
		snprintf_P(value, sizeof(value), PSTR("%d"), content[i][ph+5]);	// L1 = 6, L2 = 7, L3 = 8
		client.publish(topic, value, retain);
	}

	for (uint8_t ph = 1; ph <= 3; ph++) {
		snprintf_P(topic, sizeof(topic), PSTR("%s/APhase%d"), header, ph);
		snprintf_P(value, sizeof(value), PSTR("%.1f"), (float)content[i][ph+1]/10.0);	// L1 = 2, L2 = 3, L3 = 4
		client.publish(topic, value, retain);
	}
	LOG(m, "Publish to %s", header)
}

void mqtt_log(const char *output, const char *msg) {
	if (strcmp(cfgMqttIp, "") == 0) {
		return;	// do nothing, when Mqtt is not configured
	}

	boolean retain = true;
	char topic[10];
	char value[150];

	snprintf_P(topic, sizeof(topic), PSTR("wbec/log"), "");
	snprintf_P(value, sizeof(value), PSTR("%s%s"), output, msg);
	client.publish(topic, value, retain);
}