#include "ip5306.h"

#define IP5306_ADDR 0x75

#define REG_SYS_CTL0                0x00
#define REG_SYS_CTL1                0x01
#define REG_SYS_CTL2                0x02

#define REG_READ0                   0x70
#define REG_READ1                   0x71

#define REG_BAT_LEVEL               0x78

IP5306::IP5306(TwoWire& wire)
{
    m_wire = &wire;
}

bool IP5306::begin()
{
    return isPresent();
}

bool IP5306::isPresent()
{
    m_wire->beginTransmission(IP5306_ADDR);
    return (m_wire->endTransmission() == 0);
}

bool IP5306::readReg(uint8_t reg, uint8_t& value)
{
    m_wire->beginTransmission(IP5306_ADDR);
    m_wire->write(reg);

    if (m_wire->endTransmission(false) != 0)
    {
        return false;
    }

    if (m_wire->requestFrom(IP5306_ADDR, (uint8_t)1) != 1)
    {
        return false;
    }

    value = m_wire->read();

    return true;
}

bool IP5306::writeReg(uint8_t reg, uint8_t value)
{
    m_wire->beginTransmission(IP5306_ADDR);
    m_wire->write(reg);
    m_wire->write(value);

    return (m_wire->endTransmission() == 0);
}

bool IP5306::updateBit(uint8_t reg, uint8_t bit, bool value)
{
    uint8_t data;

    if (!readReg(reg, data))
    {
        return false;
    }

    if (value)
    {
        data |= (1U << bit);
    }
    else
    {
        data &= ~(1U << bit);
    }

    return writeReg(reg, data);
}

bool IP5306::setPowerOnLoad(bool enable)
{
    return updateBit(REG_SYS_CTL0, 1, enable);
}

bool IP5306::isPowerOnLoadEnabled()
{
    uint8_t data;

    if (!readReg(REG_SYS_CTL0, data))
    {
        return false;
    }

    return (data & (1U << 1)) != 0;
}

bool IP5306::setChargerEnabled(bool enable)
{
    return updateBit(REG_SYS_CTL0, 4, enable);
}

bool IP5306::isCharging()
{
    uint8_t data;

    if (!readReg(REG_READ0, data))
    {
        return false;
    }

    return (data & (1U << 3)) != 0;
}

bool IP5306::isChargeComplete()
{
    uint8_t data;

    if (!readReg(REG_READ1, data))
    {
        return false;
    }

    return (data & (1U << 3)) != 0;
}

IP5306::PowerMode IP5306::getPowerMode()
{
    if (isChargeComplete())
    {
        return PowerMode::ChargeComplete;
    }

    if (isCharging())
    {
        return PowerMode::Charging;
    }

    uint8_t data;

    if (!readReg(REG_READ0, data))
    {
        return PowerMode::Unknown;
    }

    if (data & (1U << 0))
    {
        return PowerMode::Discharging;
    }

    return PowerMode::Standby;
}

bool IP5306::isHeavyLoad()
{
    uint8_t data;

    if (!readReg(REG_READ1, data))
    {
        return false;
    }

    return (data & (1U << 0)) != 0;
}

IP5306::ShutdownTime IP5306::getLightLoadShutdownTime()
{
    uint8_t data;

    if (!readReg(REG_SYS_CTL2, data))
    {
        return ShutdownTime::Sec8;
    }

    uint8_t value = (data >> 2) & 0x03;

    switch (value)
    {
        case 0:
            return ShutdownTime::Sec8;

        case 1:
            return ShutdownTime::Sec16;

        case 2:
            return ShutdownTime::Sec32;

        case 3:
            return ShutdownTime::Sec64;

        default:
            return ShutdownTime::Sec8;
    }
}

bool IP5306::setLightLoadShutdownTime(ShutdownTime time)
{
    uint8_t data;

    if (!readReg(REG_SYS_CTL2, data))
    {
        return false;
    }

    data &= ~(0x03 << 2);

    switch (time)
    {
        case ShutdownTime::Sec8:
            data |= (0 << 2);
            break;

        case ShutdownTime::Sec16:
            data |= (1 << 2);
            break;

        case ShutdownTime::Sec32:
            data |= (2 << 2);
            break;

        case ShutdownTime::Sec64:
            data |= (3 << 2);
            break;
    }

    return writeReg(REG_SYS_CTL2, data);
}

uint8_t IP5306::getBatteryPercent()
{
    uint8_t data;

    if (!readReg(REG_BAT_LEVEL, data))
    {
        return 0;
    }

    /*
     * Typical LED gauge mapping:
     * bit0 = 25%
     * bit1 = 50%
     * bit2 = 75%
     * bit3 = 100%
     */

    if (data & 0x08)
    {
        return 100;
    }

    if (data & 0x04)
    {
        return 75;
    }

    if (data & 0x02)
    {
        return 50;
    }

    if (data & 0x01)
    {
        return 25;
    }

    return 0;
}