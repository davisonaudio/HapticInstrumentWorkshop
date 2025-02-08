/*

MidiComms.h

Author: Matt Davison
Date: 07/11/2024

*/

#pragma once

class MidiComms
{
public:
    enum class MessageTypes
    {
        INVALID = 0
    };

    enum class ProgrammeChangeTypes
    {
        SAVE_TO_EEPROM = 0,
        RESET_TO_DEFAULT_PARAMETERS,
        CALIBRATE_DAMPED,
        CALIBRATE_UNDAMPED
    };

    enum class ControlChangeTypes
    {

    };

    enum class PitchBendChannels
    {
        RESONANT_FREQUENCY = 0
    };
};
