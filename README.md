# HapticInstrumentWorkshop
Repository for the self-sensing vibrotactile haptic instrument design project. Part of the Augmented Instruments Lab at Imperial College London

For more information on using the kit and detailed documentation, visit the [wiki](https://github.com/davisonaudio/HapticInstrumentWorkshop/wiki).

Access the web control panel [here](https://davisonaudio.github.io/HapticInstrumentWorkshop/) for logging information and parameter adjustment.

# Kit Contents

- 1x Teensy 4.0 Microcontroller development board with headers
- 1x custom amplifier PCB (using MAX98789 amplifier)
- 1x Dayton Audio EX32VBDS-4 Voice Coil Transducer
- 1x 9V DC 5.1mm barrel jack power supply (centre-positive)
- 1x USB A to micro USB cable

# Getting Started
The Teensy 4.0 will come preloaded with the required firmware and mounted with its headers onto the top of the amplifier board. 

# Cloning the repo
To get a local copy of this repository (for editing the firmware or hardware files included), run the following command:
> git clone --recurse-submodules https://github.com/davisonaudio/HapticInstrumentWorkshop.git

This will clone the main repo as well as the audio-utils submodule within the firmware folder.

# Hacking the Hardware

It is unlikely that the PCB is something that we be modified as part of this study. For those with sufficient electronics knowledge, however, the design files (schematic and PCB PDFs along with KiCAD files) can be found in the hardware folder of this repo.

# Hacking the Example Software

# Hacking the Firmware
For advanced users, the firmware running on the Teensy can be modified and recompiled. All of the required files are within the firmware folder of this repo.

The firmware is developed for the Teensy using [PlatformIO](https://platformio.org). This provides more flexibility than using the Arduino IDE (as is often used with Teensy and similar boards), but still allows use of the Teensy system's wide array of useful C++ libraries. It also avoids the increased complexities of utilising a proper embedded development environment (such as NXP's MCUXpresso for the Teensy 4.0's NXP RT1060 microcontroller).

Visual Studio Code is required for PlatformIO development. For further details of development environment setup please refer to the wiki.


# Publications
For more information on related research output of this project, please refer to the below publications:

- M. Davison, C. J. Webb, M. Ducceschi and A. P. McPherson. A self-sensing haptic actuator for tactile interaction with physical modelling synthesis. *Proc. International Conference on New Interfaces for Musical Expression (NIME)*, Utrecht, Netherlands. 2024. [PDF](http://instrumentslab.org/data/andrew/davison_nime2024.pdf)

- M. Davison and A. P. McPherson. A self-sensing vibrotactile transducer for bidirectional interaction. *Proc. Eurohaptics* (work in progress), Lille, France. 2024. [PDF](http://instrumentslab.org/data/andrew/davison_eurohaptics2024.pdf)

# Contributors
This project is developed and maintained by Matt Davison. Massive thank you also to the additional contributors who helped this project happen:

- Anna Silver - CAD design of 3D printed elements
- David Gong - Initial PCB design and firmware bring-up of the amplifier board
- Prof. Andrew McPherson - Supervision and guidance throughout
