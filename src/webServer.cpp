// Copyright (c) 2021 steff393

#include <Arduino.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "globalConfig.h"
#include "LittleFS.h"
#include "loadManager.h"
#include "mbComm.h"
#include "SPIFFSEditor.h"
#include "webServer.h"
#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>

AsyncWebServer server(80);


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

void initWebserver() {
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

  server.on("/resetwifi", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String("Resetting the WiFi credentials..."));
    WiFiManager wm;
    wm.resetSettings();
  });

  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument data((cfgCntWb+1)/2  * 1024);  // always 1024 byte for 2 wallboxes
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
    data["modbus"]["state"]["lastTm"]  = modbusLastTime;
    data["modbus"]["state"]["millis"]  = millis();

    for (int i = 0; i < cfgCntWb; i++) {
      data["boxes"][i]["busId"]    = i+1;
      data["boxes"][i]["version"]  = String(content[i][0], HEX);
      data["boxes"][i]["chgStat"]  = content[i][1];
      data["boxes"][i]["currL1"]   = content[i][2];
      data["boxes"][i]["currL2"]   = content[i][3];
      data["boxes"][i]["currL3"]   = content[i][4];
      data["boxes"][i]["pcbTemp"]  = content[i][5];
      data["boxes"][i]["voltL1"]   = content[i][6];
      data["boxes"][i]["voltL2"]   = content[i][7];
      data["boxes"][i]["voltL3"]   = content[i][8];
      data["boxes"][i]["extLock"]  = content[i][9];
      data["boxes"][i]["power"]    = content[i][10];
      data["boxes"][i]["energyP"]   = (float)((uint32_t) content[i][11] << 16 | (uint32_t)content[i][12]) / 1000.0;
      data["boxes"][i]["energyI"]   = (float)((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 1000.0;
      data["boxes"][i]["currMax"]  = content[i][15];
      data["boxes"][i]["currMin"]  = content[i][16];
      data["boxes"][i]["logStr"]   = getAscii(i, 17,32);
      data["boxes"][i]["wdTmOut"]  = content[i][49];
      data["boxes"][i]["standby"]  = content[i][50];
      data["boxes"][i]["remLock"]  = content[i][51];
      data["boxes"][i]["currLim"]  = content[i][53];
      data["boxes"][i]["currFs"]   = content[i][54];
    
      data["load"][i]   = lm_getWbLimit(i+1);
      data["resCode"][i]   = String(modbusResultCode[i], HEX);
    }

    String response;
    serializeJson(data, response);
    Serial.println(response);
    request->send(200, "application/json", response);
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

