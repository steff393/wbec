// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include "globalConfig.h"
#include "mbComm.h"
#include "mqtt.h"
#include <ModbusRTU.h>
#include "loadManager.h"
#include <SoftwareSerial.h>


// receivePin, transmitPin, inverse_logic, bufSize, isrBufSize
SoftwareSerial S(PIN_RO, PIN_DI);

ModbusRTU mb;

uint16_t  content[WB_CNT][55];

uint32_t  modbusLastTime = 0;
uint8_t   modbusResultCode[WB_CNT];
uint8_t   msgCnt = 0;
uint8_t   id = 0;
uint8_t   writeId  = 0;
uint16_t  writeReg = 0;
uint16_t  writeVal = 0;


void timeout(uint8_t id) {
	if (cfgStandby == 4) {
		// standby disabled => timeout indicates a failure => reset all
		for (int i =  1; i <= 16; i++) { content[id][i] = 0;	}
		for (int i = 49; i <= 54; i++) { content[id][i] = 0;	}
	} else {
		// standby enabled => timeout is normal, but the following should avoid to consider 'old' values as valid
		for (int i =  2; i <= 12; i++) { content[id][i] = 0;	}
		content[id][53] = 0;
	}
	
}


bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  int id = mb.slave()-1;
	modbusResultCode[id] = event;
	if (event) {
		timeout(id);
	}
  Serial.printf_P("Request result: 0x%02X, BusID: %d\n", event, /*ESP.getFreeHeap()*/ mb.slave());
  return true;
}


void mb_setup() {
	// setup SoftwareSerial and Modbus Master
  S.begin(19200, SWSERIAL_8E1);       // Wallbox Energy Control uses 19.200 bit/sec, 8 data bit, 1 parity bit (even), 1 stop bit
  mb.begin(&S, PIN_DE_RE);
  mb.master();
}


void mb_handle() {
	if (writeReg) {
		if (!mb.slave()) {
			mb.writeHreg(writeId + 1, writeReg, &writeVal,  1, cbWrite); 
			writeId	 = 0;
			writeReg = 0;
			writeVal = 0;
		}
	}

	if (modbusLastTime == 0 || millis() > modbusLastTime + (cfgMbCycleTime*1000)) {
      if (!mb.slave()) {
				//Serial.print(millis());Serial.print(": Sending to BusID: ");Serial.print(id+1);Serial.print(" with msgCnt = ");Serial.println(msgCnt);
				switch(msgCnt) {
					case 0:                              mb.readIreg(id+1,    4,              &content[id][0] ,  15, cbWrite); break;
					case 1: if (!modbusResultCode[id]) { mb.readIreg (id+1, 100,              &content[id][15],  17); } break;
					case 2: if (!modbusResultCode[id]) { mb.readIreg (id+1, 117,              &content[id][32],  17); } break;
					case 3: if (!modbusResultCode[id]) { mb.readHreg (id+1, REG_WD_TIME_OUT,  &content[id][49],   5); } break;
					case 4: if (!modbusResultCode[id]) { mb.writeHreg(id+1, REG_WD_TIME_OUT,  &cfgMbTimeout,      1); } break;
					case 5: if (!modbusResultCode[id]) { mb.writeHreg(id+1, REG_STANDBY_CTRL, &cfgStandby,        1); } break;
					default: ; // do nothing, will be handled below
				}
				id++;
				if (id >= cfgCntWb) {
					id = 0;
					msgCnt++;
				}
				if (msgCnt > 5 || 
				   (msgCnt > 4 && modbusLastTime != 0)) {						// write the REG_WD_TIME_OUT and REG_STANDBY_CTRL only on the very first loop
					msgCnt = 0;
					Serial.print("Time:");Serial.println(millis()-modbusLastTime);
					modbusLastTime = millis();
					// 1st trial implementation of a simple loadManager
					lm_updateWbLimits();
					// publish received data to MQTT
					mqtt_publish(id);
				}
      }
    }
    mb.task();
    yield();
}


void mb_writeReg(uint8_t id, uint16_t reg, uint16_t val) {
	writeId  = id;
	writeReg = reg;
	writeVal = val;
}