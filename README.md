# [WIP] rpi-pico-dmx
A library for outputing the DMX512-A lighting control protocol from a Raspberry Pi Pico

## About

The Raspberry Pi Pico provides an exciting new addition to the market on low-cost easy-to-use microcontroller board. The RPi Pico has plenty of features making it particularly interesting for lighting control hackers:

* Lighting control software can potentially become quite large. A large chunk of flash (a whopping 2MB in the RPi Pico) is required to run networked protocols such as Art-Net and sACN (ANSI e1.31)
* Storing DMX universes requires a lot of RAM (>512B for each full DMX frame). With other microcontrollers, RAM is a scarce resource and manipulating DMX universes can quickly consume the entire RAM of the microcontroller. The RPi Pico has a solid 264kB of RAM, leaving plenty of room for dmx frame manipulation
* Libraries for sending DMX on other microcontrollers tend to rely on either bit-banging a GPIO or a hack on a hardware UART to allow the UART to assert a sufficiently long BREAK and MAB at the beginning of each transfer. Bit-banging consumes a processor core for the duration of a transfer and is extremely sensitive to interrupts. Using a hardware UART is less computationally intensive, but most low-cost microcontrollers only have 1 or 2 hardware UARTs, limiting the number of parallel universes one can transmit or receive. The Programmable IO (PIO) modules of the RPi Pico gives the possibility to create what is _basically_ hardware support for custom IO protocols (such as DMX). The RPi Pico features 2 PIO modules with 4 State Machines each, making it possible to input or output up to 8 universes _in parallel_. Thats more universes than you can shake a proverbial stick at.
* As if the PIO wasn't impressive enough, the RPi Pico has a powerful 10 channel DMA controller. In conjunction with the PIO, the DMA makes it possible to transmit or receive entire DMX universes with only a handful of instructions.
* The RPi Pico has a rather odd combination of 2 ARM Cortex M0+ cores running at speeds up to 133MHz. The dual core architecture could provide a huge benefit in processing DMX universes as computationally demanding data processing can be offloaded on the second core.

This library implements a DMX transmitter for the RPi Pico, leveraging its powerful DMA and PIO features. 

The library is written in C++ for the RPi Pico SDK. The library will be migrated to the Arduino kernel as soon as it is officially released.

## Installation

The installation assumes that you have followed the 
[getting started guide](https://www.raspberrypi.org/documentation/pico/getting-started/) and that you have a created a working RPi Pico project. It is strongly recommended to use the project generator for the Pico.

Navigate to the root directory of your project and clone the rpi-pico-dmx repository 
   
```
git clone https://github.com/jostlowe/rpi-pico-dmx.git
```
The simplest way to add the rpi-pico-dmx is to add the ```dmx.cpp``` file to your project executables. When you create a new RPi Pico project, there should be a line in your ```CMakeLists.txt``` that looks roughly like this

```cmake
add_executable(your-project-name
        your-project-name.cpp
        )
```
Append ```rpi-pico-dmx/dmx.cpp``` to this line
```cmake
add_executable(your-project-name
        your-project-name.cpp
        rpi-pico-dmx/dmx.cpp
        )
```
The PIO assembler needs to be instructed to build a c-header for the PIO assembly code driving the DMX transmitter. Add the following line to your ```CMakeLists.txt```

```cmake
pico_generate_pio_header(your-project-name ${CMAKE_CURRENT_LIST_DIR}/rpi-pico-dmx/dmx.pio)
```

Make sure that the ```hardware_dma``` and ```hardware_pio``` are added to your link libraries.

```cmake
target_link_libraries(your-project-name
        hardware_dma
        hardware_pio
        )
```

## Examples

Coming soon


## Voltage Transceivers
The RPi Pico itself cannot be directly hooked up to your DMX line, as DMX operates on RS485 logic levels, 
which do not match the voltage levels of the GPIO pins on the RPi Pico. 

Fortunately TLL to RS485 transceivers are easily available. Simple transceiver modules can be bought through online retailer for only a couple of dollars. These tend to use the MAX485 series of voltage level transceivers, which work great for most purposes. If you're planning to implement DMX on an industrial level, your device should have some kind of EMC protection. Many RS485 transceivers are available that have galvanic isolation between the TLL side and the RS485 side. These should be the preferred option.