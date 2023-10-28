# HAM-shack-gadget
Building a clock / thermometer / GPS locator with GPS date/time

This project is about building a gadget for use in the HAM shack. In the prototype stage I started out with an Arduino Uno (clone by Velleman), two DS18B20 temperature sensors and two 20 x 4 displays with I2C. This first version I used for setting up the code base.

After that first stage I switched to an ATMEGA328P chip and built it and it's surrounding hardware onto a generic PCB board.

- The gadget allows for the connection of a temperature sensor
- Date and time are received from the GPS satellite, as are the latitude and longitude
- I connected a 4x4 matrix keyboard to be able to do some functions

For more information see the PDF description.

## Version history
See top of source code

