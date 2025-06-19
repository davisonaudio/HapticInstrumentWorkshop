/*
 
TeensySlider.h
 
Author: Matt Davison
Date: 18/06/2025
 
*/

#pragma once

#include <stdint.h>
#include <Wire.h>

class TeensySlider
{
    public:

    uint8_t readPot(uint8_t pot_number)
    {
        Wire.beginTransmission(m_i2c_address);
        Wire.write(static_cast<uint8_t>(TxType::POT));
        Wire.write(pot_number);
        Wire.requestFrom((int)m_i2c_address, 1);
        return Wire.read();
    }

    uint8_t getSwitchPressCount(uint8_t switch_number)
    {
        Wire.beginTransmission(m_i2c_address);
        Wire.write(static_cast<uint8_t>(TxType::SWITCH));
        Wire.write(switch_number);
        Wire.requestFrom((int)m_i2c_address, 1);
        return Wire.read();
    }

    void setLedBrightness(uint8_t led_number, uint8_t brightness)
    {
        Wire.beginTransmission(m_i2c_address);
        Wire.write(static_cast<uint8_t>(TxType::LED));
        Wire.write(led_number);
        Wire.write(brightness);
        Wire.endTransmission();
    }

    uint8_t getFirmwareVersion()
    {
        Wire.beginTransmission(m_i2c_address);
        Wire.write(static_cast<uint8_t>(TxType::FIRMWARE_VERSION));
        Wire.requestFrom((int)m_i2c_address, 1);
        return Wire.read();
    }


    private:

    enum class TxType{
        FIRMWARE_VERSION,
        POT,
        SWITCH,
        LED
    };

    static constexpr uint8_t m_i2c_address = 0x55;

};