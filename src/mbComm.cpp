// Copyright (c) 2023 steff393, MIT license

#include <Arduino.h>
#include "globalConfig.h"
#include "logger.h"
#include "mbComm.h"
#include "mqtt.h"
#include <ModbusRTU.h>
#include "loadManager.h"
#include "phaseCtrl.h"
#include <SoftwareSerial.h>

#define RINGBUF_SIZE 20

const uint8_t m = 1;


typedef struct rb_struct {
	uint8_t   id;       // box id (0..WB_CNT)
	uint16_t reg;       // register
	uint16_t val;       // value
	uint16_t * buf;     // write: null  read: buffer where to write the response
} rb_t;


static uint16_t  content[WB_CNT][55];
uint32_t         modbusLastTime = 0;
uint8_t          modbusResultCode[WB_CNT];

static SoftwareSerial S;
static ModbusRTU mb;
static uint32_t  modbusLastMsgSentTime = 0;
static uint8_t   modbusFailureCnt[WB_CNT];
static uint8_t   msgCnt = 0;
static uint8_t   id = 0;
static uint8_t   msgCnt0_lastId = 255;
static rb_t      rb[RINGBUF_SIZE];    // ring buffer
static uint8_t   rbIn  = 0;           // last element, which was written to ring buffer
static uint8_t   rbOut = 0;           // last element, which was read from ring buffer

static boolean mb_available() {
	// don't allow new msg, when communication is still active (ca.30ms) or minimum delay time not exceeded
	if (mb.slave() || millis() - modbusLastMsgSentTime < cfgMbDelay) {
		return(false);
	} else {
		return(true);
	}
}


void mb_getAscii(uint8_t id, uint8_t from, uint8_t len, char *result) {
	// translate the uint16 values into a String
	for (int i = from; i < (from + len) ; i++) {
		result[(i-from)*2]   = (char) (content[id][i] & 0x00FF);
		result[(i-from)*2+1] = (char) (content[id][i] >> 8);
	}
	result[len*2]='\0';
}


static void timeout(uint8_t id) {
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


static bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
	int id = mb.slave()-1;
	modbusResultCode[id] = event;
	if (event) {
		LOG(m, "Comm-Failure BusID %d", mb.slave());
		if (modbusFailureCnt[id] < 250) {
			modbusFailureCnt[id]++;
		}
		if (modbusFailureCnt[id] == 10) {
			// too many consecutive timeouts --> reset values
			LOG(m, "Timeout BusID %d", mb.slave());
			timeout(id);
		}
	} else
	{
		// no failure
		modbusFailureCnt[id] = 0;
		// tell load manager that the current register was successfully read
		if (msgCnt == 6+1) {
			lm_currentReadSuccess(id);
		}
	}
	
	//log(m, "ResultCode: 0x" + String(event, HEX) + ", BusID: "+ mb.slave());
	return(true);
}


void mb_setup() {
	// setup SoftwareSerial and Modbus Master
	LOG(m, "HwVersion: %d", cfgHwVersion);
	if (cfgHwVersion == 10) {
		S.begin(19200, SWSERIAL_8E1, PIN_DI, PIN_RO); // inverted
	} else {
		S.begin(19200, SWSERIAL_8E1, PIN_RO, PIN_DI); // Wallbox Energy Control uses 19.200 bit/sec, 8 data bit, 1 parity bit (even), 1 stop bit
	}
	mb.begin(&S, PIN_DE_RE);
	mb.master();
}


void mb_loop() {
	// When pointers of the ring buffer are not equal, then there is something to send
	if (rbOut != rbIn) {
		if (mb_available()) {			// check, if bus available
			rbOut = (rbOut+1) % RINGBUF_SIZE; 		// increment pointer, but take care of overflow
			if (rb[rbOut].buf != NULL) {
				mb.readHreg (rb[rbOut].id + 1, rb[rbOut].reg,  rb[rbOut].buf, 1, cbWrite);
			} else {
				mb.writeHreg(rb[rbOut].id + 1, rb[rbOut].reg, &rb[rbOut].val, 1, cbWrite); 
			}
			modbusLastMsgSentTime = millis();
		}
	}

	if (modbusLastTime == 0 || millis() - modbusLastTime > (cfgMbCycleTime*1000)) {
			if (mb_available()) {
				//Serial.print(millis());Serial.print(": Sending to BusID: ");Serial.print(id+1);Serial.print(" with msgCnt = ");Serial.println(msgCnt);
				if (msgCnt0_lastId != 255) {
					// msgCnt=0 was recently sent => content is updated => publish to MQTT
					mqtt_publish(msgCnt0_lastId);
					msgCnt0_lastId = 255;
				}
				if (!modbusResultCode[id]) {
					//log(m, String(millis()) + ": BusID=" + (id+1) + ",msgCnt=" + msgCnt);
				}
				switch(msgCnt) {
					case 0:                                                       mb.readIreg (id+1,   4,              &content[id][0] ,  15, cbWrite); msgCnt0_lastId = id; break;
					case 1: if (!modbusResultCode[id])                          { mb.readIreg (id+1, 100,              &content[id][15],  17, cbWrite); } break;
					case 2: if (!modbusResultCode[id])                          { mb.readIreg (id+1, 117,              &content[id][32],  17, cbWrite); } break;
					case 3: if (!modbusResultCode[id])                          { mb.readHreg (id+1, REG_WD_TIME_OUT,  &content[id][49],   1, cbWrite); } break;
					case 4: if (!modbusResultCode[id] && content[id][0] > 263)  { mb.readHreg (id+1, REG_STANDBY_CTRL, &content[id][50],   1, cbWrite); } break;	// Can't be read in FW 0x0107 = 263dec
					case 5: if (!modbusResultCode[id] && content[id][0] > 263)  { mb.readHreg (id+1, REG_REMOTE_LOCK,  &content[id][51],   1, cbWrite); } break;	// Can't be read in FW 0x0107 = 263dec
					case 6: if (!modbusResultCode[id])                          { mb.readHreg (id+1, REG_CURR_LIMIT,   &content[id][53],   2, cbWrite); } break;
					case 7: if (!modbusResultCode[id])                          { mb.writeHreg(id+1, REG_WD_TIME_OUT,  &cfgMbTimeout,      1, cbWrite); } break;
					case 8: if (!modbusResultCode[id])                          { mb.writeHreg(id+1, REG_STANDBY_CTRL, &cfgStandby,        1, cbWrite); } break;
					case 9: if (!modbusResultCode[id])                          { mb.writeHreg(id+1, REG_CURR_LIMIT_FS,&cfgFailsafeCurrent,1, cbWrite); } break;
					default: ; // do nothing, should not happen
				}
				modbusLastMsgSentTime = millis();
				id++;
				if (id >= cfgCntWb) {
					id = 0;
					msgCnt++;
				}
				if (msgCnt > 9 || 
					 (msgCnt > 6 && modbusLastTime != 0)) {						// write the REG_WD_TIME_OUT and REG_STANDBY_CTRL and REG_CURR_LIMIT_FS only on the very first loop
					msgCnt = 0;
					//Serial.print("Time:");Serial.println(millis()-modbusLastTime);
					modbusLastTime = millis();
				}
			}
		}
		mb.task();
		yield();
}


void mb_writeReg(uint8_t id, uint16_t reg, uint16_t val) {
	if (pc_switchInProgress() && id == 0 && reg == REG_CURR_LIMIT) {
		// when switching of phases is in progress, then just backup the requested current
		pc_backupRequest(val);
		return;
	}
	rbIn = (rbIn+1) % RINGBUF_SIZE; 		// increment pointer, but take care of overflow
	rb[rbIn].id  =  id;
	rb[rbIn].reg = reg;
	rb[rbIn].val = val;
	rb[rbIn].buf = 0;
	if (rbIn == rbOut) {
		// we have overwritten an not-sent value -> set rbOut to next element, otherwise complete ring would be skipped
		rbOut = (rbOut+1) % RINGBUF_SIZE; 		// increment pointer, but take care of overflow
		LOG(m, "Overflow of ring buffer", "");
	}

	// direct read back, when current register was modified
	if (reg == REG_CURR_LIMIT && ((rbIn+1) % RINGBUF_SIZE != rbOut)) {	// ... but reading is not worth an overflow (with loosing data)
		rbIn = (rbIn+1) % RINGBUF_SIZE; 		// increment pointer, but take care of overflow
		rb[rbIn].id  =  id;
		rb[rbIn].reg = reg;
		rb[rbIn].val = 0;
		rb[rbIn].buf = &content[id][53];
	}
}


bool mb_isCarConnected(uint8_t id) {
	return content[id][1] >= 4 && content[id][1] <= 7;
}


uint16_t mb_version(uint8_t id) {
	return content[id][0];
}


uint16_t mb_status(uint8_t id) {
	return content[id][1];
}


uint16_t mb_temperature(uint8_t id) {
	return content[id][5];
}


uint16_t mb_extLock(uint8_t id) {
	return content[id][9];
}


uint16_t mb_power(uint8_t id) {
	return content[id][10];
}


uint16_t mb_amperageLimit(uint8_t id) {
	return content[id][53];
}


uint16_t mb_amperageMaximum(uint8_t id) {
	return content[id][15];
}


uint16_t mb_amperageMinimum(uint8_t id) {
	return content[id][16];
}


uint16_t mb_chargingRequested(uint8_t id) {
	// check charging state, as a binary number
	if (content[id][1] == 6 || content[id][1] == 7) {       // C1 or C2
		return(1 << id);
	} else {
		return(0);
	}
}


uint32_t mb_energyTotal(uint8_t id) {
	return ((uint32_t)content[id][13] << 16 | (uint32_t)content[id][14]);
}


uint32_t mb_energyPowerOn(uint8_t id) {
	return ((uint32_t)content[id][11] << 16 | (uint32_t)content[id][12]);
}


const std::array<uint16_t,3>& mb_voltages(uint8_t id) {
	return reinterpret_cast<const std::array<uint16_t,3>&>(content[id][6]);
}


const std::array<uint16_t,3>& mb_amperages(uint8_t id) {
	return reinterpret_cast<const std::array<uint16_t,3>&>(content[id][2]);
}


uint16_t mb_watchdogTimeout(uint8_t id) {
	return content[id][49];
}


uint16_t mb_standby(uint8_t id) {
	return content[id][50];
}


uint16_t mb_remLock(uint8_t id) {
	return content[id][51];
}


uint16_t mb_amperageFailsafe(uint8_t id) {
	return content[id][54];
}
