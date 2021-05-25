/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <Arduino.h>
#include <dmx.h>

// Declare an instance of the DMX transmitter
Dmx dmx;

// Create a universe that we want to send.
// The universe must be maximum 512 bytes
#define UNIVERSE_LENGTH 512
uint8_t universe[UNIVERSE_LENGTH];

void setup()
{
    // Start the DMX Transmitter on GPIO-pin 1
    dmx.begin(1);

    // Set all channels in the universe to the max allowed value (512)
    for (int i = 0; i < UNIVERSE_LENGTH; i++)
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