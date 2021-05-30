// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "globalConfig.h"
#include "goEmulator.h"
#include "LittleFS.h"
#include "loadManager.h"
#include "logger.h"
#include "mbComm.h"
#include "phaseCtrl.h"
#include "SPIFFSEditor.h"
#include "webServer.h"
#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>

const uint8_t m = 3;

AsyncWebServer server(80);
boolean resetRequested = false;

void onRequest(AsyncWebServerRequest *request){
  //Handle Unknown Request
  request->send(404);
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  //Handle body
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  //Handle upload
}

String processor(const String& var){
  String ledState;
  Serial.println(var);
  if(var == "STATE"){
    if(modbusResultCode[0]==0x00){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    Serial.print(ledState);
    return(ledState);
  }
  else if (var == "TEMPERATURE"){
    float tmp = content[0][5] / 10.0;
    return(String(tmp));
  }
  else if (var == "VOLTAGE"){
    float tmp = content[0][6] / 10.0;
    return(String(tmp));
  }
  else if (var == "CURRENT"){
    float tmp = content[0][53] / 10.0;
    return(String(tmp));
  } else return(String("notFound"));
}

String getAscii(uint8_t id, uint8_t from, uint8_t len) {
  char ch;
  String ret = "";
  // translate the uint16 values into a String
  for (int i = from; i < (from + len) ; i++) {
    ch = (char) (content[id][i] & 0x00FF);
    ret += ch;
    ch = (char) (content[id][i] >> 8);
    ret += ch;
  }
  return(ret);
}

uint8_t getSignalQuality(int rssi)
{
    int quality = 0;
    if (rssi <= -100) {
        quality = 0;
    } else if (rssi >= -50) {
        quality = 100;
    } else {
        quality = 2 * (rssi + 100);
    }
    return quality;
}

void webServer_begin() {
	// respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    //request->send(200, "application/json", "{\"message\":\"Welcome\"}");
    request->send(LittleFS, "/index.html", String(), false, processor);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });

  server.on("/cfg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/cfg.json", "application/json");
  });

  server.on("/bootlog", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", log_getBuffer());
  });

  server.on("/bootlog_reset", HTTP_GET, [](AsyncWebServerRequest *request){
    log_freeBuffer();
    request->send(200, "text/plain", "Cleared");
  });

  server.on("/delete_cfg", HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.remove("/cfg.json")) {
      request->send_P(200, "text/plain", "cfg.json successfully deleted, resetting defaults at next startup");
    } else {
      request->send_P(200, "text/plain", "cfg.json could not be deleted");
    }
  });

  server.on("/web", HTTP_GET, [](AsyncWebServerRequest *request){
    uint8_t id = 0;
    if (request->hasParam("currLim")) {
      uint16_t val = request->getParam("currLim")->value().toInt();
      if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
        mb_writeReg(id, REG_CURR_LIMIT, val);
      }
    }
    request->send(LittleFS, "/index.html", String(), false, processor);
  });

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request){
    if(modbusResultCode[0]==0x00) {
      request->send_P(200, "text/plain", String("ON").c_str());
    } else {
      request->send_P(200, "text/plain", String("OFF").c_str());
    }
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    float tmp = content[0][5] / 10.0;
    request->send_P(200, "text/plain", String(tmp).c_str());
  });
  
  server.on("/voltage", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(content[0][6]).c_str());
  });
  
  server.on("/current", HTTP_GET, [](AsyncWebServerRequest *request){
    float tmp = content[0][53] / 10.0;
    request->send_P(200, "text/plain", String(tmp).c_str());
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String("Resetting the ESP8266..."));
    resetRequested = true;
  });

  server.on("/resetwifi", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String("Resetting the WiFi credentials..."));
    WiFiManager wm;
    wm.resetSettings();
  });

  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument data((cfgCntWb+2)/3  * 2048);  // always 2048 byte for 3 wallboxes
    uint8_t id = 0;
    // modify values
    if (request->hasParam("id")) {
      id = request->getParam("id")->value().toInt();
    }

    if (request->hasParam("wdTmOut")) {
      mb_writeReg(id, REG_WD_TIME_OUT, request->getParam("wdTmOut")->value().toInt());
    }
    if (request->hasParam("currLim")) {
      uint16_t val = request->getParam("currLim")->value().toInt();
      if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
        mb_writeReg(id, REG_CURR_LIMIT, val);
      }
    }

    // provide the complete content
    data["wbec"]["version"] = cfgWbecVersion;
    data["wbec"]["bldDate"] = cfgBuildDate;
    data["wbec"]["timeNow"] = log_time();
    for (int i = 0; i < cfgCntWb; i++) {
      data["box"][i]["busId"]    = i+1;
      data["box"][i]["version"]  = String(content[i][0], HEX);
      data["box"][i]["chgStat"]  = content[i][1];
      data["box"][i]["currL1"]   = content[i][2];
      data["box"][i]["currL2"]   = content[i][3];
      data["box"][i]["currL3"]   = content[i][4];
      data["box"][i]["pcbTemp"]  = content[i][5];
      data["box"][i]["voltL1"]   = content[i][6];
      data["box"][i]["voltL2"]   = content[i][7];
      data["box"][i]["voltL3"]   = content[i][8];
      data["box"][i]["extLock"]  = content[i][9];
      data["box"][i]["power"]    = content[i][10];
      data["box"][i]["energyP"]   = (float)((uint32_t) content[i][11] << 16 | (uint32_t)content[i][12]) / 1000.0;
      data["box"][i]["energyI"]   = (float)((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 1000.0;
      data["box"][i]["currMax"]  = content[i][15];
      data["box"][i]["currMin"]  = content[i][16];
      data["box"][i]["logStr"]   = getAscii(i, 17,32);
      data["box"][i]["wdTmOut"]  = content[i][49];
      data["box"][i]["standby"]  = content[i][50];
      data["box"][i]["remLock"]  = content[i][51];
      data["box"][i]["currLim"]  = content[i][53];
      data["box"][i]["currFs"]   = content[i][54];
      data["box"][i]["load"]     = lm_getWbLimit(i);
      data["box"][i]["resCode"]  = String(modbusResultCode[i], HEX);
    }
    data["modbus"]["state"]["lastTm"]  = modbusLastTime;
    data["modbus"]["state"]["millis"]  = millis();
    data["wifi"]["mac"] = WiFi.macAddress();
    int qrssi = WiFi.RSSI();
    data["wifi"]["rssi"] = qrssi;
    data["wifi"]["signal"] = getSignalQuality(qrssi);
    data["wifi"]["channel"] = WiFi.channel();

    String response;
    serializeJson(data, response);
    log(m, response);
    request->send(200, "application/json", response);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    // forward to goEmulator
    request->send(200, "application/json", goE_getStatus());
  });

  server.on("/mqtt", HTTP_GET, [](AsyncWebServerRequest *request) {
    // set values
    if (request->hasParam("payload")) {
      log(m, "/mqtt payload: " + request->getParam("payload")->value());
      goE_setPayload(request->getParam("payload")->value());
    }
    // response
    request->send(200, "application/json", goE_getStatus());
  });

  server.on("/phaseCtrl", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("ph")) {
      pc_requestPhase(request->getParam("ph")->value().toInt());
    }
    request->send_P(200, "text/plain", String(pc_getState()).c_str());
  });

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/post-message", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
    Serial.println(response);
  });
  server.addHandler(handler);

  // add the SPIFFSEditor, which can be opened via "/edit"
  server.addHandler(new SPIFFSEditor("" ,"" ,LittleFS));//http_username,http_password));

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);
}

void webServer_handle() {
  if (resetRequested){
    ESP.restart();
  }
}