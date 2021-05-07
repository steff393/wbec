// Copyright (c) 2021 steff393

#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#define WB_CNT 			  		 16		// max. possible number of wallboxes in the system (NodeMCU has Bus-ID = 0)
#define REG_WD_TIME_OUT 	257		// modbus register for "ModBus-Master Watchdog Timeout in ms"
#define REG_STANDBY_CTRL	258		// modbus register for "Standby Function Control"
#define REG_CURR_LIMIT		261		// modbus register for "Maximal current command"

#define CURR_ABS_MIN			 60		// absolute possible lower limit for current
#define CURR_ABS_MAX			160		// absolute possible upper limit for current

#define PIN_DI							5		// GPIO5, NodeMCU pin D1, TX --> connect to DI 
#define PIN_RO							2		// GPIO2, NodeMCU pin D4, RX --> connect to RO 
#define PIN_DE_RE					  4		// GPIO4, NodeMCU pin D2     --> connect to DE & RE

extern char cfgWbecVersion[7];	          // wbec version
extern char cfgBuildDate[10];	          	// wbec build date
extern char cfgApSsid[32];	              // SSID of the initial Access Point
extern char cfgApPass[63];               	// Password of the initial Access Point
extern uint8_t cfgCntWb;									// number of connected wallboxes in the system
extern uint8_t cfgMbCycleTime;						// cycle time of the modbus (in seconds)
extern uint16_t cfgStandby;               // Standby Function Control: 0 = enable standby, 4 = disable standby

extern bool loadConfig();

#endif	/* GLOBALCONFIG_H */
