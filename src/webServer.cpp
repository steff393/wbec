// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <AsyncElegantOTA.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <button.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <globalConfig.h>
#include <goEmulator.h>
#include <LittleFS.h>
#include <loadManager.h>
#include <logger.h>
#include <mbComm.h>
#include <phaseCtrl.h>
#include <powerfox.h>
#include <pvAlgo.h>
#include <rfid.h>
#include <solarEdge.h>
#include <SPIFFSEditor.h>
#include <webServer.h>
#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>

#define PFOX_JSON_LEN 256
#define GPIO_JSON_LEN  32

static const uint8_t m = 3;


static AsyncWebServer server(80);
static boolean resetRequested = false;
static boolean resetwifiRequested = false;


static void onRequest(AsyncWebServerRequest *request){
  //Handle Unknown Request
  request->send_P(404, PSTR("text/plain"), PSTR("Not found"));
}


static void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  //Handle body
}


static void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  //Handle upload
}


static String processor(const String& var){
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


static uint8_t getSignalQuality(int rssi)
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


void webServer_setup() {
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, F("text/plain"), String(ESP.getFreeHeap()));
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, F("/web.html"), F("text/html"));
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

  server.on("/web", HTTP_GET, [](AsyncWebServerRequest *request){
    uint8_t id = 0;
    if (request->hasParam(F("currLim"))) {
      uint16_t val = request->getParam(F("currLim"))->value().toInt();
      if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
        lm_storeRequest(id, val);
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
    StaticJsonDocument<GPIO_JSON_LEN> data;

    if ((!rfid_getEnabled()) && (cfgBtnDebounce==0)) {
      // Set GPIO as an OUTPUT
      if (request->hasParam(F("on"))) {
        digitalWrite(PIN_RST_PV_SWITCH, HIGH);
      }
      if (request->hasParam(F("off"))) {
        digitalWrite(PIN_RST_PV_SWITCH, LOW);
      }
			data[F("D3")] = digitalRead(PIN_RST_PV_SWITCH);
    } else {    
			// Read GPIO as an INPUT
			data[F("D3")] = btn_getState() ? 1 : 0;
    }
		char response[GPIO_JSON_LEN];
    serializeJson(data, response, GPIO_JSON_LEN);
    request->send(200, F("application/json"), response);
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, F("text/plain"), F("Resetting the ESP8266..."));
    resetRequested = true;
  });

	server.on("/resetwifi", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(200, F("text/plain"), F("Resetting the WiFi credentials... Please power off/on"));
		resetwifiRequested = true;
	});

  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint8_t  id       = 0;
    uint8_t  from     = 0;        // used in 'for loop'
    uint8_t  to       = cfgCntWb; // used in 'for loop'
    uint16_t jsonSize = (cfgCntWb+2)/3 * 2048;  // always 2048 byte for 3 wallboxes
    // modify values
    if (request->hasParam(F("id"))) {
      id       = request->getParam(F("id"))->value().toInt();
      from     = id;      // if id is provided, then only
      to       = id+1;    // those values are returned (-> save RAM)
      jsonSize = 2048;    // for one wallbox 2048 are sufficient         
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
        lm_storeRequest(id, val);
      }
    }
    if (request->hasParam(F("currFs"))) {
      uint16_t val = request->getParam(F("currFs"))->value().toInt();
      if (val == 0 || (val >= CURR_ABS_MIN && val <= CURR_ABS_MAX)) {
        mb_writeReg(id, REG_CURR_LIMIT_FS, val);
      }
    }
    if (request->hasParam(F("pvMode"))) {
      pvMode_t val = (pvMode_t) request->getParam(F("pvMode"))->value().toInt();
      if (val <= PV_MIN_PV) {
        pv_setMode(val);
      }
    }
    if (request->hasParam(F("pvWatt"))) {
      pv_setWatt(request->getParam(F("pvWatt"))->value().toInt());
    }

    DynamicJsonDocument data(jsonSize);    
    // provide the complete content
    data[F("wbec")][F("version")] = cfgWbecVersion;
    data[F("wbec")][F("bldDate")] = cfgBuildDate;
    data[F("wbec")][F("timeNow")] = log_time();
    for (int i = from; i < to; i++) {
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
      char txt[65]; mb_getAscii(i, 17, 32, txt);
      data[F("box")][i][F("logStr")]   = txt;
      data[F("box")][i][F("wdTmOut")]  = content[i][49];
      data[F("box")][i][F("standby")]  = content[i][50];
      data[F("box")][i][F("remLock")]  = content[i][51];
      data[F("box")][i][F("currLim")]  = content[i][53];
      data[F("box")][i][F("currFs")]   = content[i][54];
      data[F("box")][i][F("lmReq")]    = lm_getLastRequest(i);
      data[F("box")][i][F("lmLim")]    = lm_getWbLimit(i);
      data[F("box")][i][F("resCode")]  = String(modbusResultCode[i], HEX);
    }
    data[F("modbus")][F("state")][F("lastTm")]  = modbusLastTime;
    data[F("modbus")][F("state")][F("millis")]  = millis();
    data[F("rfid")][F("enabled")]      = rfid_getEnabled();
    data[F("rfid")][F("release")]      = rfid_getReleased();
    data[F("rfid")][F("lastId")]       = rfid_getLastID();
    data[F("pv")][F("mode")]           = pv_getMode();
    data[F("pv")][F("watt")]           = pv_getWatt();
    data[F("wifi")][F("mac")]          = WiFi.macAddress();
    int qrssi = WiFi.RSSI();     
    data[F("wifi")][F("rssi")]         = qrssi;
    data[F("wifi")][F("signal")]       = getSignalQuality(qrssi);
    data[F("wifi")][F("channel")]      = WiFi.channel();
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

  server.on("/pfox", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<PFOX_JSON_LEN> data;
    uint8_t id = 0;
    // modify values
    if (request->hasParam(F("pvMode"))) {
      pvMode_t val = (pvMode_t) request->getParam(F("pvMode"))->value().toInt();
      if (val <= PV_MIN_PV) {
        pv_setMode(val);
      }
    }
    if (request->hasParam(F("pvWatt"))) {
      pv_setWatt(request->getParam(F("pvWatt"))->value().toInt());
    }

    data[F("box")][F("chgStat")]  = content[id][1];
    data[F("box")][F("power")]    = content[id][10];
    data[F("box")][F("currLim")]  = content[id][53];
    data[F("box")][F("resCode")]  = String(modbusResultCode[id], HEX);
    data[F("modbus")][F("millis")]  = millis();
    data[F("pv")][F("mode")]    = pv_getMode();
    data[F("pv")][F("watt")]    = pv_getWatt();
    char response[PFOX_JSON_LEN];
    serializeJson(data, response, PFOX_JSON_LEN);
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


  server.on("/solaredge", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, F("application/json"), solarEdge_getStatus());
  });


  // add the SPIFFSEditor, which can be opened via "/edit"
  server.addHandler(new SPIFFSEditor("" ,"" ,LittleFS));//http_username,http_password));

  server.serveStatic("/", LittleFS, "/");

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  
  server.begin();
}

void webServer_loop() {
  if (resetRequested){
    ESP.restart();
  }
	if (resetwifiRequested) {
		WiFi.disconnect(true);
		ESP.eraseConfig();
		resetwifiRequested = false;
	}
}
