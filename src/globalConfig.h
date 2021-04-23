// Copyright (c) 2021 steff393

#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#define WB_CNT 			  		  3		// equals number of wallboxes in the system (NodeMCU has Bus-ID = 0)
#define REG_WD_TIME_OUT 	257		// modbus register for "ModBus-Master Watchdog Timeout in ms"
#define REG_STANDBY_CTRL	258		// modbus register for "Standby Function Control"
#define REG_CURR_LIMIT		261		// modbus register for "Maximal current command"

#define CURR_ABS_MIN			 60		// absolute possible lower limit for current
#define CURR_ABS_MAX			160		// absolute possible upper limit for current

#define PIN_DI							5		// GPIO5, NodeMCU pin D1, TX --> connect to DI 
#define PIN_RO							2		// GPIO2, NodeMCU pin D4, RX --> connect to RO 
#define PIN_DE_RE					  4		// GPIO4, NodeMCU pin D2     --> connect to DE & RE


extern const char* cfgApSsid;		// SSID of the initial Access Point
extern const char* cfgApPass; 	// Password of the initial Access Point


extern bool loadConfig();
//extern bool saveConfig();

#endif	/* GLOBALCONFIG_H */
