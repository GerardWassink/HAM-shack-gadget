# HAM-shack-gadget
Building a clock / thermometer / GPS locator with GPS date/time

This project is about building a gadget for use in the HAM shack. In the prototype stage I started out with an Arduino Uno (clone by Velleman), two DS18B20 temperature sensors and two 20x4 displays with I2C. This first version I used for setting up the code base.

In later versions I went back to one temperature sensor and one 20x4 I2C display.

After that first stage I switched to an ATMEGA328P chip and built it and it's surrounding hardware onto a generic PCB board.

- The gadget allows for the connection of a temperature sensor
- Received from the GPS satellite are:
	- Date and time
	- Latitude and longitude
- A 4x4 matrix keyboard is used to do some functions

For more information see the PDF description.

## Version history
See top of source code

