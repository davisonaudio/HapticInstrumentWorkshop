# HapticInstrumentWorkshop
Repository for the self-sensing vibrotactile haptic instrument design project. Part of the Augmented Instruments Lab at Imperial College London

# Kit Contents

- 1x Teensy 4.0 Microcontroller development board with headers
- 1x custom amplifier PCB (using MAX98789 amplifier)
- 1x Dayton Audio EX32VBDS-4 Voice Coil Transducer
- 1x 9V DC 5.1mm barrel jack power supply (centre-positive)
- 1x USB A to micro USB cable

# Getting Started


# Hacking the Hardware

# Hacking the Example Software

# Hacking the Firmware
For advanced users, the firmware running on the Teensy can be modified and recompiled. All of the required files are within the firmware folder of this repo.

The firmware is developed for the Teensy using [PlatformIO](https://platformio.org). This provides more flexibility than using the Arduino IDE (as is often used with Teensy and similar boards), but still allows use of the Teensy system's wide array of useful C++ libraries. It also avoids the increased complexities of utilising a proper embedded development environment (such as NXP's MCUXpresso for the Teensy 4.0's NXP RT1060 microcontroller).

Visual Studio Code is required for PlatformIO development. For further details of development environment setup please refer the the readme within the firmware folder.