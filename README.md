# Pico-DMX

![example branch parameter](https://github.com/jostlowe/Pico-DMX/actions/workflows/Arduino-lint.yml/badge.svg?branch=arduino-port)

A library for outputing the DMX512-A lighting control protocol from a RaspberryPi Pico

## About

The RaspberryPi Pico provides an exciting new addition to the market on low-cost easy-to-use microcontroller board. The Pico has plenty of features making it particularly interesting for lighting control hackers:

* Lighting control software can potentially become quite large. A large chunk of flash is required to run networked protocols such as Art-Net and sACN (ANSI e1.31). the Pico has a whopping 2MB of flash at disposal, the Pico has a whopping 2MB of flash at disposal 
* Storing DMX universes requires a lot of RAM (>512B for each full DMX frame). With other microcontrollers, RAM is a scarce resource and manipulating DMX universes can quickly consume the entire RAM of the microcontroller. The Pico has a solid 264kB of RAM, leaving plenty of room for dmx frame manipulation
* Libraries for sending DMX on other microcontrollers tend to rely on either bit-banging a GPIO or a hack on a hardware UART to allow the UART to assert a sufficiently long BREAK and MAB at the beginning of each transfer. Bit-banging consumes a processor core for the duration of a transfer and is extremely sensitive to interrupts. Using a hardware UART is less computationally intensive, but most low-cost microcontrollers only have 1 or 2 hardware UARTs, limiting the number of parallel universes one can transmit or receive. The Programmable IO (PIO) modules of the Pico gives the possibility to create what is _basically_ hardware support for custom IO protocols (such as DMX). The Pico features 2 PIO modules with 4 State Machines each, making it possible to input or output up to 8 universes _in parallel_. Thats more universes than you can shake a proverbial stick at.
* As if the PIO wasn't impressive enough, the Pico has a powerful 10 channel DMA controller. In conjunction with the PIO, the DMA makes it possible to transmit or receive entire DMX universes with only a handful of instructions.
* The Pico has a rather odd combination of 2 ARM Cortex M0+ cores running at speeds up to 133MHz. The dual core architecture could provide a huge benefit in processing DMX universes as computationally demanding data processing can be offloaded on the second core.

This library implements a DMX transmitter for the RPi Pico, leveraging its powerful DMA and PIO features. 


## Installation
The Pico-DMX library should be available in both the Arduino libraries and the PlatformIO libraries

The Pico-DMX library can also be manually added to any project in the same way as any other Arduino library. See the [Arduino guide](https://www.arduino.cc/en/guide/libraries) for more information. 

## Usage
The library is based around the `Dmx` class, representing a single DMX output. The `Dmx` class is simply instantiated, and requires no further arguments.

```C++
   Dmx myDmxOutput;
```

After instantiation, the DMX output must be assigned to a pin. The `.begin(uint pin)` method initializes the DMX output and binds it to a pin. To start a DMX output on GPIO1 on the Pico, simply call

```C++
   myDmxOutput.begin(1);
```

The library defaults to using `pio0` as the underlying PIO instance. If you want to use `pio1` as the underlying PIO, simply add the PIO you want to use as an argument

```C++
   myDmxOutput.begin(1, pio1);
```

Even though the DMX output is initialized and ready to party, the output is still idle, waiting for a universe to transmit. A universe is simply an array of `uint8_t`, with a maximum length of 513. The zero'th value in the array is the packet start code and should be set to 0 for ordinary DMX data packets. Let's make a universe with 3 channels (plus the start code), and set channel 1 to full.

```C++
   uint8_t universe[4]; 
   universe[1] = 255;
```

To send the universe, call the `.write(...)` method to send the universe _wooshing_ down your DMX line. 

```C++
   myDmxOutput.write(universe, 4);
```

The `.write(...)` method is not blocking, and executes immediately. To check the status of your DMX transmission, you can call `.busy()`to check if the DMX output is done transmitting your universe.

```C++
   while(myDmxOutput.busy()) {
        // Patiently wait, or do other computing stuff
   }
```

## Voltage Transceivers
The RPi Pico itself cannot be directly hooked up to your DMX line, as DMX operates on RS485 logic levels, 
which do not match the voltage levels of the GPIO pins on the RPi Pico. 

Fortunately TLL to RS485 transceivers are easily available. Simple transceiver modules can be bought through online retailer for only a couple of dollars. These tend to use the MAX485 series of voltage level transceivers, which work great for most purposes. If you're planning to implement DMX on an industrial level, your device should have some kind of EMC protection. Many RS485 transceivers are available that have galvanic isolation between the TLL side and the RS485 side. These should be the preferred option.
