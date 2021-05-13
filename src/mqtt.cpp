// Copyright (c) 2021 steff393

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* mqtt_server = "192.168.178.39";

WiFiClient espClient;
PubSubClient client(espClient);
uint32_t 	lastMsg = 0;
uint32_t 	lastReconnect = 0;
char 			msg[50];
uint8_t 	value = 0;

void mqtt_begin() {
  client.setServer(mqtt_server, 1883);
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
		client.subscribe("openWB");
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
		Serial.print("MQTT sent, state "); Serial.println(client.state());
  }
}