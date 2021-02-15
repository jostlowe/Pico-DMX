#include "dmx.hpp"

#include "dmx.pio.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"

Dmx::Result Dmx::begin(uint pin, PIO pio)
{
    /* 
    Attempt to load the DMX PIO assembly program 
    into the PIO program memory
    */

    if (!pio_can_add_program(pio, &dmx_program))
    {
        return ERR_INSUFFICIENT_PRGM_MEM;
    }
    uint prgm_offset = pio_add_program(pio, &dmx_program);

    /* 
    Attempt to claim an unused State Machine 
    into the PIO program memory
    */

    uint sm = pio_claim_unused_sm(pio, false);
    if (sm == -1)
    {
        return ERR_NO_SM_AVAILABLE;
    }

    // Set this pin's GPIO function (connect PIO to the pad)
    pio_sm_set_pins_with_mask(pio, sm, 1u << pin, 1u << pin);
    pio_sm_set_pindirs_with_mask(pio, sm, 1u << pin, 1u << pin);
    pio_gpio_init(pio, pin);

    // Generate the default PIO state machine config provided by pioasm
    pio_sm_config sm_conf = dmx_program_get_default_config(prgm_offset);

    // Setup the side-set pins for the PIO state machine
    sm_config_set_out_pins(&sm_conf, pin, 1);
    sm_config_set_sideset_pins(&sm_conf, pin);

    // Setup the clock divider to run the state machine at exactly 1MHz
    uint clk_div = clock_get_hz(clk_sys) / DMX_SM_FREQ;
    sm_config_set_clkdiv(&sm_conf, clk_div);

    // Load our configuration, jump to the start of the program and run the State Machine
    pio_sm_init(pio, sm, prgm_offset, &sm_conf);
    pio_sm_set_enabled(pio, sm, true);

    // Claim an unused DMA channel.
    // The channel is kept througout the lifetime of the DMX source
    int dma = dma_claim_unused_channel(false);

    if (dma == -1)
        return ERR_NO_DMA_AVAILABLE;

    // Get the default DMA config for our claimed channel
    dma_channel_config dma_conf = dma_channel_get_default_config(dma);

    // Set the DMA to move one byte per DREQ signal
    channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_8);

    // Setup the DREQ so that the DMA only moves data when there
    // is available room in the TXF buffer of our PIO state machine
    channel_config_set_dreq(&dma_conf, pio_get_dreq(pio, sm, true));

    // Setup the DMA to write to the TXF buffer of the PIO state machine
    dma_channel_set_write_addr(dma, &pio->txf[sm], false);

    // Apply the config
    dma_channel_set_config(dma, &dma_conf, false);

    // Set member values of C++ class
    _prgm_offset = prgm_offset;
    _pio = pio;
    _sm = sm;
    _pin = pin;
    _dma = dma;

    return SUCCESS;
}

Dmx::Result Dmx::write(uint8_t *universe, uint length)
{

    // Temporarily disable the PIO state machine
    pio_sm_set_enabled(_pio, _sm, false);

    // Reset the PIO state machine to a consistent state. Clear the buffers and registers
    pio_sm_restart(_pio, _sm);

    // Start the DMX PIO program from the beginning
    pio_sm_exec(_pio, _sm, pio_encode_jmp(_prgm_offset));

    // Restart the PIO state machinge
    pio_sm_set_enabled(_pio, _sm, true);

    // Start the DMA transfer
    dma_channel_transfer_from_buffer_now(_dma, universe, length);

    return SUCCESS;
}

bool Dmx::busy()
{
    if (dma_channel_is_busy(_dma))
        return true;

    return !pio_sm_is_tx_fifo_empty(_pio, _sm);
}