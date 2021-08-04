// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "globalConfig.h"
#include "goEmulator.h"
#include <LittleFS.h>
#include "logger.h"
#include "mbComm.h"
#include "mqtt.h"
#include "phaseCtrl.h"
#include "powerfox.h"
#include "rfid.h"
#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>
#include "webServer.h"
#include "webSocket.h"

bool _handlingOTA = false;


void setup() {
  Serial.begin(115200);
  Serial.println(F("\n\nStarting wbec ;-)"));
  logger_allocate();

  // define a GPIO as output
  pinMode(PIN_RST, OUTPUT);
  
  if(!LittleFS.begin()){ 
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }

  loadConfig();

  WiFiManager wifiManager;
  char ssid[32]; strcpy(ssid, cfgApSsid);
  char pass[63]; strcpy(pass, cfgApPass);
  wifiManager.autoConnect(ssid, pass);

  logger_begin();

  // setup the Webserver
  webServer_begin();
  webSocket_begin();

  // setup the OTA server
  ArduinoOTA.setHostname("wbec");
  ArduinoOTA.begin();

  ArduinoOTA.onStart([]() 
  {
    _handlingOTA = true;
  });

  mb_setup();
  mqtt_begin();
  rfid_setup();
  powerfox_setup();
  Serial.print(F("Boot time: ")); Serial.println(millis());
  Serial.print(F("Free heap: ")); Serial.println(ESP.getFreeHeap());
}


void loop() {
  ArduinoOTA.handle();
  if(!_handlingOTA) {
    logger_handle();
    mb_handle();
    goE_handle();
    mqtt_handle();
    webServer_handle();
    webSocket_handle();
    rfid_loop();
    powerfox_loop(); 
    //pc_handle();
  }
}