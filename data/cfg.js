// Copyright (c) 2024 steff393, MIT license


// Achtung
// ----------------------------------------------------------------------------------
// Diese Datei hier ist nicht die "cfg.json", die die eigentlichen Parameter enthält.
// Parameter werden nicht hier geändert.
// ----------------------------------------------------------------------------------

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

// Default settings 24.03.2024
const defaultObj = JSON.parse(
	'{"cfgApSsid":"wbec","cfgApPass":"wbec1234","cfgCntWb":1,"cfgMbCycleTime":10,"cfgMbDelay":100,"cfgMbTimeout":60000,"cfgStandby":4,"cfgFailsafeCurrent":0,"cfgMqttIp":"","cfgMqttLp":[],"cfgMqttPort":1883,"cfgMqttUser":"","cfgMqttPass":"","cfgMqttWattTopic":"wbec/pv/setWatt","cfgMqttWattJson":"","cfgNtpServer":"europe.pool.ntp.org","cfgFoxUser":"","cfgFoxPass":"","cfgFoxDevId":"","cfgPvActive":0,"cfgPvCycleTime":30,"cfgPvLimStart":61,"cfgPvLimStop":50,"cfgPvPhFactor":69,"cfgPvOffset":0,"cfgPvCalcMode":0,"cfgPvInvert":0,"cfgPvInvertBatt":0,"cfgPvMinTime":0,"cfgPvOffCurrent":255,"cfgPvHttpIp":"","cfgPvHttpPath":"/","cfgPvHttpJson":"","cfgPvHttpPort":80,"cfgTotalCurrMax":0,"cfgLmChargeState":6,"cfgHwVersion":15,"cfgWifiSleepMode":0,"cfgLoopDelay":255,"cfgKnockOutTimer":0,"cfgShellyIp":"","cfgInverterIp":"","cfgInverterType":0,"cfgInverterPort":0,"cfgInverterAddr":0,"cfgInvSmartAddr":0,"cfgInvRegPowerInv":0,"cfgInvRegPowerInvS":0,"cfgInvRegPowerMet":0,"cfgInvRegPowerMetS":0,"cfgInvRegToGrid":0,"cfgInvRegFromGrid":0,"cfgInvRegBattery":0,"cfgBootlogSize":2000,"cfgBtnDebounce":0,"cfgWifiConnectTimeout":10,"cfgResetOnTimeout":0,"cfgEnergyOffset":0,"cfgDisplayAutoOff":2,"cfgWifiAutoReconnect":1,"cfgWifiScanMethod":0,"cfgLedIp":1,"cfgWifiOff":0,"cfgChargeLog":0,"cfgWallboxIp":"","cfgWallboxPort":502,"cfgWallboxAddr":1,"cfgRfidCurr":160,"cfgAutoEnable":1,"cfgEnwgSource":0,"cfgEnwgBox":0,"cfgWbecMac":237,"cfgWbecIp":""}'
);

const descObj = {
	cfgApSsid              :"(!) Name des initialen Access Points",
	cfgApPass              :"Passwort des initialen Access Points",
	cfgCntWb               :"Anzahl der verbundenen Wallboxen",
	cfgMbCycleTime         :"(!) [s] Modbus Zykluszeit",
	cfgMbDelay             :"(!) [ms] Min. Zeit zwischen Modbusbotschaften",
	cfgMbTimeout           :"(!) [ms] Modbus Timeout (Register 257)",
	cfgStandby             :"(!) Standby 0:aktiv, 4:inaktiv (empfohlen)",
	cfgFailsafeCurrent     :"(!) [100mA] Strom bei Modbus-Timeout (Register 262)",
	cfgMqttIp              :"MQTT-Broker: IP-Adresse, z.B. 192.168.178.123",
	cfgMqttLp              :"MQTT: Zuordnung der Ladepunkte, s. Wiki, z.B. 1 oder 1,2,3",
	cfgMqttPort            :"MQTT-Broker: Port ",
	cfgMqttUser            :"MQTT-Broker: Username (wenn nötig, max. 31 Zeichen)",
	cfgMqttPass            :"MQTT-Broker: Passwort (wenn nötig, max. 127 Zeichen)",
	cfgMqttWattTopic       :"MQTT: Topic, um den Wert Bezug/Einspeisung zu empfangen",
	cfgMqttWattJson        :"MQTT: Suchstring, um den Wert Bezug/Einspeisung zu finden",
	cfgNtpServer           :"NTP-Server",
	cfgFoxUser             :"Powerfox: Benutzername",
	cfgFoxPass             :"Powerfox: Passwort",
	cfgFoxDevId            :"Powerfox: Device ID",
	cfgPvActive            :"PV-Überschussregelung: 0:inaktiv, 1:aktiv",
	cfgPvCycleTime         :"[s] PV-Überschussregelung: Zykluszeit",
	cfgPvLimStart          :"[100mA] PV-Überschussregelung: Startstrom, z.B. 61=6,1A",
	cfgPvLimStop           :"[100mA] PV-Überschussregelung: Stopstrom, z.B. 50=5,0A",
	cfgPvPhFactor          :"PV-Überschussregelung: 23:1-ph, 42:2-ph, 69:3-phasig",
	cfgPvOffset            :"[W] PV-Überschussregelung: Offset",
	cfgPvCalcMode          :"PV-Überschussregelung: Berechnungsmodus",
	cfgPvInvert            :"PV-Überschussregelung: Vorzeichen von Bezug/Einspeisung invertieren (1)",
	cfgPvInvertBatt        :"PV-Überschussregelung: Vorzeichen von Batterieleistung invertieren (1)",
	cfgPvMinTime           :"[min] PV-Überschussregelung: Minimale Aktivierungszeit",
	cfgPvOffCurrent        :"[100mA] PV-Überschussregelung: Strom, welcher bei Wechsel auf Modus Aus eingestellt wird",
	cfgPvHttpIp            :"PV-Überschussregelung HTTP: IP-Adresse, um den Wert Bezug/Einspeisung abzufragen",
	cfgPvHttpPath          :"PV-Überschussregelung HTTP: URL, um den Wert Bezug/Einspeisung abzufragen",
	cfgPvHttpJson          :"PV-Überschussregelung HTTP: Suchstring, um den Wert Bezug/Einspeisung zu finden",
	cfgPvHttpPort          :"PV-Überschussregelung HTTP: Port, um den Wert Bezug/Einspeisung abzufragen",
	cfgTotalCurrMax        :"[100mA] Maximaler Systemstrom bei mehreren Wallbox, ACHTUNG: SICHERUNG NÖTIG!",
	cfgLmChargeState       :"(!) Ladezustand, ab dem Lastmanagement eine Ladeanforderung erkennt",
	cfgHwVersion           :"(!) intern",
	cfgWifiSleepMode       :"(!) intern",
	cfgLoopDelay           :"(!) intern",
	cfgKnockOutTimer       :"(!) [min] Zyklischer Reset von wbec alle xx Minuten",
	cfgShellyIp            :"Shelly: IP-Adresse, um den Wert Bezug/Einspeisung abzufragen",
	cfgInverterIp          :"Modbus-TCP: IP-Adresse, um den Wert Bezug/Einspeisung abzufragen",
	cfgInverterType        :"Modbus-TCP: Typ, s. Wiki",
	cfgInverterPort        :"Modbus-TCP: Port, s. Wiki",
	cfgInverterAddr        :"(!) Modbus-TCP: Modbus-Adresse, s. Wiki",
	cfgInvSmartAddr        :"(!) Modbus-TCP: Modbus-Adresse, s. Wiki",
	cfgInvRegPowerInv      :"(!) Modbus-TCP: Register",
	cfgInvRegPowerInvS     :"(!) Modbus-TCP: Register",
	cfgInvRegPowerMet      :"(!) Modbus-TCP: Register",
	cfgInvRegPowerMetS     :"(!) Modbus-TCP: Register",
	cfgInvRegToGrid        :"(!) Modbus-TCP: Register",
	cfgInvRegFromGrid      :"(!) Modbus-TCP: Register",
	cfgInvRegBattery       :"(!) Modbus-TCP: Register",
	cfgBootlogSize         :"(!) intern",
	cfgBtnDebounce         :"[ms] Entprellzeit für Taster, z.B. 300",
	cfgWifiConnectTimeout  :"(!) (s) Wartezeit bis wbec bei fehlendem WLAN einen eigenen Access Point öffnet",
	cfgResetOnTimeout      :"(!) Nullen aller Werte bei Modbus-Timeout",
	cfgEnergyOffset        :"[Wh] Offset, der vom Energiezähler abgezogen werden kann",
	cfgDisplayAutoOff      :"[min] Wartezeit für Displayabschaltung",
	cfgWifiAutoReconnect   :"(!) intern",
	cfgWifiScanMethod      :"(!) 0: WIFI_FAST_SCAN (default), 1: WIFI_ALL_CHANNEL_SCAN (evtl. bei Mesh)",
	cfgLedIp               :"IP-Adresse von wbec nach Reset signalisieren, 0:inaktiv, 1:aktiv",
	cfgWifiOff             :"(!) WLAN abschalten (1) - VORSICHT!",
	cfgChargeLog           :"Logbuch der Ladevorgänge: 0:inaktiv, 1:aktiv",
	cfgWallboxIp           :"(!) connect.home Wallbox",
	cfgWallboxPort         :"(!) connect.home Wallbox",
	cfgWallboxAddr         :"(!) connect.home Wallbox",
	cfgRfidCurr            :"[100mA] Strom bei Freischaltung per RFID",
	cfgAutoEnable          :"1: nach Wakeup von Standby den letzten Stromwert wiederherstellen",
	cfgEnwgSource          :"§14a EnWG: Quelle: 0:inaktiv, 1:Schließer, 2:Öffner, 3:HTTP, Achtung: permanent!",
	cfgEnwgBox             :"§14a EnWG: Auswahl der Box für die Leistungsreduzierung",
	cfgWbecMac             :"(!) wbecLan: Letztes Byte der wbec-MAC-Adresse ändern (dez.)",
	cfgWbecIp              :"(!) wbecLan: stat. IP-Adresse für wbec, z.B. 192.168.178.123",
}


window.addEventListener('DOMContentLoaded', () => {
	var rootElement = document.documentElement;
	rootElement.style.setProperty('--container-width-max', '1000px');

	initNavBar();
	document.getElementById('btnStore').addEventListener('click', storeCfg);
	document.getElementById('btnReset').addEventListener('click', resetWbec);
	document.getElementById('btnRefresh').addEventListener('click', refresh);
	document.getElementById('btnResWifi').addEventListener('click', resetWifi);
	const settings = {};

	createHtmlTable();

	fetch('/cfg.json')
		.then(response => response.json())
		.then(data => {
			// Walk through all defaultObj parameters and check, whether there is a differing value in "data"
			// settings = defaultObj + all changed values from data
			for (const key in defaultObj) {
					if (data.hasOwnProperty(key) && data[key] !== defaultObj[key]) {
						settings[key] = data[key];
					} else {
						settings[key] = defaultObj[key];
					}
			}
			console.log(settings);
			// fill the HTML page
			for (const key in defaultObj) {
				document.getElementById(key).value = settings[key];
			}
		})
		.catch(error => {
			console.log('cfg.json not found: ', error);
		});
});


function compareObjects(obj1, obj2) {
	// Check all objects in the default (obj1), whether they have a differing value in obj2
	const result = {};
	for (let key in obj1) {
		if ((obj1.hasOwnProperty(key) && obj2.hasOwnProperty(key) && obj1[key] !== obj2[key]) ||
		   (key=="cfgApPass") || (key=="cfgCntWb")) { // to have 2 minimum parameters
			result[key] = obj2[key]; // Return a new object that only contains differing values
		}
	}
	return JSON.stringify(result); // return as JSON
}


function storeCfg() {
	// fetch data from the HTML page
	var configObj = {};
	for (const key in defaultObj) {
		if (key == 'cfgMqttLp') {   // needs special array handling
			var val = document.getElementById(key).value;
			if (val != '') {
				configObj[key] = val.split(',').map(Number);
			} 
		} else {
			if (document.getElementById(key).type == 'number') {
				configObj[key] = parseInt(document.getElementById(key).value);
			}
			if (document.getElementById(key).type == 'text') {
				configObj[key] = document.getElementById(key).value;
			}
		}
	}

	const deltaJson = compareObjects(defaultObj, configObj);
	console.log(deltaJson);

	// append the JSON-String to a FormData and assign a file name
	var formData = new FormData();
	formData.append('file', new Blob([deltaJson.replace(',"cfgMqttLp":""', '')], { type: 'application/json' }), '/cfg.json');

	// configure the POST request
	var options = {
		method: 'POST',
		body: formData
	};

	fetch('/edit', options)
		.then(response => {
			if (response.ok) {
				console.log('POST request sent successfully');
			} else {
				console.error('Error during POST request');
			}
		})
		.catch(error => {
			console.error('Error during POST request: ', error);
		});
}


function createHtmlTable() {
	var tableContainer = document.createElement('div');
	tableContainer.className = 'config-table';

	// Iteriere über die Eigenschaften des JSON-Objekts
	for (var key in defaultObj) {
		if (defaultObj.hasOwnProperty(key)) {
			// create the row
			var row = document.createElement('div');
			row.className = 'config-row';

			// create cell 1
			var keyCell = document.createElement('div');
			keyCell.className = 'config-cell';
			keyCell.innerHTML = key;
			row.appendChild(keyCell);

			// create cell 2 for the input
			var inputCell = document.createElement('div');
			inputCell.className = 'config-cell';
			var input = document.createElement('input');
			input.type = typeof defaultObj[key] === 'number' ? 'number' : 'text';
			input.id = key;
			inputCell.appendChild(input);
			row.appendChild(inputCell);

			// create cell 3 for the description
			var descriptionCell = document.createElement('div');
			descriptionCell.className = 'config-cell';
			descriptionCell.innerHTML = descObj[key];
			row.appendChild(descriptionCell);

			tableContainer.appendChild(row);
		}
	}
	var tableBox = document.getElementById('tableBox');
	tableBox.appendChild(tableContainer);
}


function resetWbec() {
	fetch('/reset');
}


function resetWifi() {
	if (confirm('Möchtest du wirklich die WLAN-Zugangsdaten löschen? Hast du dir den Parameter cfgApPass notiert?')) {
		window.open('/resetwifi', '_self');
	}
}


function refresh() {
	location.reload();
}
