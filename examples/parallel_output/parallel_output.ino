
/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Description: 
 * Starts a 8 DMX Output on GPIO pins 0-7 
 */

#include <Arduino.h>
#include <DmxOutput.h>

// Declare 8 instances of the DMX output
DmxOutput dmxOutputs[8];

// Create a universe that we want to send in parallel on all 8 outputs.
// The universe must be maximum 512 bytes + 1 byte for the start
#define UNIVERSE_LENGTH 512
uint8_t universe[UNIVERSE_LENGTH + 1];

void setup()
{
    // Start the DMX outputs on GPIO-pins 0-7.
    // Only 4 outputs can run on a single PIO instance, so
    // the 8 outputs are divided onto the two PIO instances
    // pio0 and pio1

    for (int i = 0; i < 4; i++)
    {
        dmxOutputs[i].begin(i, pio0);
    }
    for (int i = 4; i < 8; i++)
    {
        dmxOutputs[i].begin(i, pio1);
    }
}

void loop()
{
    // Send out universe on all 8 DMX outputs
    for (int i = 0; i < 8; i++)
    {
        dmxOutputs[i].write(universe, UNIVERSE_LENGTH + 1);
    }
    for (int i = 0; i < 8; i++)
    {
        while (dmxOutputs[i].busy())
        {
            // Wait patiently until all outputs are done transmitting
        }
    }

    // delay a millisecond for stability (Not strictly necessary)
    delay(1);
}