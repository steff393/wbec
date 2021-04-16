#include <Arduino.h>
#include "globalConfig.h"
#include "mbComm.h"
#include <ModbusRTU.h>
#include "loadManager.h"
#include <SoftwareSerial.h>


// receivePin, transmitPin, inverse_logic, bufSize, isrBufSize
SoftwareSerial S(PIN_RO, PIN_DI);

ModbusRTU mb;

uint16_t  content[WB_CNT][55];

uint32_t  modbusLastTime = 0;
uint32_t  modbusCycleTime = 1000;
uint8_t   modbusResultCode = 0;
uint8_t   msgCnt = 0;
uint8_t   id = 0;
uint16_t  StdByDisable = 4;
uint16_t  writeReg = 0;
uint16_t  writeVal = 0;


bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  modbusResultCode = event;
  Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
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
			mb.writeHreg(1, writeReg, &writeVal,  1, cbWrite); 
			writeReg = 0;
			writeVal = 0;
		}
	}

	if (modbusLastTime == 0 || millis() > modbusLastTime + modbusCycleTime) {
      if (!mb.slave()) {
				switch(msgCnt++) {
					case 0: mb.readIreg(id+1, 4,   &content[id][0] ,  15, cbWrite); break;
					case 1: mb.readIreg(id+1, 100, &content[id][15],  17); break;
					case 2: mb.readIreg(id+1, 117, &content[id][32],  17); break;
					case 3: mb.readHreg(id+1,  REG_WD_TIME_OUT,  &content[id][49],   5); break;
					case 4: mb.writeHreg(id+1, REG_STANDBY_CTRL, &StdByDisable,  1); break;
					default:
						Serial.print("Time:");Serial.println(millis()-modbusLastTime);
						modbusLastTime = millis();
						// 1st trial implementation of a simple loadManager
						lm_setWbState(id, content[id][1], 60, 160);
						lm_updateWbLimits();

						msgCnt = 0;
						if (++id >= WB_CNT) { id = 0;}
						Serial.print("Next BusID: ");Serial.println(id+1);
				}
				//if (modbusResultCode != Modbus::EX_SUCCESS) {
				//	// on error: immediately go-on with next wallbox to avoid timeouts
				//	msgCnt = 250;
				//}
      }
    }
    mb.task();
    yield();
}


void mb_writeReg(uint16_t reg, uint16_t val) {
	writeReg = reg;
	writeVal = val;
}