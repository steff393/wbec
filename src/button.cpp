// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <Bounce2.h>
#include <globalConfig.h>
#include <pvAlgo.h>
#include <rfid.h>
#include <loadManager.h>
#include <mbComm.h>

#define POWER_LOSS_DELAY       3000   // ms
#define BOXID                     0		// only 1 box supported


static Bounce2::Button btn_PV_SWITCH = Bounce2::Button();
static uint32_t powerLossTimer = 0;

void btn_setup() {
	if (rfid_getEnabled()) {
		; // if RFID is enabled, then it's not possible to use the GPIOs for other purposes
	} else {
		// if RFID is not active, then PV_SWITCH can be used to change the PV mode
		if (cfgBtnDebounce > 0) {
			btn_PV_SWITCH.attach(PIN_PV_SWITCH, INPUT_PULLUP); // USE INTERNAL PULL-UP
			btn_PV_SWITCH.interval(cfgBtnDebounce);
			btn_PV_SWITCH.setPressedState(LOW);
			btn_PV_SWITCH.update();
			if (btn_PV_SWITCH.isPressed()) {
				pv_setMode(PV_ACTIVE);
			} else {
				powerLossTimer = millis(); // if Power loss: Wallbox is booting ~2s slower than WBEC: "lm_storeRequest" would be lost
			}
		}
		// and RST can be used as an output
		pinMode(PIN_RST, OUTPUT);
	}
	
}


void btn_loop() {
	if (cfgBtnDebounce > 0) {
		btn_PV_SWITCH.update();
		if (btn_PV_SWITCH.fell()) {
			pv_setMode(PV_ACTIVE);
		}
		if (btn_PV_SWITCH.rose() || 
		  ((powerLossTimer > 0) && (millis() - powerLossTimer > POWER_LOSS_DELAY))) {
			pv_setMode(PV_OFF);
			lm_storeRequest(BOXID, CURR_ABS_MAX);
			powerLossTimer = 0;  // reset the timer, when it was once used
		}
	}
}


boolean btn_getState() {
	return(btn_PV_SWITCH.isPressed());
}
