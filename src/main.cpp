#include <Arduino.h>
#include <ModbusRTU.h>
#include <SoftwareSerial.h>
// receivePin, transmitPin, inverse_logic, bufSize, isrBufSize
// connect RX to NodeMCU D2 (GPIO4), TX to NodeMCU D1 (GPIO5)
SoftwareSerial S(4, 5);

ModbusRTU mb;

bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
  return true;
}

void setup() {
  Serial.begin(115200);
  S.begin(19200, SWSERIAL_8E1);       // Wallbox Energy Control uses 19.200 bit/sec, 8 data bit, 1 parity bit (even), 1 stop bit
  mb.begin(&S, 14);                   // GPIO14, NodeMCU pin D5 --> connect to DE & RE
  mb.master();
}

uint16_t result[5];

void loop() {
  if (!mb.slave()) {
    mb.readIreg(1, 8, result, 5, cbWrite);
    Serial.printf_P("result: 0x%02X, test: %d\n", result[0], 0x42);
    Serial.printf_P("result: 0x%02X, test: %d\n", result[1], 0x42);
    Serial.printf_P("result: 0x%02X, test: %d\n", result[2], 0x42);
    Serial.printf_P("result: 0x%02X, test: %d\n", result[3], 0x42);
    Serial.printf_P("result: 0x%02X, test: %d\n", result[4], 0x42);
  }
  mb.task();
  delay(500);
  yield();
}