// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "globalConfig.h"
#include "logger.h"
#include <WiFiClientSecure.h>

const uint8_t m = 9;

#define CYCLE_TIME		   60000		// 60 s	=> 1440 calls per day (fair-use limit: 8000 per day)

uint32_t powerfox_lastHandleCall       = 0;

void powerfox_loop() {
	if ((millis() - powerfox_lastHandleCall < CYCLE_TIME) || !strcmp(cfgFoxUser, "") || !strcmp(cfgFoxPass, "") || !strcmp(cfgFoxDevId, "")) {
		// avoid unnecessary frequent calls and block the feature, when more than 1 wallbox is connected, due to timing reasons
		return;
	}
	powerfox_lastHandleCall = millis();

	WiFiClient client;
	//WiFiClientSecure client;
	HTTPClient http;

	//client.setInsecure(); //the magic line, use with caution
	//client.connect(host, httpsPort);

	http.begin(client, "http://backend.powerfox.energy/api/2.0/my/" + String(cfgFoxDevId) + "/current");
	//http.begin(client, "http://httpbin.org/basic-auth/" + String(cfgFoxUser) + "/" + cfgFoxPass);
 
	String auth = base64::encode(cfgFoxUser + String(":") + cfgFoxPass);
	http.addHeader("Authorization", "Basic " + auth);

	int httpCode = http.GET(); 

	if (httpCode > 0) { //Check for the returning code

			String payload = http.getString();
			log(m, String(httpCode));
			log(m, payload);

			DynamicJsonDocument doc(256);
			  // Parse JSON object
			DeserializationError error = deserializeJson(doc, payload);
			if (error) {
				Serial.print(F("deserializeJson() failed: "));
				Serial.println(error.f_str());
				client.stop();
				return;
			}
			/* { 
				"Watt": 84.0, 
				"Timestamp": 1566909358, 
				"A_Plus": 276696.6, 
				"A_Plus_HT": 193687.6, 
				"A_Plus_NT": 83009.0, 
				"A_Minus": 0.0, 
				"Outdated ": false 
			}  */
			// Extract values
			//log(m, String("Response: " + String(doc["user"].as<char*>())));
			log(m, String("Response: " + String(doc["Watt"].as<float>())));
			}

	else {
		log(m, "Error on HTTP request:");
		log(m, String(httpCode));
	}

	http.end();
}
