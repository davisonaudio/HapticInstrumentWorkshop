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

    static constexpr int NUM_LEDS = 10;

    TeensySlider(TwoWire* i2c_interface = &Wire)
    {
        m_i2c_interface = i2c_interface;
    }

    uint8_t readPot(uint8_t pot_number)
    {
        m_i2c_interface->beginTransmission(m_i2c_address);
        m_i2c_interface->write(static_cast<uint8_t>(TxType::POT));
        m_i2c_interface->write(pot_number);
        m_i2c_interface->endTransmission();
        // if (!m_i2c_interface->endTransmission(false))
        // {
        //     printf("Tx error!!\r\n");
        //     return 0; //Error in i2c transmission
        // }
        //delayMicroseconds(900);
        uint8_t bytes_ret = m_i2c_interface->requestFrom(m_i2c_address, 1);
        //printf("Bytes returned: %d\r\n",bytes_ret);
        uint8_t pot_val = 0;
        //delayMicroseconds(90);
        while(m_i2c_interface->available())
        {
            pot_val = m_i2c_interface->read();
        }
        return pot_val;
    }

    uint8_t getSwitchPressCount(uint8_t switch_number)
    {
        m_i2c_interface->beginTransmission(m_i2c_address);
        m_i2c_interface->write(static_cast<uint8_t>(TxType::SWITCH));
        m_i2c_interface->write(switch_number);
        m_i2c_interface->endTransmission(false);
        m_i2c_interface->requestFrom((int)m_i2c_address, 1);
        return m_i2c_interface->read();
    }

    void setLedBrightness(uint8_t led_number, uint8_t brightness)
    {
        m_i2c_interface->beginTransmission(m_i2c_address);
        m_i2c_interface->write(static_cast<uint8_t>(TxType::LED));
        m_i2c_interface->write(led_number);
        m_i2c_interface->write(brightness);
        m_i2c_interface->endTransmission();
    }

    void setLedBar(uint8_t num_leds_lit, uint8_t brightness, bool reverse_direction = false)
    {
        if (num_leds_lit > NUM_LEDS){num_leds_lit = NUM_LEDS;}

        
        for (int i = 0 ; i < NUM_LEDS ; i++)
        {
            int led_brightness = 0;
            if (i < num_leds_lit){led_brightness = brightness;}
            int led_index = i;
            if (reverse_direction){led_index = NUM_LEDS - 1 - i;}
            setLedBrightness(led_index, led_brightness);

        }
    }

    uint8_t getFirmwareVersion()
    {
        m_i2c_interface->beginTransmission(m_i2c_address);
        m_i2c_interface->write(static_cast<uint8_t>(TxType::FIRMWARE_VERSION));
        m_i2c_interface->endTransmission();
        m_i2c_interface->requestFrom((int)m_i2c_address, 1);
        return m_i2c_interface->read();
    }


    private:

    enum class TxType{
        FIRMWARE_VERSION,
        POT,
        SWITCH,
        LED
    };

    TwoWire* m_i2c_interface;

    static constexpr uint8_t m_i2c_address = 0x55;

};