# wbec
Wifi interface to Heidelberg Wallbox Energy Control using ESP8266  
  
The Heidelberg Wallbox Energy Control is a high quality wallbox, but it offers only a Modbus interface.
Goal of this project is to establish an Wifi interface, which also acts as Modbus master (for local external load management).  
Focus is on price: < 10 Euro (only NodeMCU and RS485 adapter needed)

## Features (as of now)
- Communication via Modbus working
- Registers can be read and are transmitted to serial debug terminal
- Registers can be read/written via JSON web interface
- Standby of Wallbox is inhibited by sending every 10s the register 257 with value 4
- Update via WiFi (OTA)
- Simple prototype of a local load management

## Contact
In case of any questions, feel free to open an issue ;-)

## Materials
- Heidelberg Wallbox Energy Control
- NodeMCU, e.g. https://www.amazon.de/AZDelivery-NodeMCU-ESP8266-ESP-12E-Development/dp/B0754HWZSQ/ref=sr_1_4?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=NodeMCU&qid=1618435569&sr=8-4
- TTL-RS485-Adapter, e.g. https://www.amazon.de/gp/product/B07DK4QG6H/ref=ppx_yo_dt_b_asin_title_o03_s00?ie=UTF8&psc=1

## Connections
RS485-Adapter --> NodeMCU  
DI --> D1  
DE --> D5  
RE --> D5  
RO --> D2  
VCC --> 3,3V  
GND --> GND  
A/B --> A/B of wallbox  

## Switch configuration of wallbox
S1 > 5 (16A max)  
S2 = 0000  
S3 = 0 (6A min)  
S4 = 0001 (slave address, Bus-ID)  
S5 = 0000  
S6 = 1000 (terminator 120 Ohm, only on last box)  

## Examples

Get current status:
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
http://192.168.xx.yy/json?currLim=120      --> to set current limit to 12A (on the box with id=0, i.e. ModBus Bus-ID=1)
http://192.168.xx.yy/json?currLim=60&id=2  --> to set current limit to 6A on the box with id=2 (i.e. ModBus Bus-ID=3)
```

Set Watchdog timeout:
```c++
http://192.168.xx.yy/json?wdTmOut=20000
```
