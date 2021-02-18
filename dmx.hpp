#ifndef DMX_SOURCE_H
#define DMX_SOURCE_H

#include <pico/stdlib.h>
#include <stdio.h>
#include "hardware/dma.h"
#include "hardware/pio.h"

#define DMX_UNIVERSE_SIZE 512
#define DMX_SM_FREQ 1000000

class Dmx
{
    uint _prgm_offset;
    uint _pin;
    uint _sm;
    PIO _pio;
    uint _dma;

public:
    enum Result
    {
        SUCCESS = 1,
        ERR_NO_SM_AVAILABLE = -1,
        ERR_INSUFFICIENT_PRGM_MEM = -2,
        ERR_NO_DMA_AVAILABLE = -3
    };

    Result begin(uint pin, PIO pio = pio0);
    Result write(uint8_t *universe, uint length);
    bool busy();
    void await();
    void end();
};

#endif