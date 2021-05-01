# wbec
Wifi interface to Heidelberg Wallbox Energy Control using ESP8266  
  
The Heidelberg Wallbox Energy Control is a high quality wallbox, but it offers only a Modbus interface.
Goal of this project is to establish an Wifi interface, which also acts as Modbus master (for local external load management) and to rebuild a part of the Combox functionality.  

## Features (as of now)
- Integration into openWB and EVCC (under test)
- Communication via Modbus working
- Registers can be read and are transmitted to serial debug terminal
- Registers can be read/written via JSON web interface
- Standby of Wallbox is inhibited by sending every 10s the register 257 with value 4
- Prepared for supporting up to all 16 connected boxes
- Simple prototype of a local load management
- Update via WiFi (OTA), e.g. with PlatformIO
- Access point mode, to configure your WiFi network/password (s. Wiki)

## Contact
In case of any questions, feel free to send a mail (wbec393@gmail.com) or open an issue  ;-)
When you're interested in a ready-to-use black-box, then please send a mail.

## Materials
- Heidelberg Wallbox Energy Control
- NodeMCU, ESP8266
- TTL-RS485-Adapter

<p align="center">
  <img src="/images/wbec.jpg">
</p>

## Switch configuration of wallbox
S1 > 5 (16A max)  
S2 = 0000  
S3 = 0 (6A min)  
S4 = 0001 (slave address, Bus-ID)  
S5 = 0000  
S6 = 0100 (terminator 120 Ohm, only on last box)  

## Examples
First simple web interface:
<p align="center">
  <img src="https://i.ibb.co/kKSkL1X/wbec-Web-Interface.png">
</p>

Get current status (here for 3 configured wallboxes, but only 1 connected):
```c++
http://192.168.xx.yy/json

{
  "wbec": {
    "version": "v0.0.2"
  },
  "box": [
    {
      "busId": 1,
      "version": "108",
      "chgStat": 2,
      "currL1": 0,
      "currL2": 0,
      "currL3": 0,
      "pcbTemp": 333,
      "voltL1": 232,
      "voltL2": 9,
      "voltL3": 9,
      "extLock": 1,
      "power": 0,
      "energyP": 0,
      "energyI": 0.003,
      "currMax": 16,
      "currMin": 6,
      "logStr": "<item no> <mfgDate> <serial>",
      "wdTmOut": 15000,
      "standby": 4,
      "remLock": 1,
      "currLim": 130,
      "currFs": 0,
      "load": 0,
      "resCode": "0"
    },
    {
      "busId": 2,
      "version": "0",
      "chgStat": 0,
      ...
      "load": 0,
      "resCode": "0"
    }
  ],
  "modbus": {
    "state": {
      "lastTm": 2852819,
      "millis": 2855489
    }
  },
  "wifi": {
    "bssid": "00:1F:3F:15:29:7E",
    "rssi": -76,
    "signal": 48,
    "channel": 11
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
