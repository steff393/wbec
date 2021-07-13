// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "globalConfig.h"
#include "goEmulator.h"
#include "LittleFS.h"
#include "loadManager.h"
#include "logger.h"
#include "mbComm.h"
#include "phaseCtrl.h"
#include "rfid.h"
#include "SPIFFSEditor.h"
#include "webServer.h"
#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>

const uint8_t m = 3;

static const char* otaPage PROGMEM = "%OTARESULT%<br><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

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
  String commState;
  if(var == F("STATE")){
    if(modbusResultCode[0]==0x00){
      commState = F("ON");
    }
    else{
      commState = F("OFF");
    }
    return(commState);
  }
  else if (var == F("TEMPERATURE")){
    float tmp = content[0][5] / 10.0;
    return(String(tmp));
  }
  else if (var == F("VOLTAGE")){
    float tmp = content[0][6] / 10.0;
    return(String(tmp));
  }
  else if (var == F("CURRENT")){
    float tmp = content[0][53] / 10.0;
    return(String(tmp));
  } else return(String(F("notFound")));
}

String otaProcessor(const String& var){
  if(Update.hasError()){
    return(F("Failed"));
  } else {
    return(F("OK"));
  }
}

String otaProcessorEmpty(const String& var){
  // just replace the template string with nothing, neither ok, nor fail
  return String();
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
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, F("text/plain"), String(ESP.getFreeHeap()));
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, F("/index.html"), String(), false, processor);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, F("/style.css"), F("text/css"));
  });

  server.on("/cfg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, F("/cfg.json"), F("application/json"));
  });

  server.on("/bootlog", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, F("text/plain"), log_getBuffer());
  });

  server.on("/bootlog_reset", HTTP_GET, [](AsyncWebServerRequest *request){
    log_freeBuffer();
    request->send(200, F("text/plain"), F("Cleared"));
  });

  server.on("/delete_cfg", HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.remove(F("/cfg.json"))) {
      request->send(200, F("text/plain"), F("cfg.json successfully deleted, resetting defaults at next startup"));
    } else {
      request->send(200, F("text/plain"), F("cfg.json could not be deleted"));
    }
  });

  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam(F("file"))) {
      if (LittleFS.remove(request->getParam(F("file"))->value())) {
        request->send(200, F("text/plain"), F("OK"));
      } else {
        request->send(200, F("text/plain"), F("FAIL"));
      }
    }
  }); 

  server.on("/web", HTTP_GET, [](AsyncWebServerRequest *request){
    uint8_t id = 0;
    if (request->hasParam(F("currLim"))) {
      uint16_t val = request->getParam(F("currLim"))->value().toInt();
      if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
        mb_writeReg(id, REG_CURR_LIMIT, val);
      }
    }
    request->send(LittleFS, F("/index.html"), String(), false, processor);
  });

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request){
    if(modbusResultCode[0]==0x00) {
      request->send(200, F("text/plain"), F("ON"));
    } else {
      request->send(200, F("text/plain"), F("OFF"));
    }
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    float tmp = content[0][5] / 10.0;
    request->send(200, F("text/plain"), String(tmp));
  });
  
  server.on("/voltage", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, F("text/plain"), String(content[0][6]));
  });
  
  server.on("/current", HTTP_GET, [](AsyncWebServerRequest *request){
    float tmp = content[0][53] / 10.0;
    request->send(200, F("text/plain"), String(tmp));
  });

  server.on("/gpio", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!rfid_getEnabled()) {
      if (request->hasParam(F("on"))) {
        digitalWrite(PIN_RST, HIGH);
        request->send(200, F("text/plain"), F("GPIO On"));
      }
      if (request->hasParam(F("off"))) {
        digitalWrite(PIN_RST, LOW);
        request->send(200, F("text/plain"), F("GPIO Off"));
      }    
    } else {
      request->send(200, F("text/plain"), F("RFID active, GPIO not possible"));
    }
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, F("text/plain"), F("Resetting the ESP8266..."));
    resetRequested = true;
  });

  server.on("/resetwifi", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, F("text/plain"), F("Resetting the WiFi credentials..."));
    WiFiManager wm;
    wm.resetSettings();
  });

  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument data((cfgCntWb+2)/3  * 2048);  // always 2048 byte for 3 wallboxes
    uint8_t id = 0;
    // modify values
    if (request->hasParam(F("id"))) {
      id = request->getParam(F("id"))->value().toInt();
    }

    if (request->hasParam(F("wdTmOut"))) {
      mb_writeReg(id, REG_WD_TIME_OUT, request->getParam(F("wdTmOut"))->value().toInt());
    }
    if (request->hasParam(F("remLock"))) {
      uint16_t val = request->getParam(F("remLock"))->value().toInt();
      if (val <= 1) {
        mb_writeReg(id, REG_REMOTE_LOCK, val);
      }
    }
    if (request->hasParam(F("currLim"))) {
      uint16_t val = request->getParam(F("currLim"))->value().toInt();
      if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
        mb_writeReg(id, REG_CURR_LIMIT, val);
      }
    }
    if (request->hasParam(F("currFs"))) {
      uint16_t val = request->getParam(F("currFs"))->value().toInt();
      if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
        mb_writeReg(id, REG_CURR_LIMIT_FS, val);
      }
    }

    // provide the complete content
    data[F("wbec")][F("version")] = cfgWbecVersion;
    data[F("wbec")][F("bldDate")] = cfgBuildDate;
    data[F("wbec")][F("timeNow")] = log_time();
    for (int i = 0; i < cfgCntWb; i++) {
      data[F("box")][i][F("busId")]    = i+1;
      data[F("box")][i][F("version")]  = String(content[i][0], HEX);
      data[F("box")][i][F("chgStat")]  = content[i][1];
      data[F("box")][i][F("currL1")]   = content[i][2];
      data[F("box")][i][F("currL2")]   = content[i][3];
      data[F("box")][i][F("currL3")]   = content[i][4];
      data[F("box")][i][F("pcbTemp")]  = content[i][5];
      data[F("box")][i][F("voltL1")]   = content[i][6];
      data[F("box")][i][F("voltL2")]   = content[i][7];
      data[F("box")][i][F("voltL3")]   = content[i][8];
      data[F("box")][i][F("extLock")]  = content[i][9];
      data[F("box")][i][F("power")]    = content[i][10];
      data[F("box")][i][F("energyP")]   = (float)((uint32_t) content[i][11] << 16 | (uint32_t)content[i][12]) / 1000.0;
      data[F("box")][i][F("energyI")]   = (float)((uint32_t) content[i][13] << 16 | (uint32_t)content[i][14]) / 1000.0;
      data[F("box")][i][F("currMax")]  = content[i][15];
      data[F("box")][i][F("currMin")]  = content[i][16];
      data[F("box")][i][F("logStr")]   = mb_getAscii(i, 17,32);
      data[F("box")][i][F("wdTmOut")]  = content[i][49];
      data[F("box")][i][F("standby")]  = content[i][50];
      data[F("box")][i][F("remLock")]  = content[i][51];
      data[F("box")][i][F("currLim")]  = content[i][53];
      data[F("box")][i][F("currFs")]   = content[i][54];
      data[F("box")][i][F("load")]     = lm_getWbLimit(i);
      data[F("box")][i][F("resCode")]  = String(modbusResultCode[i], HEX);
    }
    data[F("modbus")][F("state")][F("lastTm")]  = modbusLastTime;
    data[F("modbus")][F("state")][F("millis")]  = millis();
    data[F("rfid")][F("enabled")] = rfid_getEnabled();
    data[F("rfid")][F("release")] = rfid_getReleased();
    data[F("rfid")][F("lastId")]  = rfid_getLastID();
    data[F("wifi")][F("mac")] = WiFi.macAddress();
    int qrssi = WiFi.RSSI();
    data[F("wifi")][F("rssi")] = qrssi;
    data[F("wifi")][F("signal")] = getSignalQuality(qrssi);
    data[F("wifi")][F("channel")] = WiFi.channel();
    String response;
    serializeJson(data, response);
    log(m, response);
    request->send(200, F("application/json"), response);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint8_t id = 0;
    boolean fromApp = false;
    if (request->hasParam(F("box"))) {
      fromApp = true;
      id = request->getParam(F("box"))->value().toInt();
      if (id >= WB_CNT) {
        id = 0;
      }
    }
    request->send(200, F("application/json"), goE_getStatus(id, fromApp));
  });

  server.on("/mqtt", HTTP_GET, [](AsyncWebServerRequest *request) {
    // set values
    uint8_t id = 0;
    boolean fromApp = false;
    if (request->hasParam(F("box"))) {
      fromApp = true;
      id = request->getParam(F("box"))->value().toInt();
      if (id >= WB_CNT) {
        id = 0;
      }
    }
    
    if (request->hasParam(F("payload"))) {
      log(m, F("/mqtt payload: ") + request->getParam(F("payload"))->value());
      goE_setPayload(request->getParam(F("payload"))->value(), id);
    }
    // response
    request->send(200, F("application/json"), goE_getStatus(id, fromApp));
  });

  server.on("/phaseCtrl", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam(F("ph"))) {
      pc_requestPhase(request->getParam(F("ph"))->value().toInt());
    }
    request->send(200, F("text/plain"), String(pc_getState()));
  });

  // OTA via http, based on https://gist.github.com/JMishou/60cb762047b735685e8a09cd2eb42a60
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", otaPage, otaProcessorEmpty);
    response->addHeader(F("Connection"), F("close"));
    response->addHeader(F("Access-Control-Allow-Origin"), F("*"));
    request->send(response);
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    // the request handler is triggered after the upload has finished... 
    // create the response, add header, and send response
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", otaPage, otaProcessor);
    response->addHeader(F("Connection"), F("close"));
    response->addHeader(F("Access-Control-Allow-Origin"), F("*"));
    resetRequested = true;  // Tell the main loop to restart the ESP
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    //Upload handler chunks in data
    
    if(!index){ // if index == 0 then this is the first frame of data
      Serial.printf("UploadStart: %s\n", filename.c_str());
      Serial.setDebugOutput(true);
      
      // calculate sketch space required for the update
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if(!Update.begin(maxSketchSpace)){//start with max available size
        Update.printError(Serial);
      }
      Update.runAsync(true); // tell the updaterClass to run in async mode
    }

    //Write chunked data to the free sketch space
    if(Update.write(data, len) != len){
        Update.printError(Serial);
    }
    
    if(final){ // if the final flag is set then this is the last frame of data
      if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u B\nRebooting...\n", index+len);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
    }
  });
  // OTA via http (end)


  // add the SPIFFSEditor, which can be opened via "/edit"
  server.addHandler(new SPIFFSEditor("" ,"" ,LittleFS));//http_username,http_password));

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);

  server.begin();
}

void webServer_handle() {
  if (resetRequested){
    ESP.restart();
  }
}