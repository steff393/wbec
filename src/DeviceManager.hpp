// Copyright (c) 2023 mincequi, MIT license

#pragma once

#include <cstring>
#include <list>
#include <memory>

#include <SunspecMeter.hpp>
#include <globalConfig.h>

class DeviceManager {
public:
    void setup() {
        if (strcmp(cfgMeterIp, "") != 0) {
            switch (cfgMeterType) {
            case MeterType::SunSpecWyeConnect:
            case MeterType::SunSpecWyeElgris: {
                IPAddress ip;
                ip.fromString(cfgMeterIp);
                _devices.push_back(std::make_unique<SunspecMeter>(ip, cfgMeterType));
                _devices.back()->setup();
                break;
            }
            default:
                break;
            }
        }
	}

    void loop() {
        auto ts = millis();
        for (const auto& d : _devices) {
            d->loop(ts);
        }
    }

private:
    std::list<AbstractDevicePtr> _devices;
};