// Copyright (c) 2023 steff393

// ----------------------------- COMMON SECTION: START ------------------------------
function initNavBar() {
	for (const element of document.querySelectorAll('[top-nav-link]')) {
		element.addEventListener('click', function() {window.location.href = element.getAttribute('top-nav-link')});
	}
}

function assignValuesToHtml(values) {
	let valueContainerElements = document.querySelectorAll('[data-value]');
	for (const element of valueContainerElements) {
		const key = element.getAttribute('data-value');
		if (values[key] !== undefined) {
			element.innerHTML = values[key].toLocaleString('de-DE');
		}
	}
}

function setClass(element, className, state) {
	if (state) {
		element.classList.add(className)
	} else {
		element.classList.remove(className)
	}
}

function setSectionVisibility(sectionId, isVisible) {
	setClass(document.getElementById(sectionId), 'not-available', !isVisible);
}
// ----------------------------- COMMON SECTION:  END  ------------------------------

window.addEventListener('DOMContentLoaded', () => {
	// Adjustable values -----
	let kWhPer100km    = 20;  // kWh needed for driving 100km
	let phases         = 3;   // number of phases
	let maxCapacity    = 65;  // maximum battery capacity of car in kWh
	let corrFactor     = 1;   // correction factor, can be changed to 1.1 or 1.2 if needed
	let pricePerkWh    = 30   // ct per kWh
	let maxEnergyDelta = 200  // defines range of the energy slider in kWh
	let timeResolution = 4    // 4=quarter hours, 12 = 5mins, 60 = 1min
	// -----------------------
	let id             = 0;   // currently only first box supported
	let knobShift      = 10;  // vertical offset of the knobs
	let sliderContainer        = document.getElementById('sliderContainer');
	let sliderTrack            = document.getElementById('sliderTrack');
	let sliderRange            = document.getElementById('sliderRange');
	let sliderKnobStart        = document.getElementById('sliderKnobStart');
	let sliderKnobEnd          = document.getElementById('sliderKnobEnd');
	let elementSliderCurrStart = document.getElementById('sliderCurrStart');
	let elementSliderCurrEnd   = document.getElementById('sliderCurrEnd');
	let elementSliderEnergy    = document.getElementById('sliderEnergy');
	let cbOnce                 = document.getElementById('cbOnce');    
	let cbOnlyActiveOff        = document.getElementById('cbOnlyActiveOff');
	let cbDailyEnergyDelta     = document.getElementById('cbDailyEnergyDelta');
	let btnLoad                = document.getElementById('btnLoad');    
	let btnSave                = document.getElementById('btnSave');    
	let btnDeact               = document.getElementById('btnDeact');    
	let divTimeStart           = document.getElementById('divTimeStart');
	let divSliderStart         = document.getElementById('divSliderStart');
	let wallboxButtons         = document.querySelectorAll('[data-wallbox-id]');
	let activeElement          = null;  // Flag to check which element (knob or range) is being moved
	let energyNow              = 0;

	function init() {
		setSectionVisibility('boxSelection', wallboxButtons.length > 1);
		initNavBar();
		load();
		
		sliderKnobStart.addEventListener('mousedown',  handleElementStart);
		sliderKnobStart.addEventListener('touchstart', handleElementStart);
		sliderKnobEnd.  addEventListener('mousedown',  handleElementStart);
		sliderKnobEnd.  addEventListener('touchstart', handleElementStart);
		
		sliderRange.addEventListener('mousedown',  handleElementStart);
		sliderRange.addEventListener('touchstart', handleElementStart);
		
		window.addEventListener('mouseup',   handleElementEnd);
		window.addEventListener('touchend',  handleElementEnd);
		window.addEventListener('mousemove', handleElementMove);
		window.addEventListener('touchmove', handleElementMove);

		elementSliderCurrStart.addEventListener('input', onSetCurrentSliderStart);
		elementSliderCurrEnd  .addEventListener('input', onSetCurrentSliderEnd);
		elementSliderEnergy   .addEventListener('input', onSetSliderEnergy);

		cbOnlyActiveOff       .addEventListener('click', onCbOnlyActiveOff);
		btnLoad               .addEventListener('click', load);
		btnSave               .addEventListener('click', save);
		btnDeact              .addEventListener('click', deactivate);

		elementSliderEnergy.max   = maxEnergyDelta;
	}

	function onCbOnlyActiveOff() {
		if (cbOnlyActiveOff.checked) {
			// disable the Start slider, when OnlyActiveOff is set
			divTimeStart  .classList.add('divDisabled');
			divSliderStart.classList.add('divDisabled');
			elementSliderCurrStart.disabled = true;
			elementSliderCurrStart.value = 0;
			onSetCurrentSliderStart();
		} else {
			divTimeStart  .classList.remove('divDisabled');
			divSliderStart.classList.remove('divDisabled');
			elementSliderCurrStart.disabled = false;
		}
	}

	function limitCurrent(val) {
		if (val >= 60 && val <= 160) {
			return(val);
		} else if (val > 50 && val < 60 ) {
			return(60);
		} else {
			return(0);
		}
	}

	function onSetCurrentSliderStart() {
		let val = limitCurrent(parseInt(elementSliderCurrStart.value));
		elementSliderCurrStart.value = val;
		assignValuesToHtml({
			currStart : val / 10,
		});
		reCalculation();
	}

	function onSetCurrentSliderEnd() {
		let val = limitCurrent(parseInt(elementSliderCurrEnd.value));
		elementSliderCurrEnd.value = val;
		assignValuesToHtml({
			currEnd   : val / 10,
		});
		reCalculation();
	}

	function onSetSliderEnergy() {
		reCalculation();
	}

	function reCalculation() {
		let eDel = parseInt(elementSliderEnergy.value)
		assignValuesToHtml({
			powerEst     : getPowerEstimation(),
			energyEst    : getEnergyEstimation(),
			mileageEst   : getMilageEstimation(),
			percentEst   : getPercentEstimation(),
			priceEst     : getPriceEstimation(),
			energyDelta  : eDel,
			energyTarget : eDel ? energyNow + eDel : '-'
		});
	}

	function getPowerEstimation() {
		let power     = parseInt(elementSliderCurrStart.value) / 10 * 230 * phases;
		return(parseFloat((power/1000).toFixed(1)));
	}

	function getEnergyEstimation() {
		let startTime = getTime(sliderKnobStart.offsetLeft + knobShift);
		let endTime   = getTime(sliderKnobEnd.  offsetLeft + knobShift);
		let power     = parseInt(elementSliderCurrStart.value) / 10 * 230 * phases;
		let duration  = endTime - startTime;
		if (startTime > endTime) {
			duration += 24;
		} 
		let energy    = power * duration / 1000 * corrFactor;
		energy = Math.min(energy, maxCapacity);
		if (elementSliderEnergy.value != 0) {
			energy = Math.min(energy, parseInt(elementSliderEnergy.value));
		}
		return(parseFloat(energy.toFixed(1)));
	}

	function getMilageEstimation() {
		let milage = getEnergyEstimation() * 100 / kWhPer100km;
		return(parseInt(milage.toFixed(0)));
	}

	function getPercentEstimation() {
		let percent = getEnergyEstimation() * 100 / maxCapacity;
		percent = Math.min(percent, 100);
		return(parseInt(percent.toFixed(0)));
	}

	function getPriceEstimation() {
		let price = getEnergyEstimation() / 100 * pricePerkWh;
		return(parseInt(price.toFixed(0)));
	}

	function dec2hhmm(decimalTime) {
		const wholeNumberPart = Math.floor(decimalTime);
		const decimalPart = decimalTime - wholeNumberPart;
		const minutes = Math.round(decimalPart * 60);
		
		// Ensure minutes are displayed with leading zero if needed (e.g., 4:03 instead of 4:3)
		const formattedMinutes = String(minutes).padStart(2, '0');
		
		return `${wholeNumberPart}:${formattedMinutes}`;
	}

	function pos2time(pos) {
		return(pos / sliderTrack.offsetWidth * 24);
	}

	function time2pos(time) {
		return(time / 24 * sliderTrack.offsetWidth);
	}

	// Converts the slider's position to the corresponding value
	function getTime(position) {
		let value = (pos2time(position) * timeResolution).toFixed(0) / timeResolution;  // convert to quarterly hours
		return(parseFloat(value.toFixed(2)));
	}

	// Updates the displayed time range
	function updateTimeRange() {	
		let startTime = getTime(sliderKnobStart.offsetLeft + knobShift);
		let endTime   = getTime(sliderKnobEnd.  offsetLeft + knobShift);
		
		assignValuesToHtml({
			timeStart : dec2hhmm(startTime),
			timeEnd   : dec2hhmm(endTime),
		});
		reCalculation();
	}

	function drawSlider(start, end) {
		let invert = start > end; 
		sliderKnobStart.style.left  =  start - knobShift + 'px';
		sliderKnobEnd.  style.left  =  end   - knobShift + 'px'; //  inverted mode  : normal mode
		sliderRange.style.left  =          (invert ? end                    : start)         + 'px';
		sliderRange.style.width =          (invert ? (start - end)          : (end - start)) + 'px';
		sliderRange.style.backgroundColor = invert ? 'var(--theme-color-5)' : 'var(--theme-color-2)';
		sliderTrack.style.backgroundColor = invert ? 'var(--theme-color-2)' : 'var(--theme-color-5)';
	}

	// Activates the element (knob or range) when a mouse or touch start event is triggered
	function handleElementStart(event) {
		activeElement = event.target;
		
		if (event.type === 'touchstart') {
			event.preventDefault(); // Prevents screen scrolling during slider operation
		}
	}

	// Deactivates the element (knob or range) when a mouse or touch end event is triggered
	function handleElementEnd() {
		activeElement = null;
	}

	// Moves the element (knob or range) when a mouse or touch move event is triggered
	function handleElementMove(event) {
		if (activeElement) {
			let newPosition = event.clientX;
			
			if (event.type === 'touchmove') {
				newPosition = event.touches[0].clientX; // Takes finger position into account for touch events
			}
			
			let rect = sliderContainer.getBoundingClientRect();
			newPosition -= rect.left;
			
			newPosition = Math.max(0, Math.min(newPosition, sliderTrack.offsetWidth));

			let posStart = sliderKnobStart.offsetLeft + knobShift;
			let posEnd   = sliderKnobEnd.  offsetLeft + knobShift;
			let oldWidth = posEnd - posStart;

			if (activeElement === sliderKnobStart) {
				posStart = newPosition;
			} else if (activeElement === sliderKnobEnd) {
				posEnd   = newPosition;
			} else if (activeElement === sliderRange) {
				posStart = newPosition - oldWidth / 2;    // newPosition is exactly the middle of the bar
				if (posStart < posEnd) {
					posStart = Math.max(0, Math.min(posStart, sliderTrack.offsetWidth - oldWidth)); // keep the range inside the track
				} else {
					posStart = Math.max(0 - oldWidth, Math.min(posStart, sliderTrack.offsetWidth)); // keep the range inside the track (in inverted mode)
				}
				posEnd   = posStart + oldWidth;
			}
			drawSlider(posStart, posEnd);
			updateTimeRange();
		}
	}

	function load() {
		fetch('/time?id=' + id)
		.then(response => response.json())
		.then(data => {
			drawSlider(time2pos(data['mOn'] / 60), time2pos(data['mOff'] / 60));
			elementSliderCurrStart.value = data['cOn'];
			elementSliderCurrEnd.  value = data['cOff'];
			elementSliderEnergy.   value = data['eDel'];
			energyNow = data['eNow'];
			assignValuesToHtml({
				currStart    : data['cOn'] / 10,
				currEnd      : data['cOff'] / 10,
				energyDelta  : data['eDel'],
				energyNow    : data['eNow'],
				energyTarget : data['eDel'] ? data['eNow'] + data['eDel'] : '-'
			})
			cbOnce.checked             = data['flag'] & 0x01;
			cbOnlyActiveOff.checked    = data['flag'] & 0x02;
			cbDailyEnergyDelta.checked = data['flag'] & 0x04;
			updateTimeRange();
			onCbOnlyActiveOff();
		})
		.catch(error => {
			console.log('No answer to /time: ', error);
		});
	}

	function save() {
		let mOn  = getTime(sliderKnobStart.offsetLeft + knobShift) * 60; // minutes since midnight
		let mOff = getTime(sliderKnobEnd.  offsetLeft + knobShift) * 60;
		let cOn  = elementSliderCurrStart.value;
		let cOff = elementSliderCurrEnd.value;
		let flag = cbOnce.checked + 
					 2 * cbOnlyActiveOff.checked + 
					 4 * cbDailyEnergyDelta.checked;
		let eDel = elementSliderEnergy.value;
		// example: http://wbec.local/time?mOn=900&mOff=975&cOn=124&cOff=66&flag=0&eDel=1000&id=0
		fetch('/time?mOn=' + mOn + '&mOff=' + mOff + '&cOn=' + cOn + '&cOff=' + cOff + '&flag=' + flag + '&eDel=' + eDel + '&id=' + id)
		.catch(error => {
			console.log('No answer to /time: ', error);
		});
	}

	function deactivate() {
		fetch('/time?mOn=0&mOff=0&cOn=0&cOff=0&flag=0&eDel=0&id=' + id)
			.then(data => {
				load();
			})
			.catch(error => {
				console.log('No answer to /time: ', error);
			})
	}

	// Initialization
	init();
});
