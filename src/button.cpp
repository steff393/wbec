// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <Bounce2.h>
#include <globalConfig.h>
#include <pvAlgo.h>
#include <loadManager.h>
#include <mbComm.h>

#define CYCLE_TIME                1   // ms
#define BOXID                     0		// only 1 box supported


static Bounce2::Button btn_PV_SWITCH = Bounce2::Button();


void btn_setup() {
	if (cfgBtnDebounce > 0) {
		btn_PV_SWITCH.attach(PIN_RST_PV_SWITCH, INPUT_PULLUP); // USE INTERNAL PULL-UP
		btn_PV_SWITCH.interval(cfgBtnDebounce);
	} else {
		pinMode(PIN_RST_PV_SWITCH, OUTPUT);
	}
}


void btn_loop() {
	if (cfgBtnDebounce > 0) {
		btn_PV_SWITCH.update();
		if (btn_PV_SWITCH.fell()) {
			pv_setMode(PV_ACTIVE);
		}
		if (btn_PV_SWITCH.rose()) {
			pv_setMode(PV_OFF);
			lm_storeRequest(BOXID, CURR_ABS_MAX);
		}
	}
}


boolean btn_getState() {
	return(btn_PV_SWITCH.pressed());
}
