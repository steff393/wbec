// Copyright (c) 2023 steff393, MIT license

import {initNavBar, assignValuesToHtml, setSectionVisibility} from './common.js';

window.addEventListener('DOMContentLoaded', () => {
	// Adjustable values -----
	let phases         = 3;   // number of phases
	// -----------------------
	let Socket;
	let elementCurrentSlider   = document.getElementById('slideCurr');
	let pvModeButtons          = document.querySelectorAll('[data-pv-mode]');
	let wallboxButtons         = document.querySelectorAll('[data-wallbox-id]');
	let valueContainerElements = document.querySelectorAll('[data-value]');

	let sliderSliding = false;

	function init() {
		setSectionVisibility('connection', false);
		setSectionVisibility('boxSelection', wallboxButtons.length > 1);
		setSectionVisibility('pvLaden', false);
		initNavBar();
		document.getElementById('btnExit').addEventListener('click', exit);

		Socket = new WebSocket(`ws://${window.location.hostname}:81/`);
		Socket.onmessage = processReceivedCommand;

		// Update the current slider value (each time you drag the slider handle)
		elementCurrentSlider.addEventListener('input', onStartSliderSliding);
		elementCurrentSlider.addEventListener('change', onSliderReleased);

		for (const element of document.querySelectorAll('[data-send-command]')) {
			element.addEventListener('click', () => {
				sendText(element.getAttribute('data-send-command'));
			})
		}
	}
	
	function onStartSliderSliding() {
		sliderSliding = true;

		let val = parseInt(elementCurrentSlider.value);
		if (val !== 0 && !(val >= 60 && val <= 160)) {
			elementCurrentSlider.value = val = 0;
		}

		assignValuesToHtml({
			currLim:  val / 10,
			powerLim: val * 23 * phases / 1000,
		})
	}

	function onSliderReleased() {
		let val = parseInt(elementCurrentSlider.value);
		if (val !== 0 && !(val >= 60 && val <= 160)) {
			elementCurrentSlider.value = val = 0;
		}

		sendText(`currLim=${val}`);
		sliderSliding = false;
	}

	function setClass(element, className, state) {
		if (state) {
			element.classList.add(className)
		} else {
			element.classList.remove(className)
		}
	}

	function processReceivedCommand(evt) {
		const message = JSON.parse(evt.data);
		let carStat;
		let wbStat;
		switch (message.chgStat) {
			case  2: /*A1*/ carStat = 'nein'; wbStat = 'nein'; break;
			case  3: /*A2*/ carStat = 'nein'; wbStat = 'ja'; break;
			case  4: /*B1*/ carStat = 'ja, ohne Ladeanf.'; wbStat = 'nein'; break;
			case  5: /*B2*/ carStat = 'ja, ohne Ladeanf.'; wbStat = 'ja'; break;
			case  6: /*C1*/ carStat = 'ja,  mit Ladeanf.'; wbStat = 'nein'; break;
			case  7: /*C2*/ carStat = 'ja,  mit Ladeanf.'; wbStat = 'ja'; break;
			default: carStat = message.chgStat; wbStat = '-';
		}
		assignValuesToHtml({
			carStat: carStat,
			wbStat:  wbStat,
			power:   message.power / 1000,
			energyI: message.energyI,
			energyC: message.energyC,
			watt:    message.watt / 1000,
			timeNow: message.timeNow,
		})

		if (!sliderSliding) {
			assignValuesToHtml({
				currLim:  message.currLim,
				powerLim: message.currLim * 230 * phases / 1000,
			});
			elementCurrentSlider.value = message.currLim * 10;
		}

		for (const element of pvModeButtons) {
			setClass(element, 'active', message.pvMode === parseInt(element.getAttribute('data-pv-mode')));
		}
		setSectionVisibility('connection', message.failCnt >= 10);
		setSectionVisibility('pvLaden', message.pvMode >= 1 && message.pvMode <= 3);

		for (const element of wallboxButtons) {
			setClass(element, 'active', message.id === parseInt(element.getAttribute('data-wallbox-id')));
		}
	}

	function exit() {
		assignValuesToHtml({
			carStat: '-',
			wbStat:  '-',
			power:   '-',
			energyI: '-',
			energyC: '-',
			currLim: '-',
			powerLim:'-',
			watt:    '-',
			timeNow: '-',
		})
		for (const element of document.querySelectorAll('[data-wallbox-id],[data-pv-mode]')) {
			setClass(element, 'active', false);
			setClass(element, 'disabled', true);
		}
		Socket.close();
	}

	function sendText(data){
		Socket.send(data);
	}

	init();

});
