/*

TeensyEeprom.h

Author: Matt Davison
Date: 13/11/2024

For more info on the Teensy EEPROM see: https://www.pjrc.com/teensy/td_libs_EEPROM.html

*/

#pragma once

//Include the Arduino EEPROM library
#include "EEPROM.h"


class TeensyEeprom
{
    public:
        static const int NUM_EEPROM_BYTES = 1080; //Correct for Teensy 4.0

        enum class SaveParameters
        {

        };

    private:
        void writeFloat(unsigned int eeprom_address, float float_to_write)
        {

        }

        /*
         *
         *
         */
        float readFloat(unsigned int eeprom_address)
        {

        }
};
