# wbec
Wifi interface to Heidelberg Wallbox Energy Control using ESP8266

## Current Status
- Communication via Modbus working
- Registers can be read and are transmitted to serial debug terminal
- Registers can be read via JSON
- Standby of Wallbox is inhibited by sending every 10s the register 257 with value 4
- Update via WiFi

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
S4 = 0001 (slave address)  
S5 = 0000  
S6 = 1000 (terminator 120 Ohm, only on last box)  

## Examples

Get current status:
```c++
http://192.168.xx.yy/json

{
  "modbus": {
    "cfg": {
      "cycleTm": 5000
    },
    "state": {
      "resCode": "0",
      "lastTm": 103717,
      "millis": 104894
    }
  },
  "wbec": [
    {
      "slaveID": 1,
      "version": "108",
      "chgStat": 2,
      "currL1": 0,
      "currL2": 0,
      "currL3": 0,
      "pcbTemp": 322,
      "voltL1": 232,
      "voltL2": 9,
      "voltL3": 9,
      "extLock": 1,
      "power": 0,
      "currMax": 16,
      "currMin": 6,
      "logStr": "<item no> <mfgDate> <serial>",
      "wdTmOut": 15000,
      "standby": 4,
      "remLock": 1,
      "currLim": 0,
      "currFs": 0
    }
  ]
}
```

Set allowed current:
```c++
http://192.168.xx.yy/json?currLim=130
```

Set Watchdog timeout:
```c++
http://192.168.xx.yy/json?wdTmOut=20000
```
