# NintendoClassicRewind
An emulated controller that goes home and enters the rewind function when commanded.

### Overview

1. At powerup, the connected controller (if any) has control.
2. When a button is pushed, the microcontroller switches the serial data line to respond to the SNES/NES Classic input poll requests with the desired action (e.g.: go home, rewind).
3. After the button sequence is complete, control is given back to the controller.

The home button presses the Home button.

The rewind button presses Home, B, B, Down, and X.

The detect signal connected to the Classic System is pulsed low after the sequence is complete or when a controller is connected. This should force the system to re-initialize the controller on the off-chance it gets hung up by disturbing the I<sup>2</sup>C bus.

The serial clock line is un-switched and shared between the controller and the ATmega328. I included a 2K2 pull-up resistor for instances where no controller is attached, allowing the Home button to pressed (e.g.: when it's just connected to Player 2's port). There are still pull-ups in the controller, however. I didn't observe any issues when sniffing the bus using a Saleae Logic analyzer, but your mileage may vary. Suggestions are welcome if you have any other ideas.

### Parts

1. Arduino Pro Mini - 3.3V
2. CD74HC4067 16-Channel Analog Multiplexer (only 2 channels are used, so a 74HC4051 or something similar could be used)
3. NES / SNES / Wii Extension Cable (e.g.: 10ft)
4. Buttons (2)
5. Resistors: 2K2 ohm (2), 10K ohm (1)

### Rough Schematic

![Awful Schematic](https://raw.githubusercontent.com/focanmess/NintendoClassicRewind/master/rough_schematic.jpg)

I may make a proper schematic and PCB later.

### Other Thoughts

It'd probably be easier to just modify clvcon.c to replay the rewind sequence when a certain button combination is pressed, but this presents an alternative for un-modified systems.
