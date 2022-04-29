// Copyright (c) 2022 andreas.miketta, steff393, MIT license
// based on https://github.com/AMiketta/wbec
#ifndef SOLAREDGE_H
#define SOLAREDGE_H


#define INVERTER_JSON_LEN       256

// SolarEdge (sunspec) registers for modbusTCP
#define INVERTER_SE_PORT        1502
#define INVERTER_SE_ADDRESS     1
#define SMARTMETER_SE_ADDRESS   1
#define REG_SE_I_AC_Current     40071 // 40072 1 I_AC_Current uint16 Amps AC Total Current value
#define REG_SE_I_AC_Power       40083 // modbus register for "AC Power value", int16 in Watts 
#define REG_SE_I_AC_Power_SF    40084 // modbus register for "AC Power scale factor" int16
#define REG_SE_M_AC_Power       40206 // modbus register for "Total Real Power (sum of active phases)" int16 in Watts
#define REG_SE_M_AC_Power_SF    40210 // modbus register for "AC Real Power Scale Factor" int16 SF


// Fronius (sunspec) registers for modbusTCP
#define INVERTER_FR_PORT        502
#define INVERTER_FR_ADDRESS     1
#define SMARTMETER_FR_ADDRESS   240
//#define REG_FR_I_AC_Current   40071 // 40072 1 I_AC_Current uint16 Amps AC Total Current value
#define REG_FR_I_AC_Power       40003 // modbus register for "AC Power value", int16 in Watts 
#define REG_FR_I_AC_Power_SF    40004 // modbus register for "AC Power scale factor" int16
#define REG_FR_M_AC_Power       40006 // modbus register for "Total Real Power (sum of active phases)" int16 in Watts
#define REG_FR_M_AC_Power_SF    40007 // modbus register for "AC Real Power Scale Factor" int16 SF


// Kostal registers for modbusTCP
#define INVERTER_KO_PORT        502
#define INVERTER_KO_ADDRESS     1
#define SMARTMETER_KO_ADDRESS   240
//#define REG_KO_I_AC_Current   40071 // 40072 1 I_AC_Current uint16 Amps AC Total Current value
//#define REG_KO_I_AC_Power     40??? // modbus register for "AC Power value", int16 in Watts 
//#define REG_KO_I_AC_Power_SF  40??? // modbus register for "AC Power scale factor" int16
#define REG_KO_M_AC_Power       40087 // modbus register for "Total Real Power (sum of active phases)" int16 in Watts
#define REG_KO_M_AC_Power_SF    40091 // modbus register for "AC Real Power Scale Factor" int16 SF


extern void     inverter_setup();
extern void     inverter_loop();
extern String   inverter_getStatus();

#endif /* SOLAREDGE_H */
