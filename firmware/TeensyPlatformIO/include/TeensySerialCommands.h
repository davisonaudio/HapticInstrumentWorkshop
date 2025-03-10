/*

TeensySerialCommands.h

Author: Matt Davison
Date: 10/03/2025

*/

#pragma once

#include <stdio.h>

namespace SerialCommands
{
    const char* kNormalModeString = "normal";
    const char* kDebugModeString = "debug";
    const char* kSaveToEepromString = "save";
    const char* kResetParametersString = "reset_params";
    const char* kHelpString = "help";

    const char* kResonantFreqString = "rf";
    const char* kResonantGainString = "rg";
    const char* kResonantQString = "rq";
    const char* kWidebandGainString =  "wg";
    const char* kToneLevelString =  "tl";
}


void printSerialHelp()
{
    printf("Serial commands:\r\n");
    printf("For commands with value arguments, sending them without an argument will result in the current value being returned.\r\n");
    printf("%s - Enter normal mode (exit any current error/debug state)\r\n",SerialCommands::kNormalModeString);
    printf("%s - Enter debug mode (changes audio routing, enables additional serial printing)\r\n",SerialCommands::kDebugModeString);
    printf("%s - Save current parameters to EEPROM\r\n",SerialCommands::kSaveToEepromString);
    printf("%s - Reset parameters to factory defaults\r\n",SerialCommands::kResetParametersString);
    printf("%s - Display help message (this message currently displayed)\r\n",SerialCommands::kHelpString);
    
    printf("%s <resonant_frequency_hz> - Set the resonant frequency in Hz\r\n",SerialCommands::kResonantFreqString);
    printf("%s <resonant_gain_db> - Set the resonance gain in dB\r\n",SerialCommands::kResonantGainString);
    printf("%s <resonant_q> - Set the resonance Q factor\r\n",SerialCommands::kResonantQString);
    printf("%s <wideband_gain_db> - Set the wideband gain in db\r\n",SerialCommands::kWidebandGainString);
    printf("%s <tone_level_db> - Set the tone level in dB\r\n",SerialCommands::kToneLevelString);
}

