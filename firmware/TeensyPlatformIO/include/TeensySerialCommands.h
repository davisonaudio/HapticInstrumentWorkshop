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
    const char* kAnalogModeString = "analog";
    const char* kStandaloneSynthModeString = "synth";
    const char* kLoopbackTestModeString = "loopback";
    const char* kPiezoModeString = "piezo";

    const char* kSaveToEepromString = "save";
    const char* kResetParametersString = "reset_params";
    const char* kHelpString = "help";
    const char* kCalibrateDamped = "calid";
    const char* kCalibrateUndamped = "caliu";
    const char* kInfoString = "info";

    const char* kResonantFreqString = "rf";
    const char* kResonantGainString = "rg";
    const char* kResonantQString = "rq";
    const char* kWidebandGainString =  "wg";
    const char* kToneLevelString =  "tl";
    const char* kLowpassOutputFreq =  "lpo";
    const char* kLowpassInputFreq =  "lpi";

    const char* kHeadphoneLevel = "hpl";
    const char* kActuationLevel = "al";

}


void printSerialHelp()
{
    printf("Serial commands:\r\n");
    printf("For commands with value arguments, sending them without an argument will result in the current value being returned.\r\n");
    printf("%s - Enter normal mode (exit any current error/debug state)\r\n",SerialCommands::kNormalModeString);
    printf("%s - Enter debug mode (changes audio routing, enables additional serial printing)\r\n",SerialCommands::kDebugModeString);
    printf("%s - Enter analog mode - routes audio from the audio shield instead of USB for connecting to external analog synths\r\n",SerialCommands::kAnalogModeString);
    printf("%s - Enter standalone synth mode - uses internal synth of sound generation rather than USB connection\r\n",SerialCommands::kStandaloneSynthModeString);
    printf("%s - Enter loopback test mode (loops USB ins to USB outs and Analog Ins to Analog Outs)\r\n",SerialCommands::kLoopbackTestModeString);
    printf("%s - Enter piezo mode (sends teensy audio shield mic input signal to USB R)\r\n",SerialCommands::kPiezoModeString);

    printf("%s - Save current parameters to EEPROM\r\n",SerialCommands::kSaveToEepromString);
    printf("%s - Reset parameters to factory defaults\r\n",SerialCommands::kResetParametersString);
    printf("%s - Display help message (this message currently displayed)\r\n",SerialCommands::kHelpString);
    printf("%s - Calibrate the damped force sense level\r\n",SerialCommands::kCalibrateDamped);
    printf("%s - Calibrate the undamped force sense level\r\n",SerialCommands::kCalibrateUndamped);
    printf("%s - Print system information (firmware version etc.)\r\n\n",SerialCommands::kInfoString);
    
    printf("%s <resonant_frequency_hz> - Set the resonant frequency in Hz\r\n",SerialCommands::kResonantFreqString);
    printf("%s <resonant_gain_db> - Set the resonance gain in dB\r\n",SerialCommands::kResonantGainString);
    printf("%s <resonant_q> - Set the resonance Q factor\r\n",SerialCommands::kResonantQString);
    printf("%s <wideband_gain_db> - Set the wideband gain in db\r\n",SerialCommands::kWidebandGainString);
    printf("%s <tone_level_db> - Set the tone level in dB\r\n",SerialCommands::kToneLevelString);
    printf("%s <lowpass_cutoff_hz> - Set the cutoff of the output lowpass filter in Hz\r\n",SerialCommands::kLowpassOutputFreq);
    printf("%s <lowpass_cutoff_hz> - Set the cutoff of the input lowpass filter in Hz\r\n",SerialCommands::kLowpassInputFreq);
    printf("%s <headphone_level_dbFS> - Set the headphone output level in dBFS (0 = maximum)\r\n",SerialCommands::kHeadphoneLevel);
    printf("%s <actuation_level_dbFS> - Set the actuation output level in dBFS (0 = maximum)\r\n",SerialCommands::kActuationLevel);
}

