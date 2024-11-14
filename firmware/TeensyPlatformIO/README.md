# Self Sensing Haptic Transducer Teensy

Based on this repo: https://github.com/davisonaudio/self-sensing-haptic-transducer/tree/main
Ported from Bela to teensy

# To Use
- Set up platformio environment locally. See here for instructions: https://github.com/dsgong/max98389_teensy
- Measure the impedance of the voice coil you are using and obtain a digital filter that models the admittance response. Change these values in main.cpp.
- Upload to teensy.

# Hardware
The setup shown in this project consists of the following hardware:

- Teensy 4.0 or 4.1
- Audio amplifier with current and voltage sense feedback via i2s. I used the max98389 amplifier.
- A voice coil actuator - this project should work with many VCAs, surface transducer style VCAs (designed to turn a surface into a speaker) are what have primarily been used for testing. The default filter values correspond to a DAEX25Q-4. 
- See schematic.pdf for an example schematic


# MIDI Control


# EEPROM
The Teensy 4.0 has 1080 bytes of EEPROM memory available. This is used in this project to store tuning parameters to enable persistence after a power cycle.

The parameters are saved to EEPROM when the device receives a "Store to EEPROM" MIDI message (see MIDI documentation for more info).

The EEPROM values are loaded at the beginning of the firmware programme.

Each value is stored as a 32 bit value across 4 consecutive bytes, using little-endianness - i.e. the LSB of the value will be at EEPROM address n+0 with the MSB at address n+3.

## EEPROM Address Map

| Base Address       | Parameter    | Channel Num | Units | Notes |
| ------------------ | ------------ | ----------- | ----- | ----- |
| Resonant Frequency |              | 1           |       |       |
| Peak Gain          |              | 2           |       |       |
| Resonance Q        |              | 3           |       |       |
| Tone Level         |              | 4           |       |       |
|                    |              |             |       |       |
|                    |              |             |       |       |
|                    |              |             |       |       |
|                    |              |             |       |       |
|                    |              |             |       |       |
|                    |              |             |       |       |