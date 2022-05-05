/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DmxInput.h"
#include "DmxInput.pio.h"
#include "DmxInputInverted.pio.h"

#if defined(ARDUINO_ARCH_MBED)
  #include <clocks.h>
  #include <irq.h>
  #include <Arduino.h> // REMOVE ME
#else
  #include "pico/time.h"
  #include "hardware/clocks.h"
  #include "hardware/irq.h"
#endif

bool prgm_loaded[] = {false,false};
volatile uint prgm_offsets[] = {0,0};
/*
This array tells the interrupt handler which instance has interrupted.
The interrupt handler has only the ints0 register to go on, so this array needs as many spots as there are DMA channels. 
*/
#define NUM_DMA_CHANS 12
volatile DmxInput *active_inputs[NUM_DMA_CHANS] = {nullptr};

DmxInput::return_code DmxInput::begin(uint pin, uint start_channel, uint num_channels, PIO pio, bool inverted)
{
    uint pio_ind = pio_get_index(pio);
    if(!prgm_loaded[pio_ind]) {
        /* 
        Attempt to load the DMX PIO assembly program into the PIO program memory
        */
       if(!inverted) {
        if (!pio_can_add_program(pio, &DmxInput_program))
        {
            return ERR_INSUFFICIENT_PRGM_MEM;
        }
        prgm_offsets[pio_ind] = pio_add_program(pio, &DmxInput_program);
       } else {
        if (!pio_can_add_program(pio, &DmxInputInverted_program))
        {
            return ERR_INSUFFICIENT_PRGM_MEM;
        }
        prgm_offsets[pio_ind] = pio_add_program(pio, &DmxInputInverted_program);
       }

        prgm_loaded[pio_ind] = true;
    }

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
    pio_sm_config sm_conf;
    if(!inverted) {
        sm_conf = DmxInput_program_get_default_config(prgm_offsets[pio_ind]);
    } else {
        sm_conf = DmxInputInverted_program_get_default_config(prgm_offsets[pio_ind]);
    }
    sm_config_set_in_pins(&sm_conf, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&sm_conf, pin); // for JMP

    // Setup the side-set pins for the PIO state machine
    // Shift to right, autopush disabled
    sm_config_set_in_shift(&sm_conf, true, false, 8);
    // Deeper FIFO as we're not doing any TX
    sm_config_set_fifo_join(&sm_conf, PIO_FIFO_JOIN_RX);

    // Setup the clock divider to run the state machine at exactly 1MHz
    uint clk_div = clock_get_hz(clk_sys) / DMX_SM_FREQ;
    sm_config_set_clkdiv(&sm_conf, clk_div);

    // Load our configuration, jump to the start of the program and run the State Machine
    pio_sm_init(pio, sm, prgm_offsets[pio_ind], &sm_conf);
    //sm_config_set_in_shift(&c, true, false, n_bits)

    //pio_sm_put_blocking(pio, sm, (start_channel + num_channels) - 1);

    _pio = pio;
    _sm = sm;
    _pin = pin;
    _start_channel = start_channel;
    _num_channels = num_channels;
    _buf = nullptr;
    _cb = nullptr;

    _dma_chan = dma_claim_unused_channel(true);


    if (active_inputs[_dma_chan] != nullptr) {
        return ERR_NO_SM_AVAILABLE;
    }
    active_inputs[_dma_chan] = this;

    return SUCCESS;
}

void DmxInput::read(volatile uint8_t *buffer)
{
    if(_buf==nullptr) {
        read_async(buffer);
    }
    unsigned long start = _last_packet_timestamp;
    while(_last_packet_timestamp == start) {
        tight_loop_contents();
    }
}

void dmxinput_dma_handler() {
    for(int i=0;i<NUM_DMA_CHANS;i++) {
        if(active_inputs[i]!=nullptr && (dma_hw->ints0 & (1u<<i))) {
            dma_hw->ints0 = 1u << i;
            volatile DmxInput *instance = active_inputs[i];
            dma_channel_set_write_addr(i, instance->_buf, true);
            pio_sm_exec(instance->_pio, instance->_sm, pio_encode_jmp(prgm_offsets[pio_get_index(instance->_pio)]));
            pio_sm_clear_fifos(instance->_pio, instance->_sm);
#ifdef ARDUINO
            instance->_last_packet_timestamp = millis();
#else
            instance->_last_packet_timestamp = to_ms_since_boot(get_absolute_time());
#endif
            // Trigger the callback if we have one
            if (instance->_cb != nullptr) {
                (*(instance->_cb))((DmxInput*)instance);
            }
        }
    }
    
}

void DmxInput::read_async(volatile uint8_t *buffer, void (*inputUpdatedCallback)(DmxInput*)) {

    _buf = buffer;
    if (inputUpdatedCallback!=nullptr) {
        _cb = inputUpdatedCallback;
    }

    pio_sm_set_enabled(_pio, _sm, false);

    // Reset the PIO state machine to a consistent state. Clear the buffers and registers
    pio_sm_restart(_pio, _sm);

    //setup dma
    dma_channel_config cfg = dma_channel_get_default_config(_dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
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
        DMXINPUT_BUFFER_SIZE(_start_channel, _num_channels),  // transfer count,
        false
    );

    dma_channel_set_irq0_enabled(_dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dmxinput_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    //aaand start!
    dma_channel_set_write_addr(_dma_chan, buffer, true);
    pio_sm_exec(_pio, _sm, pio_encode_jmp(prgm_offsets[pio_get_index(_pio)]));
    pio_sm_clear_fifos(_pio, _sm);
#ifdef ARDUINO
    _last_packet_timestamp = millis();
#else
    _last_packet_timestamp = to_ms_since_boot(get_absolute_time());
#endif

    pio_sm_set_enabled(_pio, _sm, true);
}

unsigned long DmxInput::latest_packet_timestamp() {
    return _last_packet_timestamp;
}

uint DmxInput::pin() {
    return _pin;
}

void DmxInput::end()
{
    // Stop the PIO state machine
    pio_sm_set_enabled(_pio, _sm, false);

    // Remove the PIO DMX program from the PIO program memory
    uint pio_id = pio_get_index(_pio);
    bool inuse = false;
    for(uint i=0;i<NUM_DMA_CHANS;i++) {
        if(i==_dma_chan) {
            continue;
        }
        if(pio_id == pio_get_index(active_inputs[i]->_pio)) {
            inuse = true;
            break;
        }
    }
    if(!inuse) {
        prgm_loaded[pio_id] = false;
        pio_remove_program(_pio, &DmxInput_program, prgm_offsets[pio_id]);
        prgm_offsets[pio_id]=0;
    }

    // Unclaim the sm
    pio_sm_unclaim(_pio, _sm);

    dma_channel_unclaim(_dma_chan);
    active_inputs[_dma_chan] = nullptr;

    _buf = nullptr;
}
