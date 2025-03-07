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
    /*
     * REGISTER MAP:
     * Floats stored first, then bytes
     */
    
    public:

        enum class FloatParameters
        {
            RESONANT_FREQUENCY_HZ = 0,
            RESONANT_GAIN_DB,
            RESONANT_Q,
            TONE_LEVEL_DB,
            INDUCTANCE_FILTER_COEFFICIENT,
            BROADBAND_GAIN_DB,
            SAMPLE_RATE_HZ,
            UNDAMPED_CALIBRATION_VALUE,
            DAMPED_CALIBRATION_VALUE,

            NUM_FLOAT_PARAMETERS
        };

        enum class ByteParameters
        {
            SERIAL_NUMBER = 0,
            BOARD_REVISION,
            LAST_SAVED_MAJ_VERSION,
            LAST_SAVED_MIN_VERSION,
            GOERTZEL_WINDOW_LENGTH,
            
            NUM_BYTE_PARAMETERS = 128 // Leave buffer to enable additional byte parameters while maintaining backwards compatibility
        };

        enum class BoardRevision
        { //Amplifier PCB revision (affects i2s pin assignment)
            REV_A = 0,
            REV_B
        };

        static const int NUM_EEPROM_BYTES = 1080; //Correct for Teensy 4.0
        static constexpr int BYTES_PER_FLOAT = sizeof(float);

        
        static constexpr int BYTES_BASE = 0;
        static constexpr int FLOATS_BASE = BYTES_BASE + static_cast<int>(ByteParameters::NUM_BYTE_PARAMETERS);

        void write(ByteParameters byte_parameter, uint8_t value)
        {
            EEPROM.write(getEepromAddress(byte_parameter), value);
        }
        uint8_t read(ByteParameters byte_parameter)
        {
            return EEPROM.read(getEepromAddress(byte_parameter));
        }

        void write(FloatParameters float_parameter, float value)
        {
            writeFloat(getEepromAddress(float_parameter), value);
        }

        float read(FloatParameters float_parameter)
        {
            return readFloat(getEepromAddress(float_parameter));
        }

        void writeBoardRevision(BoardRevision board_revision)
        {
            write(ByteParameters::BOARD_REVISION, static_cast<uint8_t>(board_revision));
        }

        BoardRevision readBoardRevision()
        {
            return static_cast<BoardRevision>(read(ByteParameters::BOARD_REVISION));
        }


    private:

        //Union is used to convert between float and bytes (sorry)
        union FloatByteConverter
        {
            float float_val;
            uint8_t byte_vals[sizeof(float)];
        };

        /*
         *
         *
         */
        void writeFloat(unsigned int eeprom_address, float float_to_write)
        {
            FloatByteConverter float_to_byte;
            float_to_byte.float_val = float_to_write;

            for (unsigned int i = 0 ; i < sizeof(float) ; i++ )
            {
                EEPROM.write(eeprom_address + i, float_to_byte.byte_vals[i]);
            }
        }

        /*
         *
         *
         */
        float readFloat(unsigned int eeprom_address)
        {
            FloatByteConverter bytes_to_float;

            for (unsigned int i = 0 ; i < sizeof(float) ; i++ )
            {
                bytes_to_float.byte_vals[i] = EEPROM.read(eeprom_address + i);
            }

            return bytes_to_float.float_val;
        }

        unsigned int getEepromAddress(FloatParameters eeprom_parameter)
        {
            return FLOATS_BASE + (BYTES_PER_FLOAT * (static_cast<unsigned int>(eeprom_parameter)));
        }

        unsigned int getEepromAddress(ByteParameters eeprom_parameter)
        {
            return BYTES_BASE + static_cast<unsigned int>(eeprom_parameter);
        }

};
