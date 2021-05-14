// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "globalConfig.h"
#include "mbComm.h"
#include <PubSubClient.h>


WiFiClient espClient;
PubSubClient client(espClient);
uint32_t 	lastMsg = 0;
uint32_t 	lastReconnect = 0;


void callback(char* _topic, byte* payload, uint8_t length)
{
	String topic = String(_topic);
	char *buffer = (char *) malloc(length);
	strcpy(buffer, (char *)payload);
	
	// handle received message
	Serial.print("MQTT: Received: "); Serial.print(topic); Serial.print(", Payload: ");
	for (uint8_t i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}

	if (topic.startsWith("openWB/lp/") && topic.endsWith("/AConfigured")) {
		uint8_t val = String(buffer).toInt();
		uint8_t i   = topic.substring(10,11).toInt() - 1;		// id of the box
		// openWB has 1A resolution, wbec has 0.1A resulotion
		val = val * 10;
		// set current
		if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
			Serial.print(" Write to box: "); Serial.print(i); Serial.print(" Value: "); Serial.println(val); 
			mb_writeReg(i, REG_CURR_LIMIT, val);
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
  Serial.print("Attempting MQTT connection...");
	// Create a random client ID
	String clientId = "wbec-";
	clientId += String(random(0xffff), HEX);
	// Attempt to connect
	if (client.connect(clientId.c_str()))				// alternative: client.connect(clientId,userName,passWord)
	{
		Serial.println("connected");
		//once connected to MQTT broker, subscribe command if any
		client.subscribe("openWB/lp/1/AConfigured");
		//client.subscribe("openWB/lp/+/AConfigured");
	} else {
		Serial.print("failed, rc=");
		Serial.print(client.state());
		Serial.println(" try again in 5 seconds");
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
	if (strcmp(cfgMqttIp, "") == 0 || i >= 8) {
		return;	// openWB only supports 8 load points
	}

	// publish the contents of box i
	String header = String("openWB/set/lp/") + String(i + 1);
	boolean retain = true;
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
	client.publish(String(header + "/plugStat").c_str(),   String(ps).c_str(), retain);
	client.publish(String(header + "/chargeStat").c_str(), String(cs).c_str(), retain);

	client.publish(String(header + "/W").c_str(),          String(content[i][10]).c_str(), retain);
	client.publish(String(header + "/kWhCounter").c_str(), String(((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 1000).c_str(), retain);
	client.publish(String(header + "/VPhase1").c_str(),    String(content[i][6]).c_str(), retain);
	client.publish(String(header + "/VPhase2").c_str(),    String(content[i][7]).c_str(), retain);
	client.publish(String(header + "/VPhase3").c_str(),    String(content[i][8]).c_str(), retain);
	client.publish(String(header + "/APhase1").c_str(),    String(content[i][2]).c_str(), retain);
	client.publish(String(header + "/APhase2").c_str(),    String(content[i][3]).c_str(), retain);
	client.publish(String(header + "/APhase3").c_str(),    String(content[i][4]).c_str(), retain);
	Serial.print("MQTT: Publish to "); Serial.println(header);
}
