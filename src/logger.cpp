// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <ESP8266mDNS.h>
#include "LittleFS.h"
#include <NTPClient.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000); // GMT+1 and update every minute

const char *mod[6] = {"", "MB  ", "MQTT", "WEBS", "GO-E", "CFG "};
char bootLog[5000];
boolean written = false;

boolean getDstGermany(uint32_t unixtime) {
  
  const uint32_t SEKUNDEN_PRO_TAG   =  86400ul; /*  24* 60 * 60 */
  const uint32_t TAGE_IM_GEMEINJAHR =    365ul; /* kein Schaltjahr */
  const uint32_t TAGE_IN_4_JAHREN   =   1461ul; /*   4*365 +   1 */
  const uint32_t TAGE_IN_100_JAHREN =  36524ul; /* 100*365 +  25 - 1 */
  const uint32_t TAGE_IN_400_JAHREN = 146097ul; /* 400*365 + 100 - 4 + 1 */
  const uint32_t TAGN_AD_1970_01_01 = 719468ul; /* Tagnummer bezogen auf den 1. Maerz des Jahres "Null" */
  uint8_t month;
  uint8_t wday;
  uint8_t hour;
  
  uint16_t day;
  uint32_t dayN;
  uint32_t temp;
  
   // *unixtime += 3600; // +1 für UTC->D
  
  // alle berechnungen nachfolgend
  dayN = (TAGN_AD_1970_01_01 + unixtime / SEKUNDEN_PRO_TAG);

  wday = (uint8_t)(((unixtime / 3600 / 24) + 4) % 7); // weekday
  // Schaltjahrregel des Gregorianischen Kalenders: Jedes durch 100 teilbare Jahr ist kein Schaltjahr, es sei denn, es ist durch 400 teilbar. 
  temp = 4 * (dayN + TAGE_IN_100_JAHREN + 1) / TAGE_IN_400_JAHREN - 1;
  dayN -= TAGE_IN_100_JAHREN * temp + temp / 4;
  
  // Schaltjahrregel des Julianischen Kalenders:
  //   Jedes durch 4 teilbare Jahr ist ein Schaltjahr. 
  temp = 4 * (dayN + TAGE_IM_GEMEINJAHR + 1) / TAGE_IN_4_JAHREN - 1;
  dayN -= TAGE_IM_GEMEINJAHR * temp + temp / 4;
  
  // dayN enthaelt jetzt nur noch die Tage des errechneten Jahres bezogen auf den 1. Maerz. 
  month = (uint8_t)((5 * (uint16_t)dayN + 2) / 153);
  day = (uint16_t)((uint16_t)dayN - (uint16_t)(month * 153 + 2) / 5 + 1);

  hour = (uint8_t)((unixtime % SEKUNDEN_PRO_TAG) / 3600);

  // vom Jahr, das am 1. Maerz beginnt auf unser normales Jahr umrechnen:
  month = (uint8_t)((uint8_t)(month + 3) % 13); 
  
  if( month < 3 || month > 10 )     // month 1, 2, 11, 12
    return 0;         // -> Winter

  if(day - wday >= 25 && (wday || hour >= 2)) { // after last Sunday 2:00
    
    if(month == 10)       // October -> Winter
      return 0;
    
    } else {          // before last Sunday 2:00
    
    if(month == 3)        // March -> Winter
      return 0;
      
    }
    return 1;
    //*unixtime += 3600; // nochmal+1 für Sommerzeit

}

void logger_begin() {
  // connect to NTP time server
  timeClient.begin();
}

void logger_handle() {
	timeClient.update();
	if (getDstGermany(timeClient.getEpochTime())) timeClient.setTimeOffset(7200);
}

void log(uint8_t module, String msg, boolean newLine /* =true */) {
	String output;

  if (module) {
    output = timeClient.getFormattedTime() + ": " + String(mod[module]) + ": ";
	}
  output += msg;
	if (newLine) {
		output += "\n";
	}
  Serial.print(output);

  if (strlen(bootLog)+strlen(output.c_str()) < sizeof(bootLog)-10) {
    strcat(bootLog, output.c_str());
  } 
}

String log_time() {
	return(timeClient.getFormattedTime());
}

char* log_getBuffer() {
  return(bootLog);
}

void log_freeBuffer() {
  // set string-end character to first position to indicate an empty string
  bootLog[0] = '\0';
}