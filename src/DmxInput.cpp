/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DmxInput.h"
#include "DmxInput.pio.h"
#include <clocks.h>
#include <irq.h>

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
    // Shift to right, autopull disabled
    sm_config_set_in_shift(&sm_conf, true, false, 32);
    // Deeper FIFO as we're not doing any TX
    sm_config_set_fifo_join(&sm_conf, PIO_FIFO_JOIN_RX);

    // Setup the clock divider to run the state machine at exactly 1MHz
    uint clk_div = clock_get_hz(clk_sys) / DMX_SM_FREQ;
    sm_config_set_clkdiv(&sm_conf, clk_div);

    // Load our configuration, jump to the start of the program and run the State Machine
    pio_sm_init(pio, sm, prgm_offset, &sm_conf);
    //pio_sm_set_enabled(pio, sm, true);

    // Set member values of class
    _prgm_offset = prgm_offset;
    _pio = pio;
    _sm = sm;
    _pin = pin;
    _start_channel = start_channel;
    _end_channel = start_channel + num_channels;

    return SUCCESS;
}

void DmxInput::read(uint8_t *buffer)
{
    // Temporarily disable the PIO state machine
    pio_sm_set_enabled(_pio, _sm, false);

    // Reset the PIO state machine to a consistent state. Clear the buffers and registers
    pio_sm_restart(_pio, _sm);

    // Start the DMX PIO program from the beginning
    pio_sm_exec(_pio, _sm, pio_encode_jmp(_prgm_offset));

    // Restart the PIO state machinge
    pio_sm_set_enabled(_pio, _sm, true);

    uint channel_count = 0;

    while (channel_count < _start_channel)
    {
        // Pull the first channels before our channels of interest
        pio_sm_get_blocking(_pio, _sm);
        channel_count++;
    }

    while (channel_count <= _end_channel)
    {
        // Move our channels of interest into the buffer
        int local_index = _start_channel - channel_count;
        buffer[local_index] = (uint8_t)pio_sm_get_blocking(_pio, _sm);
        channel_count++;
    }
    pio_sm_set_enabled(_pio, _sm, false);
}

void DmxInput::end()
{
    // Stop the PIO state machine
    pio_sm_set_enabled(_pio, _sm, false);

    // Remove the PIO DMX program from the PIO program memory
    pio_remove_program(_pio, &DmxInput_program, _prgm_offset);

    // Unclaim the sm
    pio_sm_unclaim(_pio, _sm);
}