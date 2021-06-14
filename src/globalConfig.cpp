// Copyright (c) 2021 steff393, MIT license
// based on https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/ConfigFile/ConfigFile.ino

#include <ArduinoJson.h>
#include "globalConfig.h"
#include <LittleFS.h>
#include "logger.h"

const uint8_t m = 5;

char cfgWbecVersion[]     = "v0.3.0+";          // wbec version
char cfgBuildDate[]       = "2021-06-14";	     // wbec build date

char     cfgApSsid[32];	              // SSID of the initial Access Point
char     cfgApPass[63];               // Password of the initial Access Point
uint8_t  cfgCntWb;		                // number of connected wallboxes in the system
uint8_t  cfgMbCycleTime;	            // cycle time of the modbus (in seconds)
uint16_t cfgMbDelay;					        // delay time of the modbus before sending new message (in milliseconds)
uint16_t cfgMbTimeout;					      // Reg. 257: Modbus timeout (in milliseconds)
uint16_t cfgStandby;                  // Reg. 258: Standby Function Control: 0 = enable standby, 4 = disable standby
char     cfgMqttIp[16];              	// IP address of MQTT broker, "" to disable MQTT
char     cfgMqttUser[32];             // MQTT: Username
char     cfgMqttPass[32];             // MQTT: Password
uint8_t  cfgMqttLp[WB_CNT];           // Array with assignments to openWB loadpoints, e.g. [4,2,0,1]: Box0 = LP4, Box1 = LP2, Box2 = no MQTT, Box3 = LP1
char     cfgNtpServer[30];            // NTP server


bool createConfig() {
  StaticJsonDocument<1024> doc;

  // wbec default configuration parameters
  // -------------------------------------
  doc["cfgApSsid"]              = "wbec";
  doc["cfgApPass"]              = "wbec1234"; // older version had "cebw1234"
  doc["cfgCntWb"]               = 1;
  doc["cfgMbCycleTime"]         = 10;
  doc["cfgMbDelay"]             = 100;
  doc["cfgMbTimeout"]           = 60000;  
  doc["cfgStandby"]             = 4;
  doc["cfgMqttIp"]              = "";
  doc["cfgMqttUser"]            = "";
  doc["cfgMqttPass"]            = "";
  doc["cfgMqttLp"]              = serialized("[]");   // already serialized
  doc["cfgNtpServer"]           = "europe.pool.ntp.org";
  // -------------------------------------
  
  File configFile = LittleFS.open("/cfg.json", "w");
  if (!configFile) {
    return(false);
  }

  serializeJson(doc, configFile);
  configFile.close();
  return (true);
}


boolean checkConfig(JsonDocument& doc) {
  File configFile = LittleFS.open("/cfg.json", "r");
  if (!configFile) {
    log(m, "Failed to open config file... Creating default config...");
    if (createConfig()) {
      Serial.println("Successful!");
      log(0, "Successful!");
      configFile = LittleFS.open("/cfg.json", "r");
    } else {
      log(m, "Failed to create default config... Please try to erase flash");
      return(false);
    }
  }

  size_t size = configFile.size();
  if (size > 1024) {
    log(m, "Config file size is too large");
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
    log(m, "Failed to parse config file: " + String(error.c_str()));
    return(false);
  }
  configFile.close();

  //configFile = LittleFS.open("/cfg.json", "r");
  //log(m, configFile.readString());
  //configFile.close();
  return(true);
}

void loadConfig() {
  StaticJsonDocument<1024> doc;
  if (!checkConfig(doc)) {
    log(m, "Using default config");
    deserializeJson(doc, "{}");
  }

  strncpy(cfgApSsid,          doc["cfgApSsid"]            | "wbec",             sizeof(cfgApSsid));
  strncpy(cfgApPass,          doc["cfgApPass"]            | "wbec1234",         sizeof(cfgApPass));
  cfgCntWb                  = doc["cfgCntWb"]             | 1;
  cfgMbCycleTime            = doc["cfgMbCycleTime"]       | 10; 
  cfgMbDelay                = doc["cfgMbDelay"]           | 100UL; 
  cfgMbTimeout              = doc["cfgMbTimeout"]         | 60000UL;
  cfgStandby                = doc["cfgStandby"]           | 4UL; 
  strncpy(cfgMqttIp,          doc["cfgMqttIp"]            | "",                 sizeof(cfgMqttIp));
  strncpy(cfgMqttUser,        doc["cfgMqttUser"]          | "",                 sizeof(cfgMqttUser));
  strncpy(cfgMqttPass,        doc["cfgMqttPass"]          | "",                 sizeof(cfgMqttPass));
  strncpy(cfgNtpServer,       doc["cfgNtpServer"]         | "europe.pool.ntp.org", sizeof(cfgNtpServer));

  log(m, "cfgWbecVersion: " + String(cfgWbecVersion));
  log(m, "cfgBuildDate: "   + String(cfgBuildDate));
  log(m, "cfgCntWb: "       + String(cfgCntWb));

  for (uint8_t i = 0; i < WB_CNT; i++) {
    if (i < doc["cfgMqttLp"].size()) {
      cfgMqttLp[i]            = doc["cfgMqttLp"][i];
    } else {
      cfgMqttLp[i]            = 0;
    }
    if (cfgMqttLp[i] > OPENWB_MAX_LP) {
      cfgMqttLp[i]            = 0;
    }
    //log(m, "cfgMqttLp[" + String(i) + "]: " + cfgMqttLp[i]); 
  }
}
