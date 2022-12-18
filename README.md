**wbec** - WLAN-Anbindung der Heidelberg **W**all**B**ox **E**nergy **C**ontrol über ESP8266  

Die Heidelberg Wallbox Energy Control ist eine hochwertige Ladestation, bietet aber nur Modbus RTU als Schnittstelle.  
Ziel des Projekts ist es, eine WLAN-Schnittstelle zu entwickeln um zusätzliche Funktionen (z.B. PV-Überschussladen) zu ermöglichen.  

[wbec Homepage](https://steff393.github.io/wbec-site/)  
[Empfehlung im Heidelberg Amperfied Blog](https://www.amperfied.de/de/clever-laden/blog/wbec-fuer-heidelberg-wallbox-energy-control-blog/)  
  
![GitHub all releases](https://img.shields.io/github/downloads/steff393/wbec/total?color=blue&style=flat-square)  

## Funktionen
- Anbindung an openWB, EVCC, Solaranzeige
- MQTT-Kommunikation mit openWB und EVCC (ideal für mehrere Ladestationen)
- Steuerbar per Android App [Wallbox Steuerung](https://android.chk.digital/ecar-charger-control/) 
- PV-Überschussladen, Zielladen, etc. mit den o.g. Steuerungen
- Abfrage von Shelly 3EM, powerfox, Solaredge, Fronius, ...
- RFID-Kartenleser zur Freischaltung der Wallbox mit gültiger Karte/Chip (spezielle HW nötig, s. Wiki)  
- Ansteuerung aller verbundenen Ladestationen (bis zu 16 Follower am Modbus, bis zu 8 openWB-Ladepunkte)
- Einfacher Prototyp eines lokalen Lastmanagements für zwei Wallboxen (in Entwicklung)
- Softwareupdate per WLAN (Over The Air), z.B. mit PlatformIO oder einfach per Browser (s. Wiki)
- Weniger als 1W Strombedarf (trotz Ansteuerung von bis zu 16 Ladestationen)

## Kontakt
Bei Fragen oder wenn ihr Unterstützung braucht gerne einfach eine Mail schicken (wbec393@gmail.com).    
Bitte schaut auch ins [Wiki](https://github.com/steff393/wbec/wiki) und in meine anderen Projekte, z.B. den [SmartUploader](https://github.com/steff393/SmartUploader) zum Auslesen von Wechselrichtern und [hgdo](https://github.com/steff393/hgdo) zur Steuerung von Torantrieben.  

## Beispiele
Einfaches Web-Interface (geeignet für alle Browser, Smartphone, PC, etc.):  
`http://wbec.local/`  
<p align="center"> 
  <img src="https://i.ibb.co/3sg0YdL/wbec-web3.png"> 
</p>

JSON API Schnittstelle:  
`http://wbec.local/json`  
```c++
{
  "wbec": {
    "version": "v0.3.0"         // wbec version
    "bldDate": "2021-06-10"     // wbec build date
  },
  "box": [
    {                           // s. also https://wallbox.heidelberg.com/wp-content/uploads/2021/04/EC_ModBus_register_table_20210222.pdf
      "busId": 1,               // Modbus bus id (as configured by DIP switches)
      "version": "108",         // Modbus Register-Layouts Version, e.g. 1.0.8
      "chgStat": 2,             // Charging State
      "currL1": 0,              // L1 - Current RMS (in 0.1A)
      "currL2": 0,              // L2 - Current RMS (in 0.1A)
      "currL3": 0,              // L3 - Current RMS (in 0.1A)
      "pcbTemp": 333,           // PCB-Temperatur (in 0.1°C)
      "voltL1": 232,            // Voltage L1 - N rms in Volt
      "voltL2": 9,              // Voltage L2 - N rms in Volt
      "voltL3": 9,              // Voltage L3 - N rms in Volt
      "extLock": 1,             // extern lock state
      "power": 0,               // Power (L1+L2+L3) in VA
      "energyP": 0,             // Energy since PowerOn (in kWh)
      "energyI": 0.003,         // Energy since Installation (in kWh)
      "currMax": 16,            // Hardware configuration maximal current (in 0.1A)
      "currMin": 6,             // Hardware configuration minimal current (in 0.1A)
      "logStr": "<item no> <mfgDate> <serial>",
      "wdTmOut": 15000,         // ModBus-Master WatchDog Timeout (in ms)
      "standby": 4,             // Standby Function Control 
      "remLock": 1,             // Remote lock (only if extern lock unlocked) 
      "currLim": 130,           // Maximal current command
      "currFs": 0,              // FailSafe Current configuration 
      "load": 0,                // wbec load management
      "resCode": "0"            // Result code of last Modbus message (0 = ok)
    },
    {                           // Values of 2nd box ...
      "busId": 2,
      "version": "0",
      "chgStat": 0,
      ...
      "load": 0,
      "resCode": "e4"
    }
  ],
  "modbus": {
    "state": {
      "lastTm": 2852819,        // Timestamp of last Modbus message (in ms)
      "millis": 2855489         // Time since start of wbec (in ms)
    }
  },
  "rfid": {
    "enabled": true,
    "release": false,
    "lastId": "0cb6a781"
  },
  "wifi": {
    "mac": "00:1F:3F:15:29:7E", // wbec MAC address
    "rssi": -76,                // WiFi signal
    "signal": 48,               // WiFi signal quality (in %)
    "channel": 11               // WiFi channel
  }
}
```

Maximalen Ladestrom einstellen:
```c++
http://192.168.xx.yy/json?currLim=120      --> set current limit to 12A (on the box with id=0, i.e. ModBus Bus-ID=1)
http://192.168.xx.yy/json?currLim=60&id=2  --> set current limit to 6A on the box with id=2 (i.e. ModBus Bus-ID=3)
```

Watchdog Timeout einstellen:
```c++
http://192.168.xx.yy/json?wdTmOut=20000  --> 20s
```

## Danksagung
Folgende Projekte wurden in wbec genutzt/angepasst:  
- [modbus-esp8266](https://github.com/emelianov/modbus-esp8266)
- [ESP Async WebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [PubSubClient](https://github.com/knolleary/PubSubClient)
- [NTPClient](https://github.com/arduino-libraries/NTPClient)
- [MFRC522](https://github.com/miguelbalboa/MFRC522)
- [RTCVars](https://github.com/highno/RTCVars)
- [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [Web Interface](https://RandomNerdTutorials.com)
- [A Beginner's Guide to the ESP8266 - article](https://github.com/tttapa/ESP8266)
- [AsyncElegantOTA](https://github.com/ayushsharma82/AsyncElegantOTA)

Ein besonderer Dank ergeht an die frühen Tester und Unterstützer: mli987, profex1337, Clanchef und viele mehr!

## Unterstützung des Projektes
wbec gefällt dir? Dann gib dem Projekt [einen Stern auf GitHub](https://github.com/steff393/wbec/stargazers)!
