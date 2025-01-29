# ThreadX on a STM32L011K4

This repository demonstrates how to run an RTOS on a microcontroller with 2kB of RAM. ThreadX uses around 1kB. The rest is used by the application itself. The application uses pure CMSIS bindings without leveraging a heavy high-level HAL.

## Application Capabilities

The application does the following:

- Configure max clock frequency (32MHz)
- Toggle an LED on and off in two separate threads
- Print out the current state of the LED via UART (over DMA)
- Increment an integer in flash memory and print it out in two separate threads

This is not a lot of stuff, but still speaks for the minimalistic nature of ThreadX.
