< scroll down for English version and additional information >

# wbec
WLAN-Anbindung der Heidelberg **W**all**B**ox **E**nergy **C**ontrol über ESP8266  
  
![GitHub all releases](https://img.shields.io/github/downloads/steff393/wbec/total?color=blue&style=flat-square) 
![Lines of code](https://img.shields.io/tokei/lines/github.com/steff393/wbec?color=blue&style=flat-square)  
  
Die Heidelberg Wallbox Energy Control ist eine hochwertige Ladestation, bietet aber nur Modbus RTU als Schnittstelle.  
Ziel des Projekts ist es, eine WLAN-Schnittstelle zu entwickeln, die gleichzeitig die Modbus-Leader-Funktion übernimmt.  

## Funktionen
- Anbindung an openWB, EVCC, Solaranzeige (per tlw. Emulation der go-eCharger API)
- MQTT-Kommunikation mit openWB und EVCC (ideal für mehrere Ladestationen)
- Steuerbar per Android App [Wallbox Steuerung](https://android.chk.digital/ecar-charger-control/) 
- PV-Überschussladen, Zielladen, etc. mit den o.g. Steuerungen
- RFID-Kartenleser zur Freischaltung der Wallbox mit gültiger Karte/Chip (spezielle HW nötig, s. Wiki)  
- Ansteuerung aller verbundenen Ladestationen (bis zu 16 Follower am Modbus, bis zu 8 openWB-Ladepunkte)
- Lesen/Schreiben der Modbus-Register über ein JSON-Web-Interface 
- Standby-Funktion der Ladestation einstellbar
- Einfacher Prototyp einer Webseite zur Steuerung
- Einfacher Prototyp eines lokalen Lastmanagements
- Softwareupdate per WLAN (Over The Air), z.B. mit PlatformIO oder einfach per Browser (s. Wiki)
- Access-Point-Modus zur initialen Einstellung des WLANs (SSID/Passwort, s. Wiki)
- Weniger als 1W Strombedarf (trotz Ansteuerung von bis zu 16 Ladestationen)

## Kontakt
Bei Fragen oder wenn ihr Unterstützung braucht gerne einfach eine Mail schicken (wbec393@gmail.com)    
Bitte schaut auch ins [Wiki](https://github.com/steff393/wbec/wiki).  

## Bilder
<p align="center"> 
  <img src="/images/wbec_pcb.jpg"> 
</p>
  
  
  
# wbec
Wifi interface to Heidelberg **W**all**B**ox **E**nergy **C**ontrol using ESP8266  
  
The Heidelberg Wallbox Energy Control is a high quality wallbox, but it offers only a Modbus RTU interface.
Goal of this project is to establish an Wifi interface, which also acts as Modbus master.  

## Features
- Works with openWB, EVCC, Solaranzeige (by emulation of parts of the go-eCharger API)
- Support of MQTT communication to openWB and EVCC (perfect for multiple wallboxes)
- Support of Android App [Wallbox Steuerung](https://android.chk.digital/ecar-charger-control/)  
- RFID card reader for unlocking the wallbox only with valid card/chip (needs special HW, s. Wiki)  
- Prepared for supporting up to all 16 connected boxes (up to 8 openWB load points)
- Modbus registers can be read/written via JSON web interface
- Standby of Wallbox can be configured
- Simple prototype of a web interface
- Simple prototype of a local load management
- Update via WiFi (OTA), e.g. with PlatformIO or simply via Web browser (s. Wiki)
- Access point mode, to configure your WiFi network/password (s. Wiki)
- < 1W power consumption (for controlling up to 16 boxes)

## Contact
In case of any questions or in case you need support, feel free to send a mail (wbec393@gmail.com)  ;-)  
Please also take a look to the [Wiki](https://github.com/steff393/wbec/wiki).

## Switch configuration of wallbox
S1 >= 5 (16A max)  
S2 = 0000  
S3 = 0 (6A min)  
S4 = 0001 (slave address, Bus-ID)  
S5 = 0000  
S6 = 0100 (terminator 120 Ohm, only on last box)  

## Examples
Simple web interface (example with powerfox setup):  
`http://x.x.x.x/web.html` 
<p align="center"> 
  <img src="https://i.ibb.co/DtZC9tp/wbec-Web-Interface2.png"> 
</p>

Get current status (here for 2 configured wallboxes, but only 1 connected):
```c++
http://192.168.xx.yy/json

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

Set allowed current:
```c++
http://192.168.xx.yy/json?currLim=120      --> set current limit to 12A (on the box with id=0, i.e. ModBus Bus-ID=1)
http://192.168.xx.yy/json?currLim=60&id=2  --> set current limit to 6A on the box with id=2 (i.e. ModBus Bus-ID=3)
```

Set Watchdog timeout:
```c++
http://192.168.xx.yy/json?wdTmOut=20000
```

## go-eCharger API
wbec can partly emulate the API of Go-eCharger (https://github.com/goecharger/go-eCharger-API-v1) via the following HTTP commands:
```c++
Read:
http://x.x.x.x/status
{"car":"1","alw":"1","amp":"6","err":"0","stp":"0","tmp":"307","dws":"5955","ubi":"0","eto":"59","nrg":[231,232,234,0,0,0,0,0,0,0,0,0,0,0,0,0],"fwv":"40"}
{"version":"B","car":"1","err":"0","alw":"1","amp":"6","amx":"6","stp":"0","pha":"63","tmp":"307","dws":"0","dwo":"0","uby":"0","eto":"59","nrg":[233,234,233,0,0,0,0,0,0,0,0,0,0,0,0,0],"fwv":"40","sse":"123456","ama":"16","ust":"2"}

Write:
http://x.x.x.x/mqtt?payload=...
```

This offers a simple way to integrate wbec into Energy Management Systems, which support go-eCharger, but not the Heidelberg Energy Control, such as EVCC or Solaranzeige.  

## Credits
Third-party libraries included/adapted in wbec:
- [modbus-esp8266](https://github.com/emelianov/modbus-esp8266)
- [ESP Async WebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [PubSubClient](https://github.com/knolleary/PubSubClient)
- [NTPClient](https://github.com/arduino-libraries/NTPClient)
- [MFRC522](https://github.com/miguelbalboa/MFRC522)
- [RTCVars](https://github.com/highno/RTCVars)
- [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets)
- [OTA via http](https://gist.github.com/JMishou/60cb762047b735685e8a09cd2eb42a60)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [Web Interface](https://RandomNerdTutorials.com)
- [A Beginner's Guide to the ESP8266 - article](https://github.com/tttapa/ESP8266)

Special thanks also to the early testers and supporters: mli987, profex1337, Clanchef and many more!

## Support the project
You like wbec? Please [star this project on GitHub](https://github.com/steff393/wbec/stargazers)!
