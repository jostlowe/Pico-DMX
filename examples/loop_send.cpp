#include <stdio.h>
#include "pico/stdlib.h"
#include "rpi-pico-dmx/dmx.hpp"

void loop_send()
{
    stdio_init_all();
    printf("DMX Transmitter example \n");
    printf("loop send example \n");

    // Choose a pin that the DMX signal should be transmitted on
    const uint dmx_pin = 0;

    // Declare a DMX transmitter instance
    Dmx dmx;

    // Initialize the DMX transmitter instance
    if (!dmx.begin(dmx_pin))
    {
        printf("Something went wrong! \n");
    }

    // Create a universe that we can send using the DMX transmitter
    // We want 512 channels in our universe and reserve the first byte for
    // the start code 0x00. The universe is initialized to all zeros
    const uint universe_size = 513;
    uint8_t universe[universe_size];

    // Enter a loop, sending the universe with a 1ms delay between each transmission, 200 times
    printf("Sending 200 DMX frames... \n");
    for (uint i = 0; i < 200; i++)
    {
        // Instruct the DMX transmitter to start sending the universe
        dmx.write(universe, universe_size);

        // Wait for completion
        dmx.await();

        // Wait a tiny millisecond (not really necessary for stability)
        sleep_ms(1);
    }
    printf("Universe transmission done! \n");

    // release our DMX transmitter resource
    dmx.end();
}