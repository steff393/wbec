// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <globalConfig.h>
#include <inverter.h>
#include <logger.h>
#include <loadManager.h>
#include <mbComm.h>
#include <PubSubClient.h>
#include <pvAlgo.h>
#include <rfid.h>


const uint8_t m = 2;
const char*   lastWillTopic  = "wbec/connection";
const char*   lastWillMsgOff = "offline";
const char*   lastWillMsgOn  = "online";
const uint8_t lastWillQos    = 1;
const bool	  lastWillRetain = true;

WiFiClient espClient;
PubSubClient client(espClient);
uint32_t 	lastMsg = 0;
uint32_t 	lastReconnect = 0;
uint8_t   maxcurrent[WB_CNT];
boolean   callbackActive = false;


void callback(char* topic, byte* payload, uint8_t length)
{
	callbackActive = true;
	// handle received message
	char buffer[length+1];	// +1 for string termination
	for (uint8_t i = 0; i < length; i++) {
		buffer[i] = (char)payload[i];		
	}
	buffer[length] = '\0';			// add string termination
	LOGN(m, "Received: %s, Payload: %s", topic, buffer)

	// topics for openWB
	if (strstr_P(topic, PSTR("openWB/lp/")) && strstr_P(topic, PSTR("/AConfigured"))) {
		uint16_t val = atoi(buffer);
		uint8_t lp  = topic[10] - '0'; 	// loadpoint nr.
		uint8_t i;
		// search, which index fits to loadpoint, first element will be selected
		for (i = 0; i < cfgCntWb; i++) {
			if (cfgMqttLp[i] == lp) {break;}
		}
		if (cfgMqttLp[i] == lp) {
			// openWB has 1A resolution, wbec has 0.1A resolution
			val = val * 10;
			// set current
			if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
				LOG(0, ", Write to box: %d Value: %d", i, val)
				lm_storeRequest(i, val);
			}
		} else {
			LOG(0, ", no box assigned", "");
		}
	}

	// topics for openWB 2.0 (#75)
	if (strstr_P(topic, PSTR("openWB/chargepoint/")) && strstr_P(topic, PSTR("/set/current"))) {
		float val = atof(buffer);
		uint8_t lp  = topic[19] - '0'; 	// loadpoint nr.
		uint8_t i;
		// search, which index fits to loadpoint, first element will be selected
		for (i = 0; i < cfgCntWb; i++) {
			if (cfgMqttLp[i] == lp) {break;}
		}
		if (cfgMqttLp[i] == lp) {
			// openWB resolution is unclear (float with example value 12.34), wbec has 0.1A resolution
			val = val * 10;
			// set current
			if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
				LOG(0, ", Write to box: %d Value: %d", i, (uint8_t)val)
				lm_storeRequest(i, (uint8_t)val);
			}
		} else {
			LOG(0, ", no box assigned", "");
		}
	}
	
	// topics for EVCC
	if (strstr_P(topic, PSTR("wbec/lp/"))   && strstr_P(topic, PSTR("/maxcurrent"))) {
		float val = atof(buffer);
		uint8_t lp  = topic[8] - '0'; 	// loadpoint nr.
		uint8_t i;
		// search, which index fits to loadpoint, first element will be selected
		for (i = 0; i < cfgCntWb; i++) {
			if (cfgMqttLp[i] == lp) {break;}
		}
		if (cfgMqttLp[i] == lp) {
			// EVCC has 1A resolution, wbec has 0.1A resolution
			val = val * 10;
			// set current
			if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
				LOG(0, ", Write to box: %d Value: %d", i, (uint16_t) val)
				lm_storeRequest(i, val);
			}
		} else {
			LOG(0, ", no box assigned", "");
		}
	}

	if (strstr_P(topic, PSTR("wbec/lp/"))   && strstr_P(topic, PSTR("/enable"))) {
		uint8_t lp  = topic[8] - '0'; 	// loadpoint nr.
		uint8_t i;
		// search, which index fits to loadpoint, first element will be selected
		for (i = 0; i < cfgCntWb; i++) {
			if (cfgMqttLp[i] == lp) {break;}
		}
		if (cfgMqttLp[i] == lp) {
			if (strstr_P(buffer, PSTR("true"))) {
				LOG(0, ", Enable box: %d", i)
				lm_storeRequest(i, maxcurrent[i]);
			} else {
				LOG(0, ", Disable box: %d", i)
				lm_storeRequest(i, 0);
			}
		} else {
			LOG(0, ", no box assigned", "");
		}
	}

	// set the watt value via MQTT (#54)
	if (strcmp(topic, cfgMqttWattTopic) == 0) {
		if (strcmp(cfgMqttWattJson, "") == 0) {
			// directly take the value from the buffer
			pv_setWatt(atol(buffer));
		} else {
			// extract the value from a JSON string (only 1st occurence)
			// Example: {"Time":"2022-12-10T17:25:46","Main":{"power":-123,"from_grid":441.231,"to_grid":9578.253}}
			// cfgMqttWattJson = power\":                      |      |------>
			// the slash \ will escape the quote " sign
			char * pch;
			pch = strstr(buffer, cfgMqttWattJson) + strlen(cfgMqttWattJson); // search the index of cfgMqttWattJson, then add it's length
			pv_setWatt(atol(pch));
		}
	}

	callbackActive = false;
}


void mqtt_begin() {
	if (strcmp(cfgMqttIp, "") != 0) {
  	client.setServer(cfgMqttIp, cfgMqttPort);
		client.setCallback(callback);
	}
	for (uint8_t i = 0; i < cfgCntWb; i++) {
		maxcurrent[i] = CURR_ABS_MIN;
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
		con = client.connect(clientId, cfgMqttUser, cfgMqttPass, lastWillTopic, lastWillQos, lastWillRetain, lastWillMsgOff);
	} else {
		con = client.connect(clientId, lastWillTopic, lastWillQos, lastWillRetain, lastWillMsgOff);
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
				snprintf_P(topic, sizeof(topic), PSTR("wbec/lp/%d/enable"), cfgMqttLp[i]);
				client.subscribe(topic);
				snprintf_P(topic, sizeof(topic), PSTR("wbec/lp/%d/maxcurrent"), cfgMqttLp[i]);
				client.subscribe(topic);
			}
		}
		client.subscribe(cfgMqttWattTopic);
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
	char status;

	switch(content[i][1]) {
		case 2:  ps = 0; cs = 0; status = 'A'; break;
		case 3:  ps = 0; cs = 0; status = 'A'; break;
		case 4:  ps = 1; cs = 0; status = 'B'; break;
		case 5:  ps = 1; cs = 0; status = 'B'; break;
		case 6:  ps = 1; cs = 0; status = 'C'; break;
		case 7:  ps = 1; cs = 1; status = 'C'; break;
		default: ps = 0; cs = 0; status = 'F'; break; 
	}

	// publish the contents of box i
	char header[30];
	char topic[50];
	char value[20];
	
	// topics for openWB
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

	// topics for openWB 2.0 (#75)
	snprintf_P(header, sizeof(header), PSTR("openWB/set/chargepoint/%d"), cfgMqttLp[i]);

	snprintf_P(topic, sizeof(topic), PSTR("%s/get/plug_state"), header);
	snprintf_P(value, sizeof(value), PSTR("%s"), ps?"true":"false");
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/get/charge_state"), header);
	snprintf_P(value, sizeof(value), PSTR("%s"), cs?"true":"false");
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/get/power"), header);
	snprintf_P(value, sizeof(value), PSTR("%d"), content[i][10]);
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/get/imported"), header);
	snprintf_P(value, sizeof(value), PSTR("%ld"), ((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) );
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/get/voltages"), header);
	snprintf_P(value, sizeof(value), PSTR("[%d,%d,%d]"), content[i][6], content[i][7], content[i][8]);	// L1 = 6, L2 = 7, L3 = 8
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/get/currents"), header);
	snprintf_P(value, sizeof(value), PSTR("[%.1f,%.1f,%.1f]"), (float)content[i][2]/10.0, (float)content[i][3]/10.0, (float)content[i][4]/10.0);	// L1 = 2, L2 = 3, L3 = 4
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/get/phases_in_use"), header);
	snprintf_P(value, sizeof(value), PSTR("%d"), cfgPvPhFactor / 23);
	client.publish(topic, value, retain);
	
	snprintf_P(topic, sizeof(topic), PSTR("%s/get/rfid_tag"), header);
	snprintf_P(value, sizeof(value), PSTR("%s"), rfid_getLastID());
	client.publish(topic, value, retain);

	// topics for EVCC
	snprintf_P(header, sizeof(header), PSTR("wbec/lp/%d"), cfgMqttLp[i]);

	snprintf_P(topic, sizeof(topic), PSTR("%s/status"), header);
	snprintf_P(value, sizeof(value), PSTR("%c"), status);
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/enabled"), header);
	if (content[i][53] > 0) {
		client.publish(topic, "true", retain);
		maxcurrent[i] = content[i][53];       // memorize the current limit if not 0
	} else {
		client.publish(topic, "false", retain);
	}

	snprintf_P(topic, sizeof(topic), PSTR("%s/power"), header);
	snprintf_P(value, sizeof(value), PSTR("%d"), content[i][10]);
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/energy"), header);
	snprintf_P(value, sizeof(value), PSTR("%.3f"), (float)((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 1000.0);
	client.publish(topic, value, retain);
	
	for (uint8_t ph = 1; ph <= 3; ph++) {
		snprintf_P(topic, sizeof(topic), PSTR("%s/currL%d"), header, ph);
		snprintf_P(value, sizeof(value), PSTR("%.1f"), (float)content[i][ph+1]/10.0);	// L1 = 2, L2 = 3, L3 = 4
		client.publish(topic, value, retain);
	}
	
	for (uint8_t ph = 1; ph <= 3; ph++) {
		snprintf_P(topic, sizeof(topic), PSTR("%s/voltL%d"), header, ph);
		snprintf_P(value, sizeof(value), PSTR("%d"), content[i][ph+5]);	// L1 = 2, L2 = 3, L3 = 4
		client.publish(topic, value, retain);
	}
	snprintf_P(topic, sizeof(topic), PSTR("%s/currLimit"), header);
	snprintf_P(value, sizeof(value), PSTR("%.1f"), (float)content[i][53]/10.0);
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/pcbTemp"), header);
	snprintf_P(value, sizeof(value), PSTR("%.1f"), (float)content[i][5]/10.0);
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/resCode"), header);
	snprintf_P(value, sizeof(value), PSTR("%s"), String(modbusResultCode[i], HEX));
	client.publish(topic, value, retain);

	int qrssi = WiFi.RSSI();     
	snprintf_P(topic, sizeof(topic), PSTR("%s/wifiRssi"), header);
	snprintf_P(value, sizeof(value), PSTR("%d"), qrssi);
	client.publish(topic, value, retain);
	snprintf_P(topic, sizeof(topic), PSTR("%s/wifiChannel"), header);
	snprintf_P(value, sizeof(value), PSTR("%d"), WiFi.channel());
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/plugState"), header);
	snprintf_P(value, sizeof(value), PSTR("%s"), ps?"true":"false");
	client.publish(topic, value, retain);

	snprintf_P(topic, sizeof(topic), PSTR("%s/chargeState"), header);
	snprintf_P(value, sizeof(value), PSTR("%s"), cs?"true":"false");
	client.publish(topic, value, retain);

	
	// publish values from inverter
	if (strcmp(cfgInverterIp, "") != 0) {
		snprintf_P(header, sizeof(header), PSTR("wbec/inverter"));

		snprintf_P(topic, sizeof(topic), PSTR("%s/pwrInv"), header);
		snprintf_P(value, sizeof(value), PSTR("%d"), inverter_getPwrInv());
		client.publish(topic, value, retain);

		snprintf_P(topic, sizeof(topic), PSTR("%s/pwrMet"), header);
		snprintf_P(value, sizeof(value), PSTR("%d"), inverter_getPwrMet());
		client.publish(topic, value, retain);
	}

	// publish values from pvAlgo
	if (pv_getMode()) {
		snprintf_P(header, sizeof(header), PSTR("wbec/pv"));

		snprintf_P(topic, sizeof(topic), PSTR("%s/mode"), header);
		snprintf_P(value, sizeof(value), PSTR("%d"), pv_getMode());
		client.publish(topic, value, retain);

		snprintf_P(topic, sizeof(topic), PSTR("%s/watt"), header);
		snprintf_P(value, sizeof(value), PSTR("%ld"), pv_getWatt());
		client.publish(topic, value, retain);
	}

	// Wbec-Connection Status
	client.publish(lastWillTopic, lastWillMsgOn, lastWillRetain);
}

void mqtt_log(const char *output, const char *msg) {
	if (strcmp(cfgMqttIp, "") == 0 || callbackActive) {
		return;	// do nothing, when Mqtt is not configured OR when request comes from mqtt callback (#13)
	}

	boolean retain = true;
	char topic[10];
	char value[150];

	snprintf_P(topic, sizeof(topic), PSTR("wbec/log"), "");
	snprintf_P(value, sizeof(value), PSTR("%s%s"), output, msg);
	client.publish(topic, value, retain);
}