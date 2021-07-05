/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DmxInput.h"
#include "DmxInput.pio.h"
#include <clocks.h>
#include <irq.h>
#include <Arduino.h> // REMOVE ME

int DmxInput::_buffer_size() {
    int num_channels = _end_channel+1;
    return num_channels+((4-num_channels%4)%4);
}

DmxInput::return_code DmxInput::begin(uint pin, uint start_channel, uint num_channels, PIO pio)
{
    /* 
    Attempt to load the DMX PIO assembly program into the PIO program memory
    */

    if (!pio_can_add_program(pio, &DmxInput_program))
    {
        return ERR_INSUFFICIENT_PRGM_MEM;
    }
    uint prgm_offset = pio_add_program(pio, &DmxInput_program);

    /* 
    Attempt to claim an unused State Machine into the PIO program memory
    */

    int sm = pio_claim_unused_sm(pio, false);
    if (sm == -1)
    {
        return ERR_NO_SM_AVAILABLE;
    }

    // Set this pin's GPIO function (connect PIO to the pad)
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
    pio_gpio_init(pio, pin);
    gpio_pull_up(pin);

    // Generate the default PIO state machine config provided by pioasm
    pio_sm_config sm_conf = DmxInput_program_get_default_config(prgm_offset);
    sm_config_set_in_pins(&sm_conf, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&sm_conf, pin); // for JMP

    // Setup the side-set pins for the PIO state machine
    // Shift to right, autopush disabled
    sm_config_set_in_shift(&sm_conf, true, false, 32);
    // Deeper FIFO as we're not doing any TX
    sm_config_set_fifo_join(&sm_conf, PIO_FIFO_JOIN_RX);

    // Setup the clock divider to run the state machine at exactly 1MHz
    uint clk_div = clock_get_hz(clk_sys) / DMX_SM_FREQ;
    sm_config_set_clkdiv(&sm_conf, clk_div);

    // Load our configuration, jump to the start of the program and run the State Machine
    pio_sm_init(pio, sm, prgm_offset, &sm_conf);
    //sm_config_set_in_shift(&c, true, false, n_bits)

    //pio_sm_put_blocking(pio, sm, (start_channel + num_channels) - 1);

    // Set member values of class
    _prgm_offset = prgm_offset;
    _pio = pio;
    _sm = sm;
    _pin = pin;
    _start_channel = start_channel;
    _end_channel = start_channel + num_channels;

    _dma_chan = dma_claim_unused_channel(true);

    /*_buf = (volatile uint8_t*)malloc(_buffer_size());
    if(_buf == nullptr) {
        return ERR_INSUFFICIENT_SDRAM;
    }
    */
    for(int i=0;i<_buffer_size();i++) {
        ((volatile uint8_t*)_buf)[i] = 20+i;
    }

    read_with_dma();
    return SUCCESS;
}

void DmxInput::read()
{
    unsigned long start = _last_packet_timestamp;
    while(_last_packet_timestamp == start) {
        tight_loop_contents();
    }
}

volatile DmxInput *singleton;

void dmxinput_dma_handler() {
    dma_hw->ints0 = 1u << singleton->_dma_chan;
    dma_channel_set_write_addr(singleton->_dma_chan, singleton->_buf, true);
    pio_sm_exec(singleton->_pio, singleton->_sm, pio_encode_jmp(singleton->_prgm_offset));
    pio_sm_clear_fifos(singleton->_pio, singleton->_sm);
    singleton->_last_packet_timestamp = millis();
}

void DmxInput::read_with_dma() {

    pio_sm_set_enabled(_pio, _sm, false);

    // Reset the PIO state machine to a consistent state. Clear the buffers and registers
    pio_sm_restart(_pio, _sm);

    // Start the DMX PIO program from the beginning
    pio_sm_exec(_pio, _sm, pio_encode_jmp(_prgm_offset));

    //setup dma
    dma_channel_config cfg = dma_channel_get_default_config(_dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on DREQ_PIO0_RX0 (or whichever pio and sm we are using)
    channel_config_set_dreq(&cfg, pio_get_dreq(_pio, _sm, false));

    //channel_config_set_ring(&cfg, true, 5);
    dma_channel_configure(
        _dma_chan, 
        &cfg,
        NULL,    // dst
        &_pio->rxf[_sm],  // src
        _buffer_size()/4,  // transfer count,
        false
    );

    singleton = this;

    dma_channel_set_irq0_enabled(_sm, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dmxinput_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    

    //aaand start!
    //pio_sm_put_blocking(_pio, _sm, (_end_channel) - 1);
    dmxinput_dma_handler();
    pio_sm_set_enabled(_pio, _sm, true);
}

uint8_t DmxInput::get_channel(int16_t index){
    return ((volatile uint8_t*)_buf)[index];
}

unsigned long DmxInput::latest_packet_timestamp() {
    return _last_packet_timestamp;
}

void DmxInput::end()
{
    // Stop the PIO state machine
    pio_sm_set_enabled(_pio, _sm, false);

    // Remove the PIO DMX program from the PIO program memory
    pio_remove_program(_pio, &DmxInput_program, _prgm_offset);

    // Unclaim the sm
    pio_sm_unclaim(_pio, _sm);

    free((void*)_buf);
}