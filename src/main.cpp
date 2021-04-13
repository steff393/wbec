// Copyright (c) 2021 steff393
// based on example from: https://github.com/emelianov/modbus-esp8266

#include <Arduino.h>
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
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
long startTime = 10001;
long lastRead;
long lastKey;

bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
  return true;
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

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);

  server.begin();

  S.begin(19200, SWSERIAL_8E1);       // Wallbox Energy Control uses 19.200 bit/sec, 8 data bit, 1 parity bit (even), 1 stop bit
  mb.begin(&S, 14);                   // GPIO14, NodeMCU pin D5 --> connect to DE & RE
  mb.master();
}


uint8_t msgCnt = 0;
char buffer[40];
uint16_t WdTime_Readv = 0;
uint16_t WdTime_Readn = 0;
uint16_t WdTime_Write = 15000;
uint16_t content[400];
uint16_t StdByDisable = 4;

void loop() {
  
  

  int key = Serial.read();
  switch (key) {
    case '1': Serial.println(key); WdTime_Write = 15000; mb.writeHreg(1, 257, &WdTime_Write,  1, cbWrite); break;
    case '2': Serial.println(key); WdTime_Write = 30000; mb.writeHreg(1, 257, &WdTime_Write,  1, cbWrite); break;
    case '3': Serial.println(key); WdTime_Write = 60000; mb.writeHreg(1, 257, &WdTime_Write,  1, cbWrite); break;
    default: ;
  }

  if (millis()-startTime > 10000) {

    if (!mb.slave()) {
      switch(msgCnt++) {
        case 0: mb.readIreg(1, 4,   &content[0] ,  15, cbWrite); break;
        case 1: mb.readIreg(1, 100, &content[15],  17, cbWrite); break;
        case 2: mb.readIreg(1, 117, &content[32],  17, cbWrite); break;
        case 3: mb.readIreg(1, 200, &content[49],   4, cbWrite); break;
        case 4: mb.readIreg(1, 300, &content[53],  19, cbWrite); break;
        case 5: mb.readIreg(1, 500, &content[72],  25, cbWrite); break;
        case 6: mb.readIreg(1, 525, &content[97],  25, cbWrite); break;
        case 7: mb.readIreg(1, 550, &content[122], 25, cbWrite); break;
        case 8: mb.readIreg(1, 575, &content[147], 25, cbWrite); break;
        case 9: mb.readHreg(1, 257, &content[392],  5, cbWrite); break;
        case 10: mb.writeHreg(1, 258, &StdByDisable,  1, cbWrite); break;
        default:
          for (int i = 5; i < 9 ; i++) {
            Serial.print(i);Serial.print(":");Serial.println(content[i]);
          }
          //Serial.print("vorher ");Serial.print(" ");Serial.println(WdTime_Readv);
          Serial.print("Time:");Serial.println(millis()-startTime);
          
          startTime = millis();
          msgCnt = 0;
      }

    }

  }
  mb.task();
  yield();
}