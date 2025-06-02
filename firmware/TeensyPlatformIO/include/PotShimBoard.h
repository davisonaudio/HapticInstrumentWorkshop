/*

PotShimBoard.h

Author: Matt Davison
Date: 01/06/2025

Deals with reading of pot and button values from external add on board

*/

#pragma once

class PotShimBoard{
    static constexpr int NUM_POTS = 4;

    uint8_t readRawPotValue(int pot_num);
    

};