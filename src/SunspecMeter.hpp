// Copyright (c) 2023 mincequi, MIT license

#pragma once

#include "ModbusTcpDevice.hpp"

class SunspecMeter : public ModbusTcpDevice {
public:
    SunspecMeter(const IPAddress& ip, MeterType type) :
        ModbusTcpDevice(ip),
        // Elgris Smart Meters sends "wrong" sign for scale factor
        _expFactor(type == MeterType::SunSpecWyeElgris ? -1 : 1) {
    }

private:
    void onLoop() override {
        int32_t power = _values.at(0) * pow(10, _expFactor * _values.at(4));
        pv_setWatt(power);
        read(40087, &_values);
    }

    int32_t _expFactor = 1;
    std::array<int16_t,5> _values;
};