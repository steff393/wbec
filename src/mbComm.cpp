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

typedef struct rb_struct {
	uint8_t   id;
	uint16_t reg;
	uint16_t val;
} rb_t;

#define RINGBUF_SIZE 20
rb_t 		rb[RINGBUF_SIZE];			// ring buffer
uint8_t rbIn;									// last element, which was written to ring buffer
uint8_t rbOut;								// last element, which was read from ring buffer

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
  Serial.printf_P("MB  : ResultCode: 0x%02X, BusID: %d\n", event, mb.slave());
  return true;
}


void mb_setup() {
	// setup SoftwareSerial and Modbus Master
  S.begin(19200, SWSERIAL_8E1);       // Wallbox Energy Control uses 19.200 bit/sec, 8 data bit, 1 parity bit (even), 1 stop bit
  mb.begin(&S, PIN_DE_RE);
  mb.master();
}


void mb_handle() {
	// When pointers of the ring buffer are not equal, then there is something to send
	if (rbOut != rbIn) {
		if (!mb.slave()) {			// check, if bus available
			if (++rbOut >= RINGBUF_SIZE) {rbOut = 0;}		// increment pointer, but take care of overflow
			// if box is not in timeout, then send out
			//if (!modbusResultCode[id]) {
				mb.writeHreg(rb[rbOut].id + 1, rb[rbOut].reg, &rb[rbOut].val,  1, cbWrite); 
			//}
		}
	}

	if (modbusLastTime == 0 || millis() > modbusLastTime + (cfgMbCycleTime*1000)) {
      if (!mb.slave()) {
				//Serial.print(millis());Serial.print(": Sending to BusID: ");Serial.print(id+1);Serial.print(" with msgCnt = ");Serial.println(msgCnt);
				switch(msgCnt) {
					case 0:                                                       mb.readIreg (id+1,   4,              &content[id][0] ,  15, cbWrite); break;
					case 1: if (!modbusResultCode[id])                          { mb.readIreg (id+1, 100,              &content[id][15],  17); } break;
					case 2: if (!modbusResultCode[id])                          { mb.readIreg (id+1, 117,              &content[id][32],  17); } break;
					case 3: if (!modbusResultCode[id])                          { mb.readHreg (id+1, REG_WD_TIME_OUT,  &content[id][49],   1); } break;
					case 4: if (!modbusResultCode[id] && content[id][0] > 263)  { mb.readHreg (id+1, REG_STANDBY_CTRL, &content[id][50],   1); } break;	// Can't be read in FW 0x0107 = 263dec
					case 5: if (!modbusResultCode[id])                          { mb.readHreg (id+1, REG_REMOTE_LOCK,  &content[id][51],   1); } break;
					case 6: if (!modbusResultCode[id])                          { mb.readHreg (id+1, REG_CURR_LIMIT,   &content[id][53],   2); } break;
					case 7: if (!modbusResultCode[id])                          { mb.writeHreg(id+1, REG_WD_TIME_OUT,  &cfgMbTimeout,      1); } break;
					case 8: if (!modbusResultCode[id])                          { mb.writeHreg(id+1, REG_STANDBY_CTRL, &cfgStandby,        1); } break;
					default: ; // do nothing, will be handled below
				}
				id++;
				if (id >= cfgCntWb) {
					id = 0;
					msgCnt++;
				}
				if (msgCnt > 8 || 
				   (msgCnt > 7 && modbusLastTime != 0)) {						// write the REG_WD_TIME_OUT and REG_STANDBY_CTRL only on the very first loop
					msgCnt = 0;
					//Serial.print("Time:");Serial.println(millis()-modbusLastTime);
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
	if (++rbIn >= RINGBUF_SIZE) {rbIn = 0;}		// increment pointer, but take care of overflow
	rb[rbIn].id  =  id;
	rb[rbIn].reg = reg;
	rb[rbIn].val = val;
	if (rbIn == rbOut) {
		// we have overwritten an not-sent value -> set rbOut to next element, otherwise complete ring would be skipped
		if (++rbOut >= RINGBUF_SIZE) {rbOut = 0;}		// increment pointer, but take care of overflow
		Serial.println("MB  : Overflow of ring buffer");
	}
}