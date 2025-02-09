/*

MidiComms.h

Author: Matt Davison
Date: 07/11/2024

*/

#pragma once


#include <math.h>

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

    // USB midi library parses pitch bend vals as centred around 0, from -8192 to 8191
    static constexpr int NUM_PITCH_BEND_VALUES = 16384 ; //2^14
    static constexpr int MIN_PITCH_BEND_VALUE = - 8192 ;
    static constexpr int MAX_PITCH_BEND_VALUE = 8191 ;

    static sample_t pitchBendToNormalised(int pitch_bend_val)
    {
        return ((pitch_bend_val < 0
        ? -static_cast<sample_t>(pitch_bend_val) / MIN_PITCH_BEND_VALUE
        :  static_cast<sample_t>(pitch_bend_val) / MAX_PITCH_BEND_VALUE) / 2) + 0.5;
    }
};
