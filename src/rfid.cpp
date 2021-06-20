// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include "globalConfig.h"
#include <LittleFS.h>
#include "logger.h"
#include <MFRC522.h>
#include <SPI.h>

const uint8_t m = 8;

#define CYCLE_TIME		             500	// 500ms
#define INHIBIT_AFTER_DETECTION   3000  // 3s wait time after a card was detected
#define RFID_CHIP_MAX               10  // different RFID cards
#define RFID_CHIP_LEN                9  // 4 Hex values, e.g. "0ab5c780" + string termination

char      chipID[RFID_CHIP_LEN];
char      chip[RFID_CHIP_MAX][RFID_CHIP_LEN];
uint32_t  rfid_lastCall     = 0;
uint32_t  rfid_lastDetect   = 0;
boolean   rfid_enabled      = false;

MFRC522 mfrc522(PIN_SS, PIN_RST);


boolean readCards() {
  File file = LittleFS.open("/rfid.txt", "r");
  if (!file) {
    log(m, "Disabled (rfid.txt not found)");
    return(false);
  }

  uint8_t k = 0;
  log(m, "Cards: ", false);
  while (file.available() && k < RFID_CHIP_MAX) {
    // read the first characters from each line
    strncpy(chip[k], file.readStringUntil('\n').c_str(), RFID_CHIP_LEN - 1);
    if (k > 0 ) { log(0, ", ", false); }
    log(0, String(chip[k]), false);
    k++;
  }

  file.close();
  return(true);
}


void rfid_setup() {
  if (!readCards()) {
    // there is no rfid.txt file => function disabled
    rfid_enabled = false;
    return;
  }
  rfid_enabled = true;

	SPI.begin();

  // Initialize MFRC522
	mfrc522.PCD_Init();
   
	delay(10);  
  Serial.println("");
	mfrc522.PCD_DumpVersionToSerial();	// dump some details
}


void rfid_loop() {
	if ((millis() - rfid_lastCall < CYCLE_TIME) || (rfid_enabled == false) || (millis() - rfid_lastDetect < INHIBIT_AFTER_DETECTION)) {
		// avoid unnecessary frequent calls
		return;
	}
  rfid_lastCall = millis();

	// Check for new card
	if (mfrc522.PICC_IsNewCardPresent()) {
    // wait a little longer this time to avoid multiple reads
    rfid_lastDetect = millis();

    mfrc522.PICC_ReadCardSerial();

    // First 4 byte should be sufficient
    sprintf(chipID, "%02x%02x%02x%02x", mfrc522.uid.uidByte[0], mfrc522.uid.uidByte[1], mfrc522.uid.uidByte[2], mfrc522.uid.uidByte[3]);
    log(m, "Detected: " + String(chipID) + " ... ", false);
    
    // Search if this fits to a known chip
    uint8_t k = 0;
    while (strncasecmp(chipID, chip[k], RFID_CHIP_LEN - 1) != 0 && k < RFID_CHIP_MAX) {
      k++;
    };
    if (k < RFID_CHIP_MAX) {
      log(0, "found: idx=" + String(k));
    } else {
      log(0, "unknown");
    }
	}
}


char * rfid_getLastID() {
  return(chipID);
}

