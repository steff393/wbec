// Copyright (c) 2021 steff393, MIT license

#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#define WB_CNT 			  		 16		// max. possible number of wallboxes in the system (NodeMCU has Bus-ID = 0)
#define OPENWB_MAX_LP       8   // maximum supported loadpoints by openWB
#define REG_WD_TIME_OUT 	257		// modbus register for "ModBus-Master Watchdog Timeout in ms"
#define REG_STANDBY_CTRL	258		// modbus register for "Standby Function Control"
#define REG_REMOTE_LOCK 	259		// modbus register for "Remote lock (only if extern lock unlocked)"
#define REG_CURR_LIMIT		261		// modbus register for "Maximal current command"

#define CURR_ABS_MIN			 60		// absolute possible lower limit for current
#define CURR_ABS_MAX			160		// absolute possible upper limit for current

#define PIN_DI							5		// GPIO5, NodeMCU pin D1 --> connect to DI (Transmit to Modbus)
#define PIN_RO							2		// GPIO2, NodeMCU pin D4 --> connect to RO (Receive from Modbus)
#define PIN_DE_RE					  4		// GPIO4, NodeMCU pin D2 --> connect to DE & RE

extern char     cfgWbecVersion[];	           // wbec version
extern char     cfgBuildDate[];	          	 // wbec build date

extern char     cfgApSsid[32];	             // SSID of the initial Access Point
extern char     cfgApPass[63];               // Password of the initial Access Point
extern uint8_t  cfgCntWb;									   // number of connected wallboxes in the system
extern uint8_t  cfgMbCycleTime;						   // cycle time of the modbus (in seconds)
extern uint16_t cfgMbDelay;					         // delay time of the modbus before sending new message (in milliseconds)
extern uint16_t cfgMbTimeout;							   // Reg. 257: Modbus timeout (in milliseconds)
extern uint16_t cfgStandby;                  // Reg. 258: Standby Function Control: 0 = enable standby, 4 = disable standby
extern char     cfgMqttIp[16];               // IP address of MQTT broker, "" to disable MQTT
extern uint8_t  cfgMqttLp[WB_CNT];           // Array with assignments to openWB loadpoints, e.g. [4,2,0,1]: Box0 = LP4, Box1 = LP2, Box2 = no MQTT, Box3 = LP1

extern void loadConfig();

#endif	/* GLOBALCONFIG_H */
