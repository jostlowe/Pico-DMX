
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

#define DMXINPUT_BUFFER_SIZE(start_channel, num_channels) ((start_channel+num_channels+1)+((4-(start_channel+num_channels+1)%4)%4))
class DmxInput
{
    uint _pin;   
    int32_t _start_channel;
    int32_t _num_channels;
    
public:
    /*
    private properties that are declared public so the interrupt handler has access
    */
    volatile uint8_t *_buf;
    volatile PIO _pio;
    volatile uint _sm;
    volatile uint _dma_chan;
    volatile uint _prgm_offset;
    volatile unsigned long _last_packet_timestamp=0;
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
        ERR_INSUFFICIENT_SDRAM = -3,
    };

    /*
       Starts a new DMX input instance. 
       
       Param: pin
       Any valid GPIO pin on the Pico

       Param: pio
       defaults to pio0. pio0 can run up to 3
       DMX input instances. If you really need more, you can
       run 3 more on pio1  

       Param: ring_size
       sets number of dmx channels to receive.
       ring_size==2 -> channels 0 through 3
       ring_size==3 -> channels 0 through 7
       ring_size==4 -> channels 0 through 15
       ring_size==5 -> channels 0 through 31
       ring_size==6 -> channels 0 through 63
       ring_size==7 -> channels 0 through 127
       ring_size==8 -> channels 0 through 255
       ring_size==9 -> channels 0 through 511
       Note that channel 0 is not really useful and should always have the value 0.
    */

    return_code begin(uint pin, uint start_channel, uint num_channels, PIO pio = pio0);

    /*
    Wait until a new DMX frame is received. The data may be fetched using get_channel()
    */
    void read(volatile uint8_t *buffer);

    /*
    Start async read process. This should only be called once. From then on, the buffer will always contain the latest DMX data.
    */
    void read_async(volatile uint8_t *buffer);

    /*
        Get the timestamp (like millis()) from the moment the latest dmx packet was received.
        May be used to detect if the dmx signal has stopped coming in.
    */

    unsigned long latest_packet_timestamp();

    /*
        De-inits the DMX input instance. Releases PIO resources. 
        The instance can safely be destroyed after this method is called
    */
    void end();
};

#endif