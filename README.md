# HAM-shack-gadget
Building a clock / thermometer / GPS locator with fallback to NTP

Today I worked on version 0.1 of my new project. It’s in the prototype stage at this time. For this test it’s an Arduino Uno (clone by Velleman), two DS18B20 temperature sensors and two 20 x 4 displays.

This first version is for setting up the code base. The temperature sensors do work. One is the room temperature, the second one is on top of my desktop computer (that’s why it’s indicating 25.13 C at the time).

## Future plans

As for now, the time display is not working yet, but in the top right corner of the bottom display you can find a hint of what’s coming in future versions.
GPS time and location

There will be a GPS module involved, and the number of satellites to which it’s connected will be displayed there. This will also be able to display our exact position. Convenient when we’re mobile (and on battery power).

## Connected board

The Arduino Uno will be replaced with another module that has WiFi on board and will thus be able to connect to NTP servers. When the satellites are out of range, we can extract the time from the internet (provided we have internet and WiFi of course…).
Enclosure and keyboard

Of course in the end a nice enclosure will complete the build. I might even throw in a small 4 x 4 keyboard to enhance the operation and make it a multi functional work horse…
