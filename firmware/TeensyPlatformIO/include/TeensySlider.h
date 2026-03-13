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
        Wire1.beginTransmission(m_i2c_address);
        Wire1.write(static_cast<uint8_t>(TxType::POT));
        Wire1.write(pot_number);
        if (Wire1.endTransmission() != 0)
        {
            printf("Tx error!!\r\n");
            return 0; //Error in i2c transmission
        }
        uint8_t bytes_ret = Wire1.requestFrom(m_i2c_address, 1);
        printf("Bytes returned: %d\r\n",bytes_ret);
        uint8_t pot_val = 0;
        while(Wire1.available())
        {
            pot_val = Wire1.read();
        }
        return pot_val;
    }

    uint8_t getSwitchPressCount(uint8_t switch_number)
    {
        Wire1.beginTransmission(m_i2c_address);
        Wire1.write(static_cast<uint8_t>(TxType::SWITCH));
        Wire1.write(switch_number);
        if (Wire1.endTransmission() != 0)
        {
            printf("Tx error!!\r\n");
            return 0; //Error in i2c transmission
        }
        Wire1.requestFrom((int)m_i2c_address, 1);
        uint8_t switch_count = 0;
        while(Wire1.available())
        {
            switch_count = Wire1.read();
        }
        return switch_count;
    }

    void setLedBrightness(uint8_t led_number, uint8_t brightness)
    {
        Wire1.beginTransmission(m_i2c_address);
        Wire1.write(static_cast<uint8_t>(TxType::LED));
        Wire1.write(led_number);
        Wire1.write(brightness);
        Wire1.endTransmission();
    }

    uint8_t getFirmwareVersion()
    {
        Wire1.beginTransmission(m_i2c_address);
        Wire1.write(static_cast<uint8_t>(TxType::FIRMWARE_VERSION));
        Wire1.endTransmission();
        Wire1.requestFrom((int)m_i2c_address, 1);
        return Wire1.read();
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