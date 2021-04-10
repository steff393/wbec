# wbec
Wifi interface to Heidelberg Wallbox Energy Control using ESP8266

## Materials
- Heidelberg Wallbox Energy Control
- NodeMCU
- TTL-RS485-Adapter, e.g. https://www.amazon.de/gp/product/B07DK4QG6H/ref=ppx_yo_dt_b_asin_title_o03_s00?ie=UTF8&psc=1

## Connections
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
