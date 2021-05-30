/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <Arduino.h>
#include "DmxInput.h"
DmxInput dmxInput;

#define START_CHANNEL 1
#define NUM_CHANNELS 3
uint8_t buffer[NUM_CHANNELS];

void setup()
{
    // Setup our DMX Input to read on GPIO 0, from channel 1 to 3
    dmxInput.begin(0, START_CHANNEL, NUM_CHANNELS);

    // Setup the onboard LED so that we can blink when we receives packets
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    // Read a DMX packet into our buffer
    dmxInput.read(buffer);

    // Print out packet
    Serial.print("Received packet: ");
    for (uint i = 0; i < NUM_CHANNELS; i++)
    {
        Serial.print(buffer[i]);
        Serial.print(", ");
    }
    Serial.println("");

    // Blink the LED to indicate that a packet was received
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10);
    digitalWrite(LED_BUILTIN, LOW);
}