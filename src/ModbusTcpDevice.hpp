// Copyright (c) 2023 mincequi, MIT license

#pragma once

#include <ModbusTCP.h>

#include "AbstractDevice.hpp"

class ModbusTcpDevice : public AbstractDevice {
public:
    ModbusTcpDevice(const IPAddress& ip) :
        _ip(ip) {
        setLoopInterval(cfgMbCycleTime * 1000);
    }

protected:
    virtual void onLoop() {}
    template<typename T>
    void read(uint16_t offset, T* value) {
        _mb.readHreg(_ip, offset, (uint16_t*)value, sizeof(T)/2);
    }
    template<typename T, size_t N>
    void read(uint16_t offset, std::array<T,N>* value) {
        _mb.readHreg(_ip, offset, (uint16_t*)value->data(), sizeof(T)*N/2);
    }

private:
    void doSetup() override final {
        _mb.client();
    }

    void doLoop() override final {
        if (_mb.isConnected(_ip)) {
            onLoop();
        } else {
            _mb.connect(_ip);
        }
        _mb.task();
    }

    ModbusTCP _mb;
    IPAddress _ip;
};