# TeensyScope
Teensy 4 oscilloscope / XY Display

A small and cheap oscilloscope style XY display based on a Teensy 4.1
and a 3.2" or 3.5" LCD module

![XY Scope](overview.jpg)

## Current functionality
This is the basic TeensyScope for use with The Analog Thing (https://the-analog-thing.org/)
It contains an XY display function that mimics an analog CRT phosphor style display
by increasing the intensity depending on the speed of the 'beam' and
it also fades out the tail of the signal in a similar way as an analog scope does.
There is also a simple time based display but still with limited functionality:
- Triggering is fixed on the (digital) ModeOP signal from THAT
- Only 2 channels (X and Y) are supported
- The sample rate is fixed at 25 µs
- The time base can be set at full ms/div values only with 1 ms/s as fastest rate
- The display only updates when a full screen is collected so at slow OP-TIME settings
  it can take a long time before the display shows the result of the operation.

A very simple command line interface using the USB-Serial is implemented
in order to play with the parameters for the CRT simulation.
The following commands are implemented:

- ?: Print help.
- burn \<start\> \<increment\> \<max\>: Set the intensity levels of a pixel on the
        LCD. Start determines the initial intensity, increment is the value
        at which the intensity is raised when a next sample is shown at the same
        pixel and max is the maximum value of the pixel.
        The intensity goes from 0 to 255 but the max. value can be higher to
        allow for a slower decay (i.e. the pixel will be visible for a longer time)
- decay \<value\>: Determines the amount that is used to decrease the intensity of
        a pixel on the LCD. This determines how fast a pixel will fade out.
- optime: Measures the OP-time from THAT (i.e. the low period on the trigger input)
- status: shows the current values for burn and decay parameters
- reset: resets the Teensy and start again

## Hardware
A Teensy 4.1 and an LCD module are all components that are needed.
For more detailed information and schematics see the
[Hardware](Hardware) section.

## Firmware
The firmware is written in C++ using Arduino and the TeensyDuino extension.
It can be found here: [Firmware](TeensyScope)
Make sure to copy all files in a folder called TeensyScope and open the
TeensyScope.ino file in the Arduino software. 
Make sure to select the Teensy 4.1 board in the Tools -> Board menu.
The USB type should be set to "Serial" (this is the default) and
CPU Speed 600 MHz (default).

Pins 10 and 11 are used as debug pins in order to measure timing 
during development.
The current firmware (0.1.0) has a fixed 25 µs sampling interval.
Processing of one set of x,y samples takes ~ 5 µs and updating the LCD ~ 31.3 ms.
This leaves ~ 20% of the available CPU time for future enhancements.
After adding the reticle, updating the LCD takes ~ 43 ms, so only ~ 15% CPU time is avaiable.

## ToDo / Feature requests
- [x] Add photos of prototype PCB to aid in recreating this
- [x] Add a reticle (the grid on an oscilloscope)
- [x] Add a timebase to be able to use this as a standard time based oscilloscope
- [ ] Add a Z input
- [x] Add a digital trigger input
- [ ] Add buttons/encoder as a user interface
- [ ] Add channel settings on the LCD
- [ ] Add more channels
- [ ] Add an FFT plot
- [ ] Automatically adjust the time base depending on the operation period (OP-TIME)
- [ ] Have the XY display behave as a XY recorder, using the ModeOP signal to start/stop
      displaying pixels
