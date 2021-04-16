// Copyright (c) 2021 steff393

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "globalConfig.h"
#include "loadManager.h"
#include "mbComm.h"
#include "wlan_key.h"


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




bool      _handlingOTA = false;



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



void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting\n");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to WiFi\n");


  // setup the Webserver and Json
  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"message\":\"Welcome\"}");
  });

  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument data(2048);
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
    if (request->hasParam("cycleTm")) {
      uint16_t val = request->getParam("cycleTm")->value().toInt();
      if (val >=500) {
        modbusCycleTime = val;
      }
    }

    // provide the complete content
    data["modbus"]["cfg"]["cycleTm"]  = modbusCycleTime;
    //data["modbus"]["state"]["resCode"]  = String(modbusResultCode[0], HEX);
    data["modbus"]["state"]["lastTm"]  = modbusLastTime;
    data["modbus"]["state"]["millis"]  = millis();

    for (int i = 0; i < WB_CNT; i++) {
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

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);

  server.begin();

  // setup the OTA server
  ArduinoOTA.setHostname(otaHost);
  ArduinoOTA.begin();

  ArduinoOTA.onStart([]() 
  {
    _handlingOTA = true;
  });

  mb_setup();
  Serial.println(millis());
}


void loop() {
  ArduinoOTA.handle();
  if(!_handlingOTA) {
    mb_handle();
  }
}