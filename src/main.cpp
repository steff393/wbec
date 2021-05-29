// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "globalConfig.h"
#include "goEmulator.h"
#include "LittleFS.h"
#include "logger.h"
#include "mbComm.h"
#include "mqtt.h"
#include "phaseCtrl.h"
#include "SPIFFSEditor.h"
#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>
#include "webServer.h"

bool _handlingOTA = false;


void setup() {
  Serial.begin(115200);
  Serial.println("\n\nStarting wbec ;-)");
  
  if(!LittleFS.begin()){ 
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  loadConfig();

  WiFiManager wifiManager;
  char ssid[32]; strcpy(ssid, cfgApSsid);
  char pass[63]; strcpy(pass, cfgApPass);
  wifiManager.autoConnect(ssid, pass);

  // setup the Webserver
  webServer_begin();
  server.begin();

  logger_begin();

  // setup the OTA server
  ArduinoOTA.setHostname("wbec");
  ArduinoOTA.begin();

  ArduinoOTA.onStart([]() 
  {
    _handlingOTA = true;
  });

  mb_setup();
  mqtt_begin();
  Serial.print("Boot time: ");Serial.println(millis());
}


void loop() {
  ArduinoOTA.handle();
  if(!_handlingOTA) {
    logger_handle();
    mb_handle();
    goE_handle();
    mqtt_handle();
    webServer_handle();
    //pc_handle();
  }
}