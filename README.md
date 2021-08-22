# sim3 - an i960Sx emulator for microcontrollers and desktops

## Why?
For the past two years I have been working on creating a custom mainboard to allow an i960SB-10 processor live once again.
It is powered by an atmega1284p with a series of SPI IOExpanders, PSRAM, an sdcard slot, and a fully synchronized clock. 

While this works, there is significant latency in certain cases in between memory transactions as the microcontroller 
services memory requests. The actual slow down has to do with the direct mapped cache I had to implement inside the 1284p. 
Well... More like it every time I have to flush a cache line. While there will always be latency due to the use of the
IO expanders, I have hit a wall. 

My attempts to use faster microcontrollers (the adafruit grand central m4 would be perfect) led to the discovery that 
faster arm based microcontrollers have multiple clock domains. This allows the CPU and peripherals to run at 
independent rates. It is possible to emit the cpu clock over an io pin but I ran into issues with
dividing the clock signal down to the 20MHz signal required by the i960 (AHC is too slow at 3.3v and 
LVC has fewer through hole version of the ICs I need for fast prototyping).

Since I am a computer scientist, I decided the next best thing to do was to emulate 
the i960Sx (although it's more of a hackery of newer chips as well). This the third 
version of the emulator. The first two were disasters that took me many months to write
years ago. I wrote this new one in 24 hours spread across three 8-hour days. I wrote it originally
for work to provide us with real world test cases for our coverage engine.

The other main reason for this simulator is one of preservation. The i960 is a dead architecture according to 
intel but I find the instruction set so simple and elegant that I don't want it to die. If I ever decide to get
ambitious and add an i960 port to llvm then I will need this to test the results.

I also feel that the design of the i960 makes a great hobbyist platform as well but is living
on borrowed time currently. This emulator is meant to free the architecture from that fate. 

## Supported targets

The public version of this repo uses platformio to make cross platform development trivial.
There are two major targets:

1. Desktop (normal application)
2. Grand Central M4 / Arduino 

The emulator is designed in such a way so that the actual emulation core is written in C++14/17 with a single dependency
on cstdint to make cross platform trivial (C99 and later). Since it started as a way to exercise different testing scenarios
there are a considerable use of lambdas and constexpr as well. 

Adding new targets only requires implementing the load/store routines. The rest of the core is self contained with the
design taking inspiration from the physical i960 hardware. An i960 processor only talks to the rest of the system when 
it needs to load or store something. Thus the emulator works the same way. I also had to add the ability to emulate the 
boot process for different chips in the series. 

The emulator core is also in-order sequential and does not have an onboard register cache. I will eventually implement it
but this will not affect any child methods.

## Notes

The implementations of different instructions comes from both the physical hardware I have to and the manuals for several
i960 chip generations (MC, Sx, Jx, and Hx). While Intel got better about accurate documentation as time went on, some of 
the earlier manuals have absolute lies in them or copy paste errors. It is pretty funny. But if you see a rant 
in the code around some instructions know that instruction actually worked differently compared to what the docs stated. 

It was fun to do and I plan to keep adding the rest of the instructions from Numerics and the newer core operations as well.
Eventually, I will try to add the protected extensions as well but I do not have access to a chip with that feature 
(I do have the manuals though). As for Extended... probably never since I can get access to the docs but I don't think 
gcc 3.4.6 supports it nor do I have a way to test it. Those operations were designed to support Ada and its many features.
It was only ever available in the i960Mx processor for the military market only.
