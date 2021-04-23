// Copyright (c) 2021 steff393
// based on https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/ConfigFile/ConfigFile.ino

#include <ArduinoJson.h>
#include <LittleFS.h>

const char* cfgApSsid;	// SSID of the initial Access Point
const char* cfgApPass;  // Password of the initial Access Point
uint8_t cfgCntWb;		    // number of connected wallboxes in the system


bool loadConfig() {
  File configFile = LittleFS.open("/cfg.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return(false);
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

  StaticJsonDocument<200> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  cfgApSsid = doc["cfgApSsid"];
  cfgApPass = doc["cfgApPass"];
  cfgCntWb  = doc["cfgCntWb"];

  Serial.print("\nLoaded cfgApSsid: "); Serial.println(cfgApSsid);
  Serial.print("Loaded cfgApPass: "); Serial.println(cfgApPass);
  Serial.print("Loaded cfgCntWb: "); Serial.println(cfgCntWb);
  return true;
}

/*
bool saveConfig() {
  StaticJsonDocument<200> doc;
  doc["cfgApSsid"] = "----";
  doc["cfgApPass"] = "--------";

  File configFile = LittleFS.open("/cfg.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  serializeJson(doc, configFile);
  return true;
}
*/
