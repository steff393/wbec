// Copyright (c) 2021 steff393
// based on example from: https://github.com/emelianov/modbus-esp8266

#include <Arduino.h>
#include <ModbusRTU.h>
#include <SoftwareSerial.h>
// receivePin, transmitPin, inverse_logic, bufSize, isrBufSize
// connect RX to NodeMCU D2 (GPIO4), TX to NodeMCU D1 (GPIO5)
SoftwareSerial S(4, 5);

ModbusRTU mb;
long startTime;
long lastRead;
long lastKey;

bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
  return true;
}

void setup() {
  Serial.begin(115200);
  S.begin(19200, SWSERIAL_8E1);       // Wallbox Energy Control uses 19.200 bit/sec, 8 data bit, 1 parity bit (even), 1 stop bit
  mb.begin(&S, 14);                   // GPIO14, NodeMCU pin D5 --> connect to DE & RE
  mb.master();
}

uint16_t result[5];


uint16_t resultA[15];   // Bus-Adr. 4-18
uint16_t resultB[34];   // Bus-Adr. 100-133
uint16_t resultC[4];    // Bus-Adr. 200-203
uint16_t resultD[19];   // Bus-Adr. 300-318
uint16_t resultE[320];  // Bus-Adr. 500-819
uint8_t msgCnt = 0;
char buffer[40];
uint16_t WdTime_Readv = 0;
uint16_t WdTime_Readn = 0;
uint16_t WdTime_Write = 15000;

void loop() {
  
  

  int key = Serial.read();
  switch (key) {
    case '1': Serial.println(key); WdTime_Write = 15000; mb.writeHreg(1, 257, &WdTime_Write,  1, cbWrite); break;
    case '2': Serial.println(key); WdTime_Write = 15002; mb.writeHreg(1, 257, &WdTime_Write,  1, cbWrite); break;
    case '3': Serial.println(key); WdTime_Write = 15003; mb.writeHreg(1, 257, &WdTime_Write,  1, cbWrite); break;
    default: ;
  }

  if (millis()-startTime > 10000) {

    if (!mb.slave()) {
      switch(msgCnt++) {
        case 0: mb.readIreg(1, 4,   resultA, 15, cbWrite); break;
        case 1: mb.readIreg(1, 100, resultB, 25, cbWrite); break;
        case 2: mb.readIreg(1, 125, &resultB[25], 9, cbWrite); break;
        case 3: mb.readIreg(1, 200, resultC,  4, cbWrite); break;
        case 4: mb.readHreg(1, 257, &WdTime_Readv,  1, cbWrite); break;
        //case 5: mb.writeHreg(1, 257, &WdTime_Write,  1, cbWrite); break;
        //case 6: mb.readHreg(1, 257, &WdTime_Readn,  1, cbWrite); break;
        default:
          for (int i = 5; i < 8 ; i++) {
            Serial.print(i);Serial.print(" ");Serial.println(resultA[i]);
          }
          //for (int i = 0; i < 34 ; i++) {
            //Serial.print(i);Serial.print(" ");Serial.printf_P("Resp: %s\n", (char*) resultB);
          //  Serial.print(i);Serial.print(" ");Serial.println(resultB[i]);
          //}
          //Serial.print("Test");Serial.print(" ");Serial.println((char*) resultB);
          //strcpy(buffer, (char *) result);
          //Serial.print("Test");Serial.print(" ");Serial.println(buffer);
          //for (int i = 0; i < 4 ; i++) {
          //  Serial.print(i);Serial.print(" ");Serial.println(resultC[i]);
          //}
          Serial.print("vorher ");Serial.print(" ");Serial.println(WdTime_Readv);
          //Serial.print("Write..");Serial.print(" ");Serial.println(WdTime_Write);
          //Serial.print("nachher");Serial.print(" ");Serial.println(WdTime_Readn);
          Serial.println(millis()-startTime);
          
          startTime = millis();
          msgCnt = 0;
      }

    }

  }
  mb.task();
  yield();
  delay(500);
}