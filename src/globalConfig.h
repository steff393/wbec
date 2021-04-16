#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#define WB_CNT 			  		  3		// equals number of wallboxes in the system (NodeMCU has Bus-ID = 0)
#define REG_WD_TIME_OUT 	257		// modbus register for "ModBus-Master Watchdog Timeout in ms"
#define REG_STANDBY_CTRL	258		// modbus register for "Standby Function Control"
#define REG_CURR_LIMIT		261		// modbus register for "Maximal current command"

#define CURR_ABS_MIN			 60		// absolute possible lower limit for current
#define CURR_ABS_MAX			160		// absolute possible upper limit for current

#define PIN_DI							5		// TX to NodeMCU D1 (GPIO5)
#define PIN_RO							4		// RX to NodeMCU D2 (GPIO4)
#define PIN_DE_RE					 14		// GPIO14, NodeMCU pin D5 --> connect to DE & RE

#endif	/* GLOBALCONFIG_H */
