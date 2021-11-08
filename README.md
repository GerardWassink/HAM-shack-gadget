# HAM-shack-gadget
Building a clock / thermometer / GPS locator with GPS date/time

This project is about building a gadget for in the HAM shack. It’s in the prototype stage at this time. I started out with an Arduino Uno (clone by Velleman), two DS18B20 temperature sensors and two 20 x 4 displays with I2C. This first version I used for setting up the code base.

- The gadget allows for the connection of two temperature sensors. I.E. one for room temperature, the other for the outside temp?
- Date and time are received from the GPS satellite, as are the latitude and longitude.
- I connected a 4x4 matrix keyboard to be able to do some functions, from the main screen there are two buttons:
  - A - switch the backlights on / off
  - \* - show the main menu

## Future plans

### Settings
- Make the offset for local time and DST dynamically changeable

### Enclosure and keyboard
- Of course in the end a nice enclosure will complete the build. 
