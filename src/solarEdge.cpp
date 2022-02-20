// Copyright (c) 2022 andreas.miketta, steff393, MIT license
// based on https://github.com/AMiketta/wbec
#include <Arduino.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <globalConfig.h>
#include <IPAddress.h>
#include <ModbusIP_ESP8266.h>
#include <pvAlgo.h>

#define SOLAR_EDGE_PORT        1502
#define SOLAR_EDGE_JSON_LEN     256

static IPAddress remote;   // Address of Modbus Slave device
static ModbusIP  mb;       // Declare ModbusTCP instance

static bool      solarEgdeActive            = false;
static bool      isConnected                = false;
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
#ifdef DEBUG_SOLAREDGE
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


void solarEdge_setup() {
	if (strcmp(cfgSolarEdgeIp, "") != 0) {
		if (remote.fromString(cfgSolarEdgeIp)) {
			mb.client(); // Act as Modbus TCP server
			solarEgdeActive = true;
		}
	}
}


void solarEdge_loop() {
	if ((millis() - lastHandleCall < (uint16_t)cfgPvCycleTime * 1000) ||     // avoid unnecessary frequent calls
			(solarEgdeActive == false)) {
		return;
	}
	lastHandleCall = millis();

	isConnected = mb.isConnected(remote);
	if (!isConnected) {            // Check if connection to Modbus Slave is established
		mb.connect(remote, SOLAR_EDGE_PORT);    // Try to connect if no connection
	} else {  
		mb.readHreg(remote, REG_I_AC_Current,  (uint16 *) &ac_current,           1, cb, 1);  
		mb.readHreg(remote, REG_I_AC_Power,    (uint16 *) &power_inverter,       1, cb, 1);  //Power Inverter
		mb.readHreg(remote, REG_I_AC_Power_SF, (uint16 *) &power_inverter_scale, 1, cb, 1);  //Power Inverter Scale Factor
		mb.readHreg(remote, REG_M_AC_Power,    (uint16 *) &power_meter,          1, cb, 1);  //Power Zähler
		mb.readHreg(remote, REG_M_AC_Power_SF, (uint16 *) &power_meter_scale,    1, cb, 1);  //Power Zähler Scale Factor
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


String solarEdge_getStatus() {
	StaticJsonDocument<SOLAR_EDGE_JSON_LEN> data;
	data[F("solaredge")][F("isConnected")] = String(isConnected);
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
