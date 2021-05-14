// Copyright (c) 2021 steff393

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "mbComm.h"
#include <PubSubClient.h>

const char* mqtt_server = "192.168.178.39";

WiFiClient espClient;
PubSubClient client(espClient);
uint32_t 	lastMsg = 0;
uint32_t 	lastReconnect = 0;
char 			msg[50];
uint8_t 	value = 0;


void callback(char* topic, byte* payload, uint8_t length)
{
    // handle received message
		Serial.print("MQTT received: ");
		Serial.print(topic);
		Serial.print(", ");
		for (uint8_t i = 0; i < length; i++) {
    	Serial.print((char)payload[i]);
  	}
		Serial.print(", len: ");
		Serial.println(length);

}


void mqtt_begin() {
  client.setServer(mqtt_server, 1883);
	client.setCallback(callback);
	
}

void mqtt_reconnect() {
  Serial.print("Attempting MQTT connection...");
	// Create a random client ID
	String clientId = "wbec-";
	clientId += String(random(0xffff), HEX);
	// Attempt to connect
	if (client.connect(clientId.c_str()))				// alternative: client.connect(clientId,userName,passWord)
	{
		Serial.println("connected");
		//once connected to MQTT broker, subscribe command if any
		client.subscribe("openWB/lp/2/AConfigured");
	} else {
		Serial.print("failed, rc=");
		Serial.print(client.state());
		Serial.println(" try again in 5 seconds");
	}
}

void mqtt_handle() {
	uint32_t now = millis();

  if (!client.connected()) {
		if (now - lastReconnect > 5000 || lastReconnect == 0) {
	    mqtt_reconnect();
			lastReconnect = now;
		}
  }

  client.loop();

  if (now - lastMsg > 5000) {
    lastMsg = now;           
		boolean retain = true;
    client.publish("openWB/set/evu/W", "1456", retain);
    client.publish("openWB/set/evu/APhase1", "11", retain);
		client.publish("openWB/set/evu/APhase2", "12", retain);
		client.publish("openWB/set/evu/APhase3", "13", retain);
		client.publish("openWB/set/evu/WhImported", "1234", retain);
		client.publish("openWB/set/evu/WhExported", "1567", retain);
		client.publish("openWB/set/evu/VPhase1", "231", retain);
		client.publish("openWB/set/evu/VPhase2", "232", retain);
		client.publish("openWB/set/evu/VPhase3", "233", retain);
		client.publish("openWB/set/evu/HzFrequenz", "50", retain);

		client.publish("openWB/set/lp/2/plugStat", "1", retain);
		client.publish("openWB/set/lp/2/chargeStat", "1", retain);
		Serial.print("MQTT sent, state "); Serial.println(client.state());
  }
}


void mqtt_publish(uint8_t i) {
	// publish the contents of box i
	String header = String("openWB/set/lp/" + i);
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

	client.publish(String(header + "/plugStat").c_str(), String(ps).c_str(), retain);
	client.publish(String(header + "/chargeStat").c_str(), String(cs).c_str(), retain);

	client.publish(String(header + "/W").c_str(),          String(cs).c_str(), retain);
	client.publish(String(header + "/kWhCounter").c_str(), String(cs).c_str(), retain);
}




/* 	data["alw"] = String(alw);
	data["amp"] = String(amp);
	data["err"] = "0";
	data["stp"] = "0";
	data["tmp"] = String(content[i][5]);
	data["dws"] = String(((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) - energyI);
	data["ubi"] = "0";
	data["eto"] = String(((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 100);
	data["nrg"][0] = content[i][6]; // L1
	data["nrg"][1] = content[i][7]; // L2
	data["nrg"][2] = content[i][8]; // L3
	data["nrg"][3] = 0;
	data["nrg"][4] = content[i][2]; // L1
	data["nrg"][5] = content[i][3]; // L2
	data["nrg"][6] = content[i][4]; // L3
	data["nrg"][7] = 0;
	data["nrg"][8] = 0;
	data["nrg"][9] = 0;
	data["nrg"][10] = 0;
	data["nrg"][11] = content[i][10] / 10;
	data["nrg"][12] = 0;
	data["nrg"][13] = 0;
	data["nrg"][14] = 0;
	data["nrg"][15] = 0;
	data["fwv"] = "40"; */