// Copyright (c) 2023 steff393, MIT license
// based on https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/ConfigFile/ConfigFile.ino

#include <ArduinoJson.h>
#include <globalConfig.h>
#include <LittleFS.h>
#include <logger.h>

const uint8_t m = 5;

#define WBEC_VER(s) "v" MAJOR_VER_STRING(s) ".4.9"     // token stringification
#define MAJOR_VER_STRING(s) #s                         // .. with two levels of macros

char     cfgWbecVersion[]             = WBEC_VER(WBEC_VERSION_MAJOR); // wbec version
char     cfgBuildDate[]               = __DATE__ " " __TIME__;        // wbec build date

char     cfgApSsid[32];               // SSID of the initial Access Point
char     cfgApPass[63];               // Password of the initial Access Point
uint8_t  cfgCntWb;                    // number of connected wallboxes in the system
uint8_t  cfgMbCycleTime;              // cycle time of the modbus (in seconds)
uint16_t cfgMbDelay;                  // delay time of the modbus before sending new message (in milliseconds)
uint16_t cfgMbTimeout;                // Reg. 257: Modbus timeout (in milliseconds)
uint16_t cfgStandby;                  // Reg. 258: Standby Function Control: 0 = enable standby, 4 = disable standby
uint16_t cfgFailsafeCurrent;          // <don't use - still beta> Reg. 262: Failsafe Current configuration in case of loss of Modbus communication (in 0.1A)
char     cfgMqttIp[16];               // IP address of MQTT broker, "" to disable MQTT
uint16_t cfgMqttPort;                 // Port of MQTT broker (optional)
char     cfgMqttUser[32];             // MQTT: Username
char     cfgMqttPass[32];             // MQTT: Password
uint8_t  cfgMqttLp[WB_CNT];           // Array with assignments to openWB loadpoints, e.g. [4,2,0,1]: Box0 = LP4, Box1 = LP2, Box2 = no MQTT, Box3 = LP1
char     cfgMqttWattTopic[60];        // MQTT: Topic for setting the watt value for PV charging, default: "wbec/pv/setWatt"
char     cfgMqttWattJson[30];         // MQTT: Optional: Element in a JSON string, which contains the power in watt, default: ""
char     cfgNtpServer[30];            // NTP server
char     cfgFoxUser[32];              // powerfox: Username
char     cfgFoxPass[16];              // powerfox: Password
char     cfgFoxDevId[16];             // powerfox: DeviceId
uint8_t  cfgPvActive;                 // PV charging: Active (1) or inactive (0)
uint8_t  cfgPvCycleTime;              // PV charging: cycle time (in seconds)
uint8_t  cfgPvLimStart;               // PV charging: Target current needed for starting (in 0.1A), e.g. 61=6.1A
uint8_t  cfgPvLimStop;                // PV charging: Target current to stop charging when below (in 0.1A)
uint8_t  cfgPvPhFactor;               // PV charging: Power/Current factor, e.g. 69: 1A equals 690W at 3phases, 23: 1A equals 230W at 1phase
uint16_t cfgPvOffset;                 // PV charging: Offset for the available power calculation (in W); can be used to assure that no/less current is consumed from net
uint8_t  cfgPvInvert;                 // PV charging: Invert the watt value (pos./neg.)
uint8_t  cfgPvMinTime;                // PV charging: Minimum activation time (in minutes), 0 to disable
uint16_t cfgTotalCurrMax;             // Total current limit for load management (in 0.1A) - !! Additional fuse mandatory !!
uint8_t  cfgHwVersion;                // Selection of the used HW
uint8_t  cfgWifiSleepMode;            // Set sleep type for power saving, recomendation is 255 (=no influence) or 0 (=WIFI_NONE_SLEEP)
uint8_t  cfgLoopDelay;                // Delay [ms] at end of main loop, might have an impact on web server reactivitiy, default: 255 = inactive
uint16_t cfgKnockOutTimer;            // Interval[min] after which wbec knocks itself out, i.e. triggers a reset, default: 0 = inactive; values < 20min not allowed
char     cfgShellyIp[16];             // IP address of Shelly 3em, "" to disable 
char     cfgInverterIp[16];           // IP address of Inverter, "" to disable 
uint8_t  cfgInverterType;             // 0=off, 1=SolarEdge, 2=Fronius, 3=Kostal
uint16_t cfgInverterPort;             // Overwrite default inverter port setting
uint16_t cfgInverterAddr;             // Overwrite default inverter address setting
uint16_t cfgInvSmartAddr;             // Overwrite default smart meter address setting
uint16_t cfgBootlogSize;              // Size of the bootlog buffer for debugging, max. 5000 [bytes]
uint16_t cfgBtnDebounce;              // Debounce time for button [ms]
uint16_t cfgWifiConnectTimeout;       // Timeout in seconds to connect to Wifi before change to AP-Mode
uint8_t  cfgResetOnTimeout;           // Set (some) Modbus values to 0 after 10x message timeout


static bool createConfig() {
	StaticJsonDocument<128> doc;

	// default configuration parameters
	doc["cfgApPass"]              = F("wbec1234"); // older version had "cebw1234"
	doc["cfgCntWb"]               = 1;
	
	File configFile = LittleFS.open(F("/cfg.json"), "w");
	if (!configFile) {
		return(false);
	}

	serializeJson(doc, configFile);
	configFile.close();
	return (true);
}


static boolean checkConfig(JsonDocument& doc) {
	File configFile = LittleFS.open(F("/cfg.json"), "r");
	if (!configFile) {
		LOG(m, "Failed to open config file... Creating default config...","")
		if (createConfig()) {
			LOG(0, "Successful!", "");
			configFile = LittleFS.open(F("/cfg.json"), "r");
		} else {
			LOG(m, "Failed to create default config... Please try to erase flash","");
			return(false);
		}
	}

	size_t size = configFile.size();
	if (size > 2048) {
		LOG(m, "Config file size is too large","");
		return(false);
	}

	// Allocate a buffer to store contents of the file.
	std::unique_ptr<char[]> buf(new char[size]);

	// We don't use String here because ArduinoJson library requires the input
	// buffer to be mutable. If you don't use ArduinoJson, you may as well
	// use configFile.readString instead.
	configFile.readBytes(buf.get(), size);
	
	auto error = deserializeJson(doc, buf.get());
	if (error) {
		LOG(m, "Failed to parse config file: %s", error.c_str());
		return(false);
	}
	configFile.close();
	
	//configFile = LittleFS.open("/cfg.json", "r");
	//log(m, configFile.readString());
	//configFile.close();
	return(true);
}


void loadConfig() {
	StaticJsonDocument<2048> doc;
	if (!checkConfig(doc)) {
		LOG(m, "Using default config", "");
		deserializeJson(doc, F("{}"));
	}

	strncpy(cfgApSsid,          doc["cfgApSsid"]             | "wbec",             sizeof(cfgApSsid));
	strncpy(cfgApPass,          doc["cfgApPass"]             | "wbec1234",         sizeof(cfgApPass));
	cfgCntWb                  = doc["cfgCntWb"]              | 1;
	cfgMbCycleTime            = doc["cfgMbCycleTime"]        | 10; 
	cfgMbDelay                = doc["cfgMbDelay"]            | 100UL; 
	cfgMbTimeout              = doc["cfgMbTimeout"]          | 60000UL;
	cfgStandby                = doc["cfgStandby"]            | 4UL; 
	cfgFailsafeCurrent        = doc["cfgFailsafeCurrent"]    | 0UL; 
	strncpy(cfgMqttIp,          doc["cfgMqttIp"]             | "",                 sizeof(cfgMqttIp));
	cfgMqttPort               = doc["cfgMqttPort"]           | 1883UL;
	strncpy(cfgMqttUser,        doc["cfgMqttUser"]           | "",                 sizeof(cfgMqttUser));
	strncpy(cfgMqttPass,        doc["cfgMqttPass"]           | "",                 sizeof(cfgMqttPass));
	strncpy(cfgMqttWattTopic,   doc["cfgMqttWattTopic"]      | "wbec/pv/setWatt",  sizeof(cfgMqttWattTopic));
	strncpy(cfgMqttWattJson,    doc["cfgMqttWattJson"]       | "",                 sizeof(cfgMqttWattJson));
	strncpy(cfgNtpServer,       doc["cfgNtpServer"]          | "europe.pool.ntp.org", sizeof(cfgNtpServer));
	strncpy(cfgFoxUser,         doc["cfgFoxUser"]            | "",                 sizeof(cfgFoxUser));
	strncpy(cfgFoxPass,         doc["cfgFoxPass"]            | "",                 sizeof(cfgFoxPass));
	strncpy(cfgFoxDevId,        doc["cfgFoxDevId"]           | "",                 sizeof(cfgFoxDevId));
	cfgPvActive               = doc["cfgPvActive"]           | 0; 
	cfgPvCycleTime            = doc["cfgPvCycleTime"]        | 30; 
	cfgPvLimStart             = doc["cfgPvLimStart"]         | 61; 
	cfgPvLimStop              = doc["cfgPvLimStop"]          | 50; 
	cfgPvPhFactor             = doc["cfgPvPhFactor"]         | 69; 
	cfgPvOffset               = doc["cfgPvOffset"]           | 0UL;
	cfgPvInvert               = doc["cfgPvInvert"]           | 0L;
	cfgPvMinTime              = doc["cfgPvMinTime"]          | 0L;
	cfgTotalCurrMax           = doc["cfgTotalCurrMax"]       | 0UL;
	cfgHwVersion              = doc["cfgHwVersion"]          | 15;
	cfgWifiSleepMode          = doc["cfgWifiSleepMode"]      | 0;
	cfgLoopDelay              = doc["cfgLoopDelay"]          | 255;
	cfgKnockOutTimer          = doc["cfgKnockOutTimer"]      | 0UL;
	strncpy(cfgShellyIp,        doc["cfgShellyIp"]           | "",                 sizeof(cfgShellyIp));
	strncpy(cfgInverterIp,      doc["cfgInverterIp"]         | "",                 sizeof(cfgInverterIp));
	cfgInverterType           = doc["cfgInverterType"]       | 0;
	cfgInverterPort           = doc["cfgInverterPort"]       | 0UL;
	cfgInverterAddr           = doc["cfgInverterAddr"]       | 0UL;
	cfgInvSmartAddr           = doc["cfgInvSmartAddr"]       | 0UL;
	cfgBootlogSize            = doc["cfgBootlogSize"]        | 2000;
	cfgBtnDebounce            = doc["cfgBtnDebounce"]        | 0;
	cfgWifiConnectTimeout     = doc["cfgWifiConnectTimeout"] | 10;
	cfgResetOnTimeout         = doc["cfgResetOnTimeout"]     | 1;
	
	LOG(m, "cfgWbecVersion: %s", cfgWbecVersion);
	LOG(m, "cfgBuildDate: %s"  , cfgBuildDate);
	LOG(m, "cfgCntWb: %d"      , cfgCntWb);

	for (uint8_t i = 0; i < WB_CNT; i++) {
		if (i < doc["cfgMqttLp"].size()) {
			cfgMqttLp[i]            = doc["cfgMqttLp"][i];
		} else {
			cfgMqttLp[i]            = 0;
		}
		if (cfgMqttLp[i] > OPENWB_MAX_LP) {
			cfgMqttLp[i]            = 0;
		}
	}
}
