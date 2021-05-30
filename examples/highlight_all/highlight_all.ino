/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Description: 
 * Starts a DMX Output on GPIO pin 0 and sets all channels to max (255)
 */

#include <Arduino.h>
#include <DmxOutput.h>

// Declare an instance of the DMX Output
DmxOutput dmx;

// Create a universe that we want to send.
// The universe must be maximum 512 bytes + 1 byte of start code
#define UNIVERSE_LENGTH 512
uint8_t universe[UNIVERSE_LENGTH + 1];

void setup()
{
    // Start the DMX Output on GPIO-pin 0
    dmx.begin(0);

    // Set all channels in the universe to the max allowed value (512)
    for (int i = 1; i < UNIVERSE_LENGTH + 1; i++)
    {
        universe[i] = 255;
    }
}

void loop()
{
    // Send out universe on GPIO-pin 1
    dmx.write(universe, UNIVERSE_LENGTH);

    while (dmx.busy())
    {
        /* Do nothing while the DMX frame transmits */
    }

    // delay a millisecond for stability (Not strictly necessary)
    delay(1);
}