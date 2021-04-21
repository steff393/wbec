# wbec
Wifi interface to Heidelberg Wallbox Energy Control using ESP8266  
  
The Heidelberg Wallbox Energy Control is a high quality wallbox, but it offers only a Modbus interface.
Goal of this project is to establish an Wifi interface, which also acts as Modbus master (for local external load management) and to rebuild a part of the Combox functionality.  

## Features (as of now)
- Communication via Modbus working
- Registers can be read and are transmitted to serial debug terminal
- Registers can be read/written via JSON web interface
- Standby of Wallbox is inhibited by sending every 10s the register 257 with value 4
- Prepared for supporting up to all 16 connected boxes
- Simple prototype of a local load management
- Update via WiFi (OTA)
- Access point mode, to configure your WiFi network/password

## Contact
In case of any questions, feel free to send a mail (wbec393@gmail.com) or open an issue  ;-)

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
S6 = 1000 (terminator 120 Ohm, only on last box)  

## Examples
First simple web interface:
<p align="center">
  <img src="https://i.ibb.co/kKSkL1X/wbec-Web-Interface.png">
</p>

Get current status (here for 3 configured wallboxes, but only 1 connected):
```c++
http://192.168.xx.yy/json

{
  "modbus": {
    "cfg": {
      "cycleTm": 10000
    },
    "state": {
      "lastTm": 185055,
      "millis": 186817
    }
  },
  "boxes": [
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
      "currFs": 0
    },
    {
      "busId": 2,
      "version": "0",
      "chgStat": 0,
      "currL1": 0,
      "currL2": 0,
      "currL3": 0,
      "pcbTemp": 0,
      "voltL1": 0,
      "voltL2": 0,
      "voltL3": 0,
      "extLock": 0,
      "power": 0,
      "energyP": 0,
      "energyI": 0,
      "currMax": 0,
      "currMin": 0,
      "logStr": "",
      "wdTmOut": 0,
      "standby": 0,
      "remLock": 0,
      "currLim": 0,
      "currFs": 0
    },
    {
      "busId": 3,
      "version": "0",
      "chgStat": 0,
      "currL1": 0,
      "currL2": 0,
      "currL3": 0,
      "pcbTemp": 0,
      "voltL1": 0,
      "voltL2": 0,
      "voltL3": 0,
      "extLock": 0,
      "power": 0,
      "energyP": 0,
      "energyI": 0,
      "currMax": 0,
      "currMin": 0,
      "logStr": "",
      "wdTmOut": 0,
      "standby": 0,
      "remLock": 0,
      "currLim": 0,
      "currFs": 0
    }
  ],
  "load": [
    0,
    0,
    0
  ],
  "resCode": [
    "0",
    "e4",
    "e4"
  ]
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
