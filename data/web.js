// Copyright (c) 2021 steff393, MIT license

var Socket;


function init() 
{
	Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
	Socket.onmessage = function(event) { processReceivedCommand(event); };

	//output.innerHTML = document.getElementById("slideCurr").value; // Display the default slider value
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
		document.getElementById('chgStat').innerHTML = obj.chgStat;
		document.getElementById('power').innerHTML = obj.power;
		document.getElementById('energyI').innerHTML = obj.energyI;
		document.getElementById('currLim').innerHTML = obj.currLim;
		document.getElementById('watt').innerHTML = obj.watt;
		document.getElementById("slideCurr").value = obj.currLim;
}
 
 
document.getElementById('btnAus').addEventListener('click', function() {
	sendText('pfoxDisable');
	document.getElementById("pvStat").innerHTML = 'Aus';
});
document.getElementById('btnMinPv').addEventListener('click', function() {
	sendText('pfoxEnable');
	document.getElementById("pvStat").innerHTML = 'Min+PV';
});
document.getElementById('btnPv').addEventListener('click', function() {
	sendText('pfoxEnable');
	document.getElementById("pvStat").innerHTML = 'PV';
});


function sendText(data){
	Socket.send(data);
}


window.onload = function(e){ 
	init();
}