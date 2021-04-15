// Copyright (c) 2021 steff393
// based on example from: https://github.com/emelianov/modbus-esp8266

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "loadManager.h"
#include <ModbusRTU.h>
#include <SoftwareSerial.h>
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


// receivePin, transmitPin, inverse_logic, bufSize, isrBufSize
// connect RX to NodeMCU D2 (GPIO4), TX to NodeMCU D1 (GPIO5)
SoftwareSerial S(4, 5);


ModbusRTU mb;
uint32_t  modbusLastTime = 0;
uint32_t  modbusCycleTime = 5000;
uint8_t   modbusResultCode = 0;
bool      _handlingOTA = false;
uint8_t   msgCnt = 0;
uint16_t  writeReg = 0;
uint16_t  writeVal = 0;
uint16_t  content[55];
uint16_t  StdByDisable = 4;

bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  modbusResultCode = event;
  Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
  return true;
}

String getAscii(uint8_t from, uint8_t len) {
  char ch;
  String ret = "";
  for (int i = from; i < (from + len) ; i++) {
    ch = (char) (content[i] & 0x00FF);
    ret += ch;
    ch = (char) (content[i] >> 8);
    ret += ch;
  }
  Serial.println(ret);
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
    StaticJsonDocument<600> data;
    // modify values
    if (request->hasParam("wdTmOut")) {
      writeReg = 257;
      writeVal = request->getParam("wdTmOut")->value().toInt();
    }
    if (request->hasParam("currLim")) {
      uint16_t val = request->getParam("currLim")->value().toInt();
      if (val == 0 || (val >=60 && val <=160)) {
        writeReg = 261;
        writeVal = val;
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
    data["modbus"]["state"]["resCode"]  = String(modbusResultCode, HEX);
    data["modbus"]["state"]["lastTm"]  = modbusLastTime;
    data["modbus"]["state"]["millis"]  = millis();

    data["wbec"][0]["slaveID"]  = 1;
    data["wbec"][0]["version"]  = String(content[0], HEX);
    data["wbec"][0]["chgStat"]  = content[1];
    data["wbec"][0]["currL1"]   = content[2];
    data["wbec"][0]["currL2"]   = content[3];
    data["wbec"][0]["currL3"]   = content[4];
    data["wbec"][0]["pcbTemp"]  = content[5];
    data["wbec"][0]["voltL1"]   = content[6];
    data["wbec"][0]["voltL2"]   = content[7];
    data["wbec"][0]["voltL3"]   = content[8];
    data["wbec"][0]["extLock"]  = content[9];
    data["wbec"][0]["power"]    = content[10];
    data["wbec"][0]["energyP"]   = (float)((uint32_t) content[11] << 16 | (uint32_t)content[12]) / 1000.0;
    data["wbec"][0]["energyI"]   = (float)((uint32_t) content[13] << 16 | (uint32_t)content[14]) / 1000.0;
    data["wbec"][0]["currMax"]  = content[15];
    data["wbec"][0]["currMin"]  = content[16];
    data["wbec"][0]["logStr"]   = getAscii(17,32);
    data["wbec"][0]["wdTmOut"]  = content[49];
    data["wbec"][0]["standby"]  = content[50];
    data["wbec"][0]["remLock"]  = content[51];
    data["wbec"][0]["currLim"]  = content[53];
    data["wbec"][0]["currFs"]   = content[54];

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

  // setup SoftwareSerial and Modbus Master
  S.begin(19200, SWSERIAL_8E1);       // Wallbox Energy Control uses 19.200 bit/sec, 8 data bit, 1 parity bit (even), 1 stop bit
  mb.begin(&S, 14);                   // GPIO14, NodeMCU pin D5 --> connect to DE & RE
  mb.master();
  Serial.println(millis());
}


void loop() {
  ArduinoOTA.handle();
  if(!_handlingOTA) {
    if (modbusLastTime == 0 || millis() > modbusLastTime + modbusCycleTime) {
      if (!mb.slave()) {
        switch(msgCnt++) {
          case 0: mb.readIreg(1, 4,   &content[0] ,  15, cbWrite); break;
          case 1: mb.readIreg(1, 100, &content[15],  17, cbWrite); break;
          case 2: mb.readIreg(1, 117, &content[32],  17, cbWrite); break;
          case 3: mb.readHreg(1, 257, &content[49],  5, cbWrite); break;
          case 4: mb.writeHreg(1, 258, &StdByDisable,  1, cbWrite); break;
          case 5: 
            if (writeReg) {
              mb.writeHreg(1, writeReg, &writeVal,  1, cbWrite); 
              writeReg = 0;
              writeVal = 0;
              break;
            }
          default:
            for (int i = 5; i < 9 ; i++) {
              Serial.print(i);Serial.print(":");Serial.println(content[i]);
            }
            Serial.print("Time:");Serial.println(millis()-modbusLastTime);
            modbusLastTime = millis();

            // 1st trial implementation of a simple loadManager
            lm_setWbState(1, content[1], 60, 160);
            lm_setWbState(2, 6, 60, 160);
            lm_setWbState(3, 0, 60, 160);
            lm_updateWbLimits();
            Serial.print("Allowed Current WB2: ");Serial.println(lm_getWbLimit(2));

            msgCnt = 0;
        }

      }

    }
    mb.task();
    yield();
  }
}