// Copyright (c) 2021 steff393
// based on https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/ConfigFile/ConfigFile.ino

#include <ArduinoJson.h>
#include <LittleFS.h>

const char* cfgApSsid;	// SSID of the initial Access Point
const char* cfgApPass;  // Password of the initial Access Point
uint8_t cfgCntWb;		    // number of connected wallboxes in the system
uint8_t cfgMbCycleTime;	// cycle time of the modbus (in seconds)


bool createConfig() {
  StaticJsonDocument<1024> doc;

  // wbec default configuration parameters
  // -------------------------------------
  doc["cfgApSsid"]      = "wbec";
  doc["cfgApPass"]      = "wbec1234"; // older version had "cebw1234"
  doc["cfgCntWb"]       = "1";
  doc["cfgMbCycleTime"] = "3";
  // -------------------------------------

  File configFile = LittleFS.open("/cfg.json", "w");
  if (!configFile) {
    return(false);
  }

  serializeJson(doc, configFile);
  return (true);
}


bool loadConfig() {
  File configFile = LittleFS.open("/cfg.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file... Creating default config...");
    if (createConfig()) {
      Serial.println("Successful!");
      configFile = LittleFS.open("/cfg.json", "r");
    } else {
      Serial.println("Failed to create default config... Please try to erase flash");
      return(false);
    }
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return(false);
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<1024> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  cfgApSsid       = doc["cfgApSsid"];
  cfgApPass       = doc["cfgApPass"];
  cfgCntWb        = doc["cfgCntWb"];
  cfgMbCycleTime  = doc["cfgMbCycleTime"]; 

  Serial.print("\nLoaded cfgApSsid: "); Serial.println(cfgApSsid);
  Serial.print("Loaded cfgApPass: "); Serial.println(cfgApPass);
  Serial.print("Loaded cfgCntWb: "); Serial.println(cfgCntWb);
  return true;
}
