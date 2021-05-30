
/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DMX_INPUT_H
#define DMX_INPUT_H

#include <dma.h>
#include <pio.h>

#define DMX_UNIVERSE_SIZE 512
#define DMX_SM_FREQ 1000000

class DmxInput
{
    uint _prgm_offset;
    uint _pin;
    uint _sm;
    PIO _pio;
    uint _start_channel;
    uint _end_channel;

public:
    /*
        All different return codes for the DMX class. Only the SUCCESS
        Return code guarantees that the DMX output instance was properly configured
        and is ready to run
    */
    enum return_code
    {
        SUCCESS = 0,

        // There were no available state machines left in the
        // pio instance.
        ERR_NO_SM_AVAILABLE = -1,

        // There is not enough program memory left in the PIO to fit
        // The DMX PIO program
        ERR_INSUFFICIENT_PRGM_MEM = -2,
    };

    /*
       Starts a new DMX input instance. 
       
       Param: pin
       Any valid GPIO pin on the Pico

       Param: pio
       defaults to pio0. pio0 can run up to 3
       DMX input instances. If you really need more, you can
       run 3 more on pio1  
    */

    return_code begin(uint pin, uint start_channel, uint num_channels, PIO pio = pio0);

    /*
        Read the selected channels from .begin(...) into a buffer.
        Method call blocks until the selected channels have been received

        Param: buffer
        A pointer to the location where the channels should be received 
        The buffer should have a max length of
        513 bytes (1 byte start code + 512 bytes frame). For ordinary
        DMX data frames, the start code should be 0x00.
    */

    void read(uint8_t *buffer);

    /*
        De-inits the DMX input instance. Releases PIO resources. 
        The instance can safely be destroyed after this method is called
    */
    void end();
};

#endif