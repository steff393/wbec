# wbec
Wifi interface to Heidelberg **W**all**B**ox **E**nergy **C**ontrol using ESP8266  
  
The Heidelberg Wallbox Energy Control is a high quality wallbox, but it offers only a Modbus RTU interface.
Goal of this project is to establish an Wifi interface, which also acts as Modbus master (for local external load management).  

## Features (as of now)
- Works with openWB, EVCC, Solaranzeige (by emulation of go-eCharger API or via MQTT)
- Support of MQTT communication to openWB
- Prepared for supporting up to all 16 connected boxes (up to 8 openWB load points)
- Modbus registers can be read/written via JSON web interface
- Standby of Wallbox can be configured
- Simple prototype of a web interface
- Simple prototype of a local load management
- Update via WiFi (OTA), e.g. with PlatformIO or simply via Web browser (s. Wiki)
- Access point mode, to configure your WiFi network/password (s. Wiki)
- < 1W power consumption (for controlling up to 16 boxes)

## Contact
In case of any questions, feel free to send a mail (wbec393@gmail.com) or open an issue  ;-)
When you're interested in a ready-to-use black-box, then please send a mail.

## Pictures
<p align="center">
  <img src="/images/wbec.jpg"> <br> <img src="https://user-images.githubusercontent.com/73853447/117866120-0e9e6700-b297-11eb-9cd1-4a33e714bceb.PNG">
</p>

## Switch configuration of wallbox
S1 > 5 (16A max)  
S2 = 0000  
S3 = 0 (6A min)  
S4 = 0001 (slave address, Bus-ID)  
S5 = 0000  
S6 = 0100 (terminator 120 Ohm, only on last box)  

## Examples
First simple web interface (might be removed again in future, as there are better alternatives with openWB, EVCC, ...):
<p align="center">
  <img src="https://i.ibb.co/kKSkL1X/wbec-Web-Interface.png">
</p>

Get current status (here for 2 configured wallboxes, but only 1 connected):
```c++
http://192.168.xx.yy/json

{
  "wbec": {
    "version": "v0.2.0"         // wbec version
    "bldDate": "2021-05-23"     // wbec build date
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

Write:
http://x.x.x.x/mqtt?payload=...
```

This offers a simple way to integrate wbec into Energy Management Systems, which support go-eCharger, but not the Heidelberg Energy Control, such as EVCC or Solaranzeige.  

## Support the project
You like wbec? Please [star this project on GitHub](https://github.com/steff393/wbec/stargazers)!

What? You don't like it but you *love* it?  When wbec helps you in using your wallbox more efficiently, you can also [![](https://img.shields.io/badge/send%20me%20a%20small%20gift-paypal-blue.svg?style=flat-square)](https://paypal.me/steff393)
