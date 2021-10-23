/* ------------------------------------------------------------------------- *
 * Name   : HAM-gadget
 * Author : Gerard Wassink
 * Date   : October 22, 2021
 * Purpose: Time, temp, GPS location indication with NTP fallback
 * Versions:
 *   0.1  : Initial code base, temp sensors working
 *   0.2  : Cleaned op the code
 *          built in backlight switch (on / off)
 *          built in UTC / QTH switch
 *   0.3  : Switched over to Nano RP2040 Connect
 *          Somehow had to switch from pin D3 to D5
 * ------------------------------------------------------------------------- */
#define progVersion "0.3"                   // Program version definition
/* ------------------------------------------------------------------------- *
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

/* ------------------------------------------------------------------------- *
 *       Include libraries for peripherals 
 * ------------------------------------------------------------------------- */
#include <OneWire.h>                        // Onewire comms library
#include <DallasTemperature.h>              // Temperature library

#include <Wire.h>                           // I2C comms library
#include <LiquidCrystal_I2C.h>              // LCD library


/* ------------------------------------------------------------------------- *
 *       Definitions
 * ------------------------------------------------------------------------- */
#define ONE_WIRE_BUS    2                   // Data wire plugged into pin 2
#define PIN_BACKLIGHT   5                   // Pin switch backlight on / off
#define PIN_UTC_QTH     4                   // Pin switch UTC and QTH time
                                            
/* ------------------------------------------------------------------------- *
 *       Create objects for sensors
 * ------------------------------------------------------------------------- */
OneWire oneWire(ONE_WIRE_BUS);              // Setup oneWire for sensors
DallasTemperature sensors(&oneWire);        // Pass oneWire reference to Dallas

/* ------------------------------------------------------------------------- *
 *       Create objects with addres(ses) for the LCD screen(s)
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C lcd1(0x27,20,4);          // Initialize display 1
LiquidCrystal_I2C lcd2(0x26,20,4);          // Initialize display 2

/* ------------------------------------------------------------------------- *
 *       Define global variables
 * ------------------------------------------------------------------------- */
  float temp1, temp2;                       // Temperatures read from sensors
  bool boolTimeSwitch;                      // Indicate UTC 0) or QTH (1) time
  bool boolBacklight;                       // Indicate backlight on / off


/* ------------------------------------------------------------------------- *
 *       Main routine, repeating loop                                 loop()
 * ------------------------------------------------------------------------- */
void loop()
{
  /* 
   * Read backlight switch status and switch backlight on / off accordingly
   */
  switchBacklights();
  
  /* 
   * Read UTC / QTH switch and set time accordingly
   */
  timeSelect();
  
  /* 
   * Requesting temperatures... 
   */
  sensors.requestTemperatures();
  /* 
   * Why "byIndex"? You can have more than one DS18B20 on the same bus,
   * and 0 refers to the first IC on the wire 
   */
  temp1 = sensors.getTempCByIndex(0);       // temp in degrees Celcius from first sensor
  temp2 = sensors.getTempCByIndex(1);       // temp in degrees Celcius from second sensor
  
  /* 
   * Fill in temperatures in template on display 1
   */
  LCD_display(lcd1, 1,  6, String(temp1));  // display Celcius
  LCD_display(lcd1, 1, 13, String(temp2));  // display Celcius

  delay(100);
}


/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
    screen.setCursor(col, row);
    screen.print(text);
}
  

/* ------------------------------------------------------------------------- *
 *       Show initial screen, then paste template          doInitialScreen()
 * ------------------------------------------------------------------------- */
void doInitialScreen() {
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
   * Put template on LCD 2 
   */
  LCD_display(lcd2, 0, 0, "Station     NL14080");
  LCD_display(lcd2, 1, 0, "Operator    Gerard");
  LCD_display(lcd2, 2, 0, "QTH Locator JO33di");
}


/* ------------------------------------------------------------------------- *
 *       Read timeSelect switch into boolean                    timeSelect()
 * ------------------------------------------------------------------------- */
void timeSelect() {
  boolTimeSwitch = digitalRead(PIN_UTC_QTH);
  
  /* 
   * Fill in times in template on display 1
   */
  if (boolTimeSwitch == 1){
    LCD_display(lcd1, 3, 0, "Time JO33di 12:12:30");
  } else {
    LCD_display(lcd1, 3, 0, "Time UTC    10:12:30");
  }
}
  

/* ------------------------------------------------------------------------- *
 *       Read backlight pin & backlight on / off          switchBacklights()
 * ------------------------------------------------------------------------- */
void switchBacklights() {
  boolBacklight = digitalRead(PIN_BACKLIGHT);
  
  if (boolBacklight == 1) {
    lcd1.noDisplay(); 
    lcd1.noBacklight();
    lcd2.noDisplay(); 
    lcd2.noBacklight();
  } else {
    lcd1.display(); 
    lcd1.backlight();
    lcd2.display(); 
    lcd2.backlight();
  }
}
  

/* ------------------------------------------------------------------------- *
 *       One time setup routine, initial housekeeping
 * ------------------------------------------------------------------------- */
void setup()
{
  /* 
   * Initialize pins
   */
  pinMode(PIN_BACKLIGHT, INPUT_PULLUP);     // Initialize BackLight pin
  pinMode(PIN_UTC_QTH, INPUT_PULLUP);       // Initialize timeSelect pin

  /* 
   * Initialize wire library
   */
  Wire.begin();                              // Initialize I2C library
  
  /* 
   * Initialize screens
   */
  lcd1.init();                              // Initialize LCD Screen 1
  lcd2.init();                              // Initialize LCD Screen 1

  /* 
   * Initialize sensors
   */
  sensors.begin();                          // Initialize Temp sensors

  /* 
   * Initially switch backlights on
   */
  lcd1.backlight();                         // Backlights on by default
  lcd2.backlight();                         // Backlights on by default
  
  /* 
   * Initial screen display
   */
  doInitialScreen();                        // Paint initial screen
}

/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8....+....9...+*/
