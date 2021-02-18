#include <stdio.h>
#include "pico/stdlib.h"
#include "rpi-pico-dmx/dmx.hpp"

void simple_send()
{
    stdio_init_all();
    printf("DMX Transmitter example \n");
    printf("simple send example \n");

    // Choose a pin that the DMX signal should be transmitted on
    const uint dmx_pin = 0;

    // Declare a DMX transmitter instance
    Dmx dmx;

    // Initialize the DMX transmitter instance
    if (!dmx.begin(dmx_pin))
    {
        printf("Something went wrong!");
    }

    // Create a universe that we can send using the DMX transmitter
    // We want 512 channels in our universe and reserve the first byte for
    // the start code 0x00. The universe is initialized to all zeros
    const uint universe_size = 513;
    uint8_t universe[universe_size];

    // Transmit the universe
    dmx.write(universe, universe_size);
    printf("Universe transmitting... \n");

    // The processor can now do other tasks while the universe is being sent
    // Here we busy wait until the transmission is done
    while (dmx.busy())
    {
        /* Patiently waiting for the DMX universe to be sent */
    }

    printf("Universe transmission done! \n");

    // release our DMX transmitter resource
    dmx.end();
}