#pragma once

#include <Arduino.h>
#include <Wire.h>

class IP5306
{
public:
    enum class PowerMode
    {
        Standby,
        Charging,
        Discharging,
        ChargeComplete,
        Unknown
    };

    enum class ShutdownTime
    {
        Sec8,
        Sec16,
        Sec32,
        Sec64
    };

    explicit IP5306(TwoWire& wire = Wire);

    bool begin();
    bool isPresent();

    bool isCharging();
    bool isChargeComplete();

    PowerMode getPowerMode();

    bool isHeavyLoad();

    bool setPowerOnLoad(bool enable);
    bool isPowerOnLoadEnabled();

    bool setChargerEnabled(bool enable);

    ShutdownTime getLightLoadShutdownTime();
    bool setLightLoadShutdownTime(ShutdownTime time);

    uint8_t getBatteryPercent();

private:
    bool readReg(uint8_t reg, uint8_t& value);
    bool writeReg(uint8_t reg, uint8_t value);
    bool updateBit(uint8_t reg, uint8_t bit, bool value);

    TwoWire* m_wire;
};