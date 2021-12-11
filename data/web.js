// Copyright (c) 2021 steff393, MIT license

var Socket;


function init() 
{
	Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
	Socket.onmessage = function(event) { processReceivedCommand(event); };
	showPFoxElements("hidden");
}

// Update the current slider value (each time you drag the slider handle)
document.getElementById("slideCurr").oninput = function() {
	var val = parseInt(this.value);
	if (val == 0 || (val >= 6 && val <=16)) {
		document.getElementById("currLim").innerHTML = val;
		sendText('currLim=' + val * 10);
	} else {
		document.getElementById("currLim").innerHTML = 0;
		document.getElementById("slideCurr").value = 0;
		sendText('currLim=0');
	}
}
 
 
function processReceivedCommand(evt) 
{
		const obj = JSON.parse(evt.data);
		var carStat;
		var wbStat;
		switch (obj.chgStat) {
			case  2: /*A1*/ carStat = 'nein'; wbStat = 'nein'; break;
			case  3: /*A2*/ carStat = 'nein'; wbStat = 'ja'; break;
			case  4: /*B1*/ carStat = 'ja, ohne Ladeanf.'; wbStat = 'nein'; break;
			case  5: /*B2*/ carStat = 'ja, ohne Ladeanf.'; wbStat = 'ja'; break;
			case  6: /*C1*/ carStat = 'ja,  mit Ladeanf.'; wbStat = 'nein'; break;
			case  7: /*C2*/ carStat = 'ja,  mit Ladeanf.'; wbStat = 'ja'; break;
			default: carStat = obj.chgStat; wbStat = '-';
		}
		document.getElementById('carStat').innerHTML = carStat;
		document.getElementById('wbStat').innerHTML = wbStat;
		document.getElementById('power').innerHTML = obj.power;
		document.getElementById('energyI').innerHTML = obj.energyI;
		document.getElementById('currLim').innerHTML = obj.currLim;
		document.getElementById('watt').innerHTML = obj.watt;
		document.getElementById('timeNow').innerHTML = obj.timeNow;
		document.getElementById("slideCurr").value = obj.currLim;
		switch (obj.pvMode) {
			case 1:  document.getElementById('pvMode').innerHTML = 'Aus';    showPFoxElements("visible"); break;
			case 2:  document.getElementById('pvMode').innerHTML = 'PV';     showPFoxElements("visible"); break;
			case 3:  document.getElementById('pvMode').innerHTML = 'Min+PV'; showPFoxElements("visible");break;
			default: {
				document.getElementById('pvMode').innerHTML = '-'; showPFoxElements("hidden");
			}
		}
}


function showPFoxElements(state) {
	document.getElementById('trPFox1').style.visibility = state;
	document.getElementById('trPFox2').style.visibility = state;
	document.getElementById('pvLaden').style.visibility = state;
}
 
 
document.getElementById('btnAus').addEventListener('click', function() {
	sendText('PV_OFF');
	document.getElementById("pvStat").innerHTML = 'Aus';
});
document.getElementById('btnPv').addEventListener('click', function() {
	sendText('PV_ACTIVE');
	document.getElementById("pvStat").innerHTML = 'PV';
});
document.getElementById('btnMinPv').addEventListener('click', function() {
	sendText('PV_MIN_PV');
	document.getElementById("pvStat").innerHTML = 'Min+PV';
});


/*
document.getElementById('btn1').addEventListener('click', function() {
	sendText('id=0');
});
document.getElementById('btn2').addEventListener('click', function() {
	sendText('id=1');
});
document.getElementById('btn3').addEventListener('click', function() {
	sendText('id=2');
});
*/


function sendText(data){
	Socket.send(data);
}


window.onload = function(e){ 
	init();
}