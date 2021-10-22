/* ------------------------------------------------------------------------- *
 * Name   : HAM-gadget.ino
 * Author : Gerard Wassink
 * Date   : October 22, 2021
 * Purpose: Time, temp, GPS location indication with NTP fallback
 * ------------------------------------------------------------------------- *
 *
 * ------------------------------------------------------------------------- *
 *             GNU LICENSE CONDITIONS
 * ------------------------------------------------------------------------- *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ------------------------------------------------------------------------- *
 *       Copyright (C) 2021 Gerard Wassink
 * ------------------------------------------------------------------------- */

/* 
 * Include libraries for peripherals 
 */
#include <OneWire.h>                        // Onewire comms library
#include <DallasTemperature.h>              // Temperature library

#include <Wire.h>                           // I2C comms library
#include <LiquidCrystal_I2C.h>              // LCD library

/*
 * Definitions
 */
 // Data wire is plugged into pin 2 on the Arduino 
#define ONE_WIRE_BUS 2 

/*
 * Create objects
 */
// Setup a oneWire instance to communicate with any OneWire devices  
OneWire oneWire(ONE_WIRE_BUS); 

// Pass the oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Define objects with addres(ses) for the LCD screen(s)
LiquidCrystal_I2C lcd1(0x27,20,4);          // Initialize display 1
LiquidCrystal_I2C lcd2(0x26,20,4);          // Initialize display 2
//LiquidCrystal_I2C lcd3(0x25,20,4);          // Initialize display 3

/*
 * Global variables
 */
String progVersion = "1.0";


void setup()
{
  /*
   * Initialize LCD's 
   */
  lcd1.init();
  lcd2.init();
  /* 
   * Switch backlights on 
   */
  lcd1.backlight();
  lcd2.backlight();
  /* 
   * Show initial screen with version and such
   */
  LCD_display(lcd1, 0, 0, "   --- NL14080 ---  ");
  LCD_display(lcd1, 1, 0, "  HAM contraption   ");
  LCD_display(lcd1, 2, 0, "  Displaying stuff  ");
  LCD_display(lcd1, 3, 0, "Software version    ");
  LCD_display(lcd1, 3, 17, progVersion);

  LCD_display(lcd2, 0, 0, "   --- NL14080 ---  ");
  LCD_display(lcd2, 1, 0, "  HAM contraption   ");
  LCD_display(lcd2, 2, 0, "  Displaying stuff  ");
  LCD_display(lcd2, 3, 0, "Software version    ");
  LCD_display(lcd2, 3, 17, progVersion);
  delay(5000);
  /* 
   * Put template text on LCD 1 
   */
  LCD_display(lcd1, 0, 0, "NL14080 --- Sats: nn");
  LCD_display(lcd1, 1, 0, "Temp  .....  ..... C");
  LCD_display(lcd1, 2, 0, "Date      22-10-2021");
  /* 
   * Put start message on LCD 2 
   */
  LCD_display(lcd2, 0, 0, "Station     NL14080");
  LCD_display(lcd2, 1, 0, "Operator    Gerard");
  LCD_display(lcd2, 2, 0, "QTH Locator JO33di");

  /* 
   * Start up the Temperature sensing library 
   */
  sensors.begin();
  
}


void loop()
{
  float temp1, temp2;

  /* 
   * Requesting temperatures... 
   */
  sensors.requestTemperatures();
  /* 
   * Why "byIndex"? 
   * You can have more than one DS18B20 on the same bus.  
   * and 0 refers to the first IC on the wire 
   */
  temp1 = sensors.getTempCByIndex(0);            // temp in degrees Celcius from first sensor
  temp2 = sensors.getTempCByIndex(1);            // temp in degrees Celcius from second sensor

  /* 
   * Fill in temperatures in template on display 1
   */
  LCD_display(lcd1, 1,  6, String(temp1));          // display Celcius
  LCD_display(lcd1, 1, 13, String(temp2));          // display Celcius
  
  /* 
   * Fill in times in template on display 1
   */
  LCD_display(lcd1, 3, 0, "Time JO33di 12:12:30");
  LCD_display(lcd2, 3, 0, "Time UTC    10:12:30");

  delay(1000);
}

/*
 * Routine to combine the setCursor and print routines for both (all) displays
 */
void LCD_display(LiquidCrystal_I2C displ, int row, int col, String text) {
    displ.setCursor(col, row);
    displ.print(text);
}
