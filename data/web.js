var Socket;
function init() 
{
	Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
	Socket.onmessage = function(event) { processReceivedCommand(event); };
}
 
 
function processReceivedCommand(evt) 
{
		document.getElementById('rd').innerHTML = evt.data;
		if (evt.data ==='0') 
		{  
				document.getElementById('BTN_LED').innerHTML = 'Turn on the LED';  
				document.getElementById('LED_status').innerHTML = 'LED is off';  
		}
		if (evt.data ==='1') 
		{  
				document.getElementById('BTN_LED').innerHTML = 'Turn off the LED'; 
				document.getElementById('LED_status').innerHTML = 'LED is on';   
		}
}
 
 
document.getElementById('BTN_LED').addEventListener('click', buttonClicked);
function buttonClicked()
{   
	var btn = document.getElementById('BTN_LED')
	var btnText = btn.textContent || btn.innerText;
	if (btnText ==='Turn on the LED') { btn.innerHTML = 'Turn off the LED'; document.getElementById('LED_status').innerHTML = 'LED is on';  sendText('1'); }  
	else                              { btn.innerHTML = 'Turn on the LED';  document.getElementById('LED_status').innerHTML = 'LED is off'; sendText('0'); }
}

function sendText(data)
{
	Socket.send(data);
}


window.onload = function(e)
{ 
	init();
}