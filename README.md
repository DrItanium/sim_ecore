# sim_ecore - an i960Sx emulator for microcontrollers to allow drop in replacement of i960Sx processors

## Purpose
This is an out cropping of all the work I have done in bringing up i960 based
custom hardware. I have been successful in bringing up a real i960SB-10/SA-16
processor in a design I call type 1 with type2 on the way. However, the i960
processors themselves are a limited resource that will eventually run out. So
originally sim3 was written to solve that little issue. However, it is very
_boring_ because it is a full emulation of an i960 design. So there is really
no fun with connecting it up to physical memory chips or external devices. 
It also required that I maintain basically two copies of my chipset design
which really killed momentum for me. 

Recently, I have been experimenting with the EBI/EMI of AVR processors and
found it to be exceptionally powerful (especially on the 8515 and 162... or as
I call them: Challenge Chips!). 

I came up with an idea to map the address and data lines of a real i960 to the
EBI of a proper chipset MCU to greatly improve performance. However, you still
need a real i960 processor plus various support hardware. But what if we could
just turn the chipset itself into the i960 processor? 

Well with an EBI/EMI based microcontroller we can! The microcontroller core
runs the simulator itself and accesses everything through the EBI/EMI (or just
EBI from now on). Through bank switching we can even map the entire 32-bit
memory space as well.

This idea also fits with something like the Feather-S2 with its onboard 8 Megs
of PSRAM. All one would need to do is provide a large flash chip that is
accessible over SPI and it could be easily done. 

The only problems I can see with this design is lack of program flash to store
the emulator. Thus I have forked sim3 so I can make the design as tight as
possible. My initial target is an ATmega8515 (8k flash, 512 byte SRAM) so the
bus will actually only be 24-bits wide. 

The only part that will be awkward is dealing with the EBI not mapping
everything into RAM correctly. On SRAM, this isn't an issue but a FLASH chip
will be much more awkward as it would require holes where things are not
mapped... I'll have to come up with a solution to map everything without going
crazy. 

Regardless of the outcome, all of this will be very interesting to be sure :)

