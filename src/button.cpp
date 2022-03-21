// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include <Bounce2.h>
#include <globalConfig.h>
#include <pvAlgo.h>
#include <loadManager.h>
#include <mbComm.h>

#define CYCLE_TIME                1		// ms
#define BOXID                	  0		// only 1 box supported


static Bounce2::Button btn_PV_SWITCH = Bounce2::Button();

void btn_setup() {
	btn_PV_SWITCH.attach(PIN_RST_PV_SWITCH, INPUT_PULLUP); // USE INTERNAL PULL-UP
	btn_PV_SWITCH.interval(cfgBtnDebounce);
	//btn_PV_SWITCH.setPressedState(LOW);
}

void btn_loop() {
	btn_PV_SWITCH.update();
	if (btn_PV_SWITCH.fell()) {
		pv_setMode(PV_ACTIVE);
		pvAlgo(); // Force pvAlgo => not waiting for pv-loop
	}
	if (btn_PV_SWITCH.rose()) {
		pv_setMode(PV_OFF);
		lm_storeRequest(BOXID, CURR_ABS_MAX);
	}
}
