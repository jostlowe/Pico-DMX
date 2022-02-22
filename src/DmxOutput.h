/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DMX_OUTPUT_H
#define DMX_OUTPUT_H

#if defined(ARDUINO_ARCH_MBED)
  #include <dma.h>
  #include <pio.h>
#else
  #ifdef ARDUINO
    #include <Arduino.h>
  #endif
  #include "hardware/dma.h"
  #include "hardware/pio.h"
#endif

#define DMX_UNIVERSE_SIZE 512
#define DMX_SM_FREQ 1000000

class DmxOutput
{
    uint _prgm_offset;
    uint _pin;
    uint _sm;
    PIO _pio;
    uint _dma;

public:
    /*
        All different return codes for the DMX class. Only the SUCCESS
        Return code guarantees that the DMX transmitter instance was properly configured
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

        // There are no available DMA channels to handle
        // the transfer of DMX data to the PIO
        ERR_NO_DMA_AVAILABLE = -3
    };

    /*
       Starts a new DMX transmitter instance. 
       
       Param: pin
       Any valid GPIO pin on the RPi Pico

       Param: pio
       defaults to pio0. pio0 can run up to 4
       DMX instances. If you really need more, you can
       run 4 more on pio1  
    */

    return_code begin(uint pin, PIO pio = pio0);

    /*
        write a DMX universe to the DMX transmitter instance.
        Returns imediatly after function call and does not block. 
        The status of the DMX transmission can be checked using
        busy() or you can block until the transmission is done 
        using await()

        Param: universe
        A pointer to the location of the DMX frame that should
        be transmitted. the universe should have a max length of
        513 bytes (1 byte start code + 512 bytes frame). For ordinary
        DMX data frames, the start code should be 0x00.

        Param: length
        The number of bytes from the DMX frame that should be 
        transmitted 
    */

    void write(uint8_t *universe, uint length);

    /*
        Checks whether the DMX transmitter is busy sending
        a DMX data frame. Returns immediately
    */
    bool busy();

    /*
        Wait for the DMX transmitter to finish transmitting
        the current DMX frame. Returns immediately if no
        frame is currently being transmitted
    */

    // void await();

    /*
        De-inits the DMX transmitter instance. Releases PIO 
        and DMA resources. The instance can safely be destroyed
        after this method is called
    */
    void end();
};

#endif
