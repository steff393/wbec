// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <button.h>
#include <globalConfig.h>
#include <goEmulator.h>
#include <inverter.h>
#include <LittleFS.h>
#include <loadManager.h>
#include <logger.h>
#include <mbComm.h>
#include <mqtt.h>
#include <phaseCtrl.h>
#include <powerfox.h>
#include <pvAlgo.h>
#include <rfid.h>
#include <shelly.h>
#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>
#include <webServer.h>
#include <webSocket.h>
#include <DeviceManager.hpp>


static bool _handlingOTA = false;
DeviceManager deviceManager;


void setup() {
  Serial.begin(115200);
  Serial.println(F("\n\nStarting wbec ;-)"));
  logger_allocate();
  
  if(!LittleFS.begin()){ 
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }

  loadConfig();

  WiFiManager wifiManager;
  char ssid[32]; strcpy(ssid, cfgApSsid);
  char pass[63]; strcpy(pass, cfgApPass);
  wifiManager.setConnectTimeout(cfgWifiConnectTimeout);
  wifiManager.autoConnect(ssid, pass);

  // still experimental (see #12):
  if (cfgWifiSleepMode >= WIFI_NONE_SLEEP && cfgWifiSleepMode <= WIFI_MODEM_SLEEP) {
    WiFi.setSleepMode((WiFiSleepType_t)cfgWifiSleepMode);
  }

  logger_setup();

  // setup the Webserver
  webServer_setup();
  webSocket_setup();

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
  shelly_setup();
  inverter_setup();
  btn_setup();
  pv_setup();
  lm_setup();
  deviceManager.setup();
  Serial.print(F("Boot time: ")); Serial.println(millis());
  Serial.print(F("Free heap: ")); Serial.println(ESP.getFreeHeap());
}


void loop() {
  ArduinoOTA.handle();
  if(!_handlingOTA) {
    logger_loop();
    mb_loop();
    goE_handle();
    mqtt_handle();
    webServer_loop();
    webSocket_loop();
    rfid_loop();
    powerfox_loop(); 
    shelly_loop();
    inverter_loop();
    btn_loop();
    pv_loop();
    //pc_handle();
    lm_loop();
    deviceManager.loop();
    if (cfgLoopDelay <= 10) {          // see #18, might have an effect to reactivity of webserver in some environments 
      delay(cfgLoopDelay);
    }
  }
}
