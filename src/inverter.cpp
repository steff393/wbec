// Copyright (c) 2022 andreas.miketta, steff393, andy5macht, MIT license
// based on https://github.com/AMiketta/wbec and https://github.com/andy5macht/wbec
#include <Arduino.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <globalConfig.h>
#include <inverter.h>
#include <IPAddress.h>
#include <ModbusIP_ESP8266.h>
#include <pvAlgo.h>


static IPAddress remote;   // Address of Modbus Slave device
static ModbusIP  mb;       // Declare ModbusTCP instance

static bool      inverterActive             = false;
static bool      isConnected                = false;

static uint16_t  inverterPort               = 0;
static uint16_t  inverterAddr               = 0;
static uint16_t  smartmetAddr               = 0;
static uint16_t  regAcCurrent               = 0;
static uint16_t  regPowerInv                = 0;
static uint16_t  regPowerInvS               = 0;
static uint16_t  regPowerMet                = 0;
static uint16_t  regPowerMetS               = 0;

static int16_t   power_inverter             = 0;
static int16_t   power_inverter_scale       = 0;
static int16_t   power_meter                = 0; // power (pos. = 'Einspeisung', neg. = 'Bezug')
static int16_t   power_meter_scale          = 0;
static uint16_t  ac_current                 = 0;
static uint16_t  power_house                = 0; 
static uint32_t  lastHandleCall             = 0;


static bool cb(Modbus::ResultCode event, uint16_t transactionId, void *data) {
	if (event != Modbus::EX_SUCCESS) {
		Serial.printf("Modbus result: %02X\n", event);
	}
#ifdef DEBUG_INVERTER
	if (event == Modbus::EX_TIMEOUT) {
		Serial.println("Timeout");
	}
#endif
	return true;
}


static int16_t pow_int16(int16_t base, uint16_t exp) {
	int16_t x = 1;
	for (uint16_t i = 0; i < exp; i++) {
		x = x * base;
	}
	return(x);
}


void inverter_setup() {
	if (strcmp(cfgInverterIp, "") != 0) {
		if (remote.fromString(cfgInverterIp)) {
			mb.client(); // Act as Modbus TCP server
			inverterActive = true;

			switch(cfgInverterType) {   // select the port and addresses based on the different inverter types
				case 1: // SolarEdge (sunspec) registers for modbusTCP
						inverterPort = 1502;
						inverterAddr = 1;
						smartmetAddr = 1;
						regAcCurrent = 40071;   // 40072 1 I_AC_Current uint16 Amps AC Total Current value
						regPowerInv  = 40083;   // modbus register for "AC Power value", int16 in Watts 
						regPowerInvS = 40084;   // modbus register for "AC Power scale factor" int16
						regPowerMet  = 40206;   // modbus register for "Total Real Power (sum of active phases)" int16 in Watts
						regPowerMetS = 40210;   // modbus register for "AC Real Power Scale Factor" int16 SF
						break;
				case 2: // Fronius (sunspec) registers for modbusTCP
						inverterPort = 502;
						inverterAddr = 1;
						smartmetAddr = 240;
						//regAcCurrent = 40071;   // 40072 1 I_AC_Current uint16 Amps AC Total Current value
						regPowerInv  = 40083;   // modbus register for "AC Power value", int16 in Watts 
						regPowerInvS = 40084;   // modbus register for "AC Power scale factor" int16
						regPowerMet  = 40087;   // modbus register for "Total Real Power (sum of active phases)" int16 in Watts
						regPowerMetS = 40091;   // modbus register for "AC Real Power Scale Factor" int16 SF
						break;
				case 3: // Kostal (sunspec) registers for modbusTCP
						inverterPort = 502;
						inverterAddr = 1;
						smartmetAddr = 240;
						//regAcCurrent = 40071;   // 40072 1 I_AC_Current uint16 Amps AC Total Current value
						//regPowerInv  = 40083;   // modbus register for "AC Power value", int16 in Watts 
						//regPowerInvS = 40084;   // modbus register for "AC Power scale factor" int16
						regPowerMet  = 40087;   // modbus register for "Total Real Power (sum of active phases)" int16 in Watts
						regPowerMetS = 40091;   // modbus register for "AC Real Power Scale Factor" int16 SF
						break;
				default: ;// nothing
			}
			// overwrite, if specifically configured by parameter
			if (cfgInverterPort) { inverterPort = cfgInverterPort; }
			if (cfgInverterAddr) { inverterAddr = cfgInverterAddr; }
			if (cfgInvSmartAddr) { smartmetAddr = cfgInvSmartAddr; }
		}
	}
}


void inverter_loop() {
	if ((millis() - lastHandleCall < (uint16_t)cfgPvCycleTime * 1000) ||     // avoid unnecessary frequent calls
			(inverterActive == false)) {
		return;
	}
	lastHandleCall = millis();

	isConnected = mb.isConnected(remote);
	
	if (!isConnected) {            // Check if connection to Modbus Slave is established
		mb.connect(remote, inverterPort);    // Try to connect if no connection
	} else {  
		if (regAcCurrent) { mb.readHreg(remote, regAcCurrent, (uint16_t *) &ac_current,           1, cb, inverterAddr); } 
		if (regPowerInv)  { mb.readHreg(remote, regPowerInv,  (uint16_t *) &power_inverter,       1, cb, inverterAddr); }    //Power Inverter
		if (regPowerInvS) { mb.readHreg(remote, regPowerInvS, (uint16_t *) &power_inverter_scale, 1, cb, inverterAddr); }    //Power Inverter Scale Factor
		if (regPowerMet)  { mb.readHreg(remote, regPowerMet,  (uint16_t *) &power_meter,          1, cb, smartmetAddr); }    //Power Zähler
		if (regPowerMetS) { mb.readHreg(remote, regPowerMetS, (uint16_t *) &power_meter_scale,    1, cb, smartmetAddr); }    //Power Zähler Scale Factor
		if (cfgInverterType == 2) {
			// Fronius Smartmeter provides the value inverted (pos. = 'Bezug', neg. = 'Einspeisung'), see #24
			power_meter = -1 * power_meter;
		}
	}

	mb.task();  // Common local Modbus task

	int16_t pwrInv;
	int16_t pwrMet;

	if (power_inverter_scale < 0) {  // if negative, then divide
		pwrInv = power_inverter / pow_int16(10, (uint16_t)(-power_inverter_scale));
	} else {                         // if positive, then multiply
		pwrInv = power_inverter * pow_int16(10, (uint16_t)  power_inverter_scale);
	}
	if (power_meter_scale < 0) {     // if negative, then divide
		pwrMet = power_meter / pow_int16(10, (uint16_t)(-power_meter_scale));
	} else {                         // if positive, then multiply
		pwrMet = power_meter * pow_int16(10, (uint16_t)  power_meter_scale);
	}
	power_house = pwrInv - pwrMet;

	pv_setWatt(-pwrMet); // pvAlgo expects the value inverted 
}


String inverter_getStatus() {
	StaticJsonDocument<INVERTER_JSON_LEN> data;
	data[F("inverter")][F("isConnected")]  = String(isConnected);
	data[F("power")][F("AC_Total")]        = String(ac_current);
	data[F("power")][F("house")]           = String(power_house);
	data[F("power")][F("inverter")]        = String(power_inverter);
	data[F("power")][F("inverter_scale")]  = String(power_inverter_scale);
	data[F("power")][F("meter")]           = String(power_meter);
	data[F("power")][F("meter_scale")]     = String(power_meter_scale);

  String response;
  serializeJson(data, response);
	return(response);
}
