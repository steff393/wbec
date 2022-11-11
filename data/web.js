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
	if (val == 0 || (val >= 60 && val <=160)) {
		document.getElementById("currLim").innerHTML = val / 10;
		sendText('currLim=' + val);
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
		document.getElementById('energyC').innerHTML = obj.energyC;
		document.getElementById('currLim').innerHTML = obj.currLim;
		document.getElementById('watt').innerHTML = obj.watt;
		document.getElementById('timeNow').innerHTML = obj.timeNow;
		document.getElementById("slideCurr").value = obj.currLim * 10;
		var btnAus   = document.getElementById('btnAus');
		var btnPv    = document.getElementById('btnPv');
		var btnMinPv = document.getElementById('btnMinPv');
		switch (obj.pvMode) {
			case 1:  btnAus.style.backgroundColor="orange"; btnPv.style.backgroundColor="grey"; btnMinPv.style.backgroundColor="grey"; showPFoxElements("visible"); break;
			case 2:  btnAus.style.backgroundColor="grey"; btnPv.style.backgroundColor="orange"; btnMinPv.style.backgroundColor="grey"; showPFoxElements("visible"); break;
			case 3:  btnAus.style.backgroundColor="grey"; btnPv.style.backgroundColor="grey"; btnMinPv.style.backgroundColor="orange"; showPFoxElements("visible"); break;
			default: {
				showPFoxElements("hidden");
			}
		}
		/* begin BoxSelection */
		var btn1     = document.getElementById('btn1');
		var btn2     = document.getElementById('btn2');
		var btn3     = document.getElementById('btn3');
		switch (obj.id) {
			case 0:  btn1.style.backgroundColor="orange"; btn2.style.backgroundColor="grey"; btn3.style.backgroundColor="grey"; break;
			case 1:  btn1.style.backgroundColor="grey"; btn2.style.backgroundColor="orange"; btn3.style.backgroundColor="grey"; break;
			case 2:  btn1.style.backgroundColor="grey"; btn2.style.backgroundColor="grey"; btn3.style.backgroundColor="orange"; break;
			default: {
				btn1.style.backgroundColor="grey"; btn2.style.backgroundColor="grey"; btn3.style.backgroundColor="grey";
			}
		}
		/* end BoxSelection */
}


function showPFoxElements(state) {
	var stylePv = document.getElementById('pvLaden').style;
	if (state == "hidden") { stylePv.display = "none"; } else { stylePv.display = "block"; }
	document.getElementById('boxSelection').style.display = "none"; // choose "block" if the buttons shall appear /* BoxSelection */
}
 
 
document.getElementById('btnAus').addEventListener('click', function() {
	sendText('PV_OFF');
});
document.getElementById('btnPv').addEventListener('click', function() {
	sendText('PV_ACTIVE');
});
document.getElementById('btnMinPv').addEventListener('click', function() {
	sendText('PV_MIN_PV');
});
document.getElementById('btnExit').addEventListener('click', function() {
	document.getElementById('carStat').innerHTML = '-';
	document.getElementById('wbStat').innerHTML = '-';
	document.getElementById('power').innerHTML = '-';
	document.getElementById('energyI').innerHTML = '-';
	document.getElementById('energyC').innerHTML = '-';
	document.getElementById('currLim').innerHTML = '-';
	document.getElementById('watt').innerHTML = '-';
	document.getElementById('timeNow').innerHTML = '-';
	document.getElementById('btnAus').style.backgroundColor="grey"; 
	document.getElementById('btnPv').style.backgroundColor="grey"; 
	document.getElementById('btnMinPv').style.backgroundColor="grey"; 
	document.getElementById('btn1').style.backgroundColor="grey"; /* BoxSelection */
	document.getElementById('btn2').style.backgroundColor="grey"; /* BoxSelection */
	document.getElementById('btn3').style.backgroundColor="grey"; /* BoxSelection */
	Socket.close();
});


/* begin BoxSelection */
document.getElementById('btn1').addEventListener('click', function() {
	sendText('id=0');
});
document.getElementById('btn2').addEventListener('click', function() {
	sendText('id=1');
});
document.getElementById('btn3').addEventListener('click', function() {
	sendText('id=2');
});
/* end BoxSelection */


function sendText(data){
	Socket.send(data);
}


window.onload = function(e){ 
	init();
}
