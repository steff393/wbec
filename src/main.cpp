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
uint8_t cReg = 0;

void loop() {
  startTime = millis();
  if (!mb.slave()) {
    switch(cReg++) {
      case 0:
        mb.readIreg(1, 4,   resultA, 15, cbWrite);
      break;
      case 1:
        mb.readIreg(1, 100, resultB, 25, cbWrite);
      break;
      case 2:
        mb.readIreg(1, 200, resultC,  4, cbWrite);
      break;
      default:
        for (int i = 0; i < 15 ; i++) {
          yield();
          Serial.print(i);Serial.print(" ");Serial.println(resultA[i]);
        }
        for (int i = 0; i < 34 ; i++) {
          yield();
          Serial.print(i);Serial.print(" ");Serial.println(resultB[i]);
        }
        for (int i = 0; i < 4 ; i++) {
          yield();
          Serial.print(i);Serial.print(" ");Serial.println(resultC[i]);
        }
        Serial.println(millis()-startTime);
        
        delay(5000);
        cReg = 0;
    }


    //mb.readIreg(1, 4,   resultA, 15, cbWrite);
    //mb.readIreg(1, 100, resultB, 2, cbWrite);
    //mb.readIreg(1, 200, resultC,  4, cbWrite);
    //mb.readIreg(1, 300, resultD, 19, cbWrite);
    //mb.readIreg(1, 300, resultE,320, cbWrite);

  }
  mb.task();
  //delay(500);
  yield();
}