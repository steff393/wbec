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
#include <pvHttp.h>
#include <rfid.h>
#include <shelly.h>
#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>
#include <webServer.h>
#include <webSocket.h>

const uint8_t m = 14;


static bool _handlingOTA = false;


void setup() {
  rst_info* rinfo = ESP.getResetInfoPtr(); // get reset cause

  Serial.begin(115200);
  Serial.println(F(" ")); Serial.println(F("-----------------"));
  Serial.println(F("Starting wbec ;-)"));
  Serial.println(F("-----------------")); Serial.println(F(" "));
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
  pvHttp_setup();
  inverter_setup();
  btn_setup();
  pv_setup();
  lm_setup();
  LOG(m, "Boot time: %ld ms", millis());
  LOG(m, "Free heap: %ld Byte",ESP.getFreeHeap());

  // For Debugging:
  //
  // https://www.espressif.com/sites/default/files/documentation/esp8266_reset_causes_and_common_fatal_exception_causes_en.pdf
  // Identifying and logging the Reset Cause:
  // rinfo->reason:
  // 0 = Power reboot
  // 1 = Hardware WDT reset
  // 2 = Fatal exception
  // 3 = Software watchdog reset
  // 4 = Software reset
  // 5 = Deep-sleep
  // 6 = Hardware reset
  //
  // Fatal Exception causes
  // 0 = Invalidcommand
  // 6 = Division by zero
  // 9 = Unaligned read/write operation address
  // 28= Access to invalid address
  // 29= Access to invalid address

  LOG(m, "Reset reason: %x", rinfo->reason);
  if (rinfo->reason == REASON_EXCEPTION_RST) 
  {
    LOG(m, "Fatal excception: %d", rinfo->exccause);
  }
  LOG(m, "  epc1=0x%08X", rinfo->epc1);
  LOG(m, "  epc2=0x%08X", rinfo->epc2);
  LOG(m, "  epc3=0x%08X", rinfo->epc3);
  LOG(m, "  excvaddr=%0x%08X", rinfo->excvaddr);
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
    //pvHttp_loop();
    inverter_loop();
    btn_loop();
    pv_loop();
    //pc_handle();
    lm_loop();
    if (cfgLoopDelay <= 10) {          // see #18, might have an effect to reactivity of webserver in some environments 
      delay(cfgLoopDelay);
    }
  }
}
