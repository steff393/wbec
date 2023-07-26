// Copyright (c) 2023 mincequi, MIT license

#pragma once

#include <cstdint>

class AbstractDevice {
public:
    void setup() {
        doSetup();
    }

    void loop(uint32_t ts) {
        if (_ts + _loopInterval < ts) {
            _ts = ts;
            doLoop();
        }
    }

    void setLoopInterval(uint32_t ms) {
        _loopInterval = ms;
    }

protected:
    virtual void doSetup() = 0;
    virtual void doLoop() = 0;

private:
    uint32_t _ts = 0;
    uint32_t _loopInterval = 10000;  // [ms]
};

using AbstractDevicePtr = std::unique_ptr<AbstractDevice>;