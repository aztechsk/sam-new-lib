
# sam-new-lib

The C library **sam-new-lib** provides an API for controlling the peripherals of the microcontroller.
The supported devices include microcontrollers from the Microchip (Atmel) **AT91SAMd** family.
The supported standard peripherals include DMAC, DSU, EIC, GCLK, NVM, PM, PORT, SYSCTRL, SERCOM, TC, WDT and various hardware components
connected to the microcontroller such as buttons, LEDs, LEDUI, IO extenders (shift registers), etc.

### Library features

- Standardized API (for the AZTech framework).
- Implementation of low-level serial communication protocols.
- Designed for real-time multitasking applications (dependent on FreeRTOS).
- Efficient interrupt handling.
- Use of DMA where appropriate.
- Communication peripheral instances are represented by C structures with synchronous (blocking) read() and write() I/O operations.
- Support for low-power microcontroller modes. Peripheral blocks are turned off when entering sleep mode.
