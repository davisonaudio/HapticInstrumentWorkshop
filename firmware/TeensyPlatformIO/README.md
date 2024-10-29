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