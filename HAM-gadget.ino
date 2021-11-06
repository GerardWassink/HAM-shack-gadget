/* ------------------------------------------------------------------------- *
 * Name   : HAM-gadget
 * Author : Gerard Wassink
 * Date   : November 1, 2021
 * Purpose: Time, temp, GPS location indication with NTP fallback
 * Versions:
 *   0.1  : Initial code base, temp sensors working
 *   0.2  : Cleaned op the code
 *          built in backlight switch (on / off)
 *          built in UTC / QTH switch
 *   0.3  : Switched over to Nano RP2040 Connect
 *          Somehow had to switch from pin D3 to D5
 *   0.4  : Switched Back to Velleman Uno, the library did not work
 *            on the new architecture
 *   0.5    Built in the Velleman library VMA430_GPS
 *          Date and time now showing as UTC time from Satellite
 *          Latitude and Longitude also showing from Satellite
 *   0.6    Small corrections (Rxpin and TXpin switched)
 *   0.7    Built in temporary backlight switch: press the switch while
 *              in dark mode and the displays will light up as long as
 *              the switch is pressed
 *          Built in Zulu time (local Dutch time)
 *   0.8    Upgraded display precision of latitude / longitude to 8 decimals
 *   0.9    Built in a summer / winter time switch
 *   0.10   Code cleanup
 *          Better debugging method
 *   0.11   Display improvements of time and lat/long
 *   
 *   1.0    version 0.11 is the basis for release 1.0
 *   
 *   1.1    Remove separate switches, replace by keyboard menu
 *   1.2    Code cleanup
 *   1.3    Lead in time too long, corrected
 *          More code cleanup
 * ------------------------------------------------------------------------- */
#define progVersion "1.3"                   // Program version definition
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
 *       Debugging
 * ------------------------------------------------------------------------- */
#define DEBUG 0

#if DEBUG == 1
  #define debugstart(x) Serial.begin(x)
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debugstart(x)
  #define debug(x)
  #define debugln(x)
#endif

/* ------------------------------------------------------------------------- *
 *       Include libraries for peripherals 
 * ------------------------------------------------------------------------- */
#include <OneWire.h>                        // Onewire comms library
#include <DallasTemperature.h>              // Temperature library

#include <Wire.h>                           // I2C comms library
#include <LiquidCrystal_I2C.h>              // LCD library

#include <VMA430_GPS.h>                     // GPS module library
#include <SoftwareSerial.h>                 // Software serial library

#include <Keypad.h>                         // Keypad library

/* ------------------------------------------------------------------------- *
 *       Pin definitions for temp sensors
 * ------------------------------------------------------------------------- */
#define RXpin           2                   // TX & RX pins
#define TXpin           3                   //    to GPS
#define ONE_WIRE_BUS    4                   // Data wire plugged into this pin

/* ------------------------------------------------------------------------- *
 *       Other definitions
 * ------------------------------------------------------------------------- */
#define GPSbaud 9600                        // Baud rate to/from GPS
#define tempInterval 30000                  // time between temp requests
#define latLongInterval 1000                // time between lat/long displays
#define summerTimeOffset +2                 // Dutch summer time offset from UTC
#define winterTimeOffset +1                 // Dutch winter time offset from UTC

#define LOCAL true                          // Values determining
#define UTC false                           //   disaply of time type

#define WINTER true                         // Values determining
#define SUMMER false                        //   disaply of time type

#define ROWS 4                              // four rows for keyboard
#define COLS 4                              // four columns for keyboard


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
 *       Create objects for GPS module
 * ------------------------------------------------------------------------- */
SoftwareSerial ss(TXpin, RXpin);            // TX, RX
VMA430_GPS gps(&ss);                        // Pass object to GPS library

/* ------------------------------------------------------------------------- *
 *       Define keypad variables
 *       columns  1-4  connected to pins D13, D12, D11, D10
 *       row      1-4  connected to pins D9, D8, D7, D6
 * ------------------------------------------------------------------------- */
  char keys[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
  };
  byte rowPins[ROWS] = {9,8,7,6};           // row pins of the keypad
  byte colPins[COLS] = {13,12,11,10};       // column pins of the keypad
  
/* ------------------------------------------------------------------------- *
 *       Create objects for Keypad
 * ------------------------------------------------------------------------- */
  Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/* ------------------------------------------------------------------------- *
 *       Define global variables
 * ------------------------------------------------------------------------- */
  float temp1, temp2;                       // Temperatures read from sensors
  
  bool boolBacklight = true;                // Indicate backlight on / off
  bool boolTimeSwitch = UTC;                // Indicate UTC (0) or QTH (1) time
  bool boolSumWint = WINTER;                // Indicate summer / winter time

  String GPSdate;                           // Date from GPS
  String GPStime;                           // Time from GPS
  String zuluTime;                          // Local time NL

  float GPS_latitude;                       // Latitude from GPS
  float GPS_longitude;                      // Longitude from GPS
  
  long tempPreviousMillis = 30000;          // Make timeouts work first time
  long latlongPreviousMillis = 1000;        // Make timeouts work first time
  
/* ------------------------------------------------------------------------- *
 *       Main routine, repeating loop                                 loop()
 * ------------------------------------------------------------------------- */
void loop()
{
  /* 
   * Read key from keypad
   */
  char key = keypad.getKey();
  
  if (key != NO_KEY){
    
    switch (key) {
      case 'A': {
        boolBacklight = !boolBacklight;         // Switch backlight on / off
        break;
      }
      case 'B': {
        boolTimeSwitch = !boolTimeSwitch;       // UTC - Local time
        break;
      }
      case 'C': {
        boolSumWint = !boolSumWint;             // Summer - Winter time (DST)
        break;
      }
      default: {
        break;
      }
    }
    
  } else {
    
    /* 
     * Switch backlight on / off according to ststua
     */
    if (boolBacklight) {
      lcd1.backlight();
      lcd2.backlight();
    } else {
      lcd1.noBacklight();
      lcd2.noBacklight();
    }
    
    /* 
     * Display UTC / QTH date & time according to status
     */
    LCD_display(lcd1, 2,10, GPSdate);
    
    if (boolTimeSwitch == LOCAL){
      LCD_display(lcd1, 3, 0, "Local time");
      LCD_display(lcd1, 3,12, zuluTime); 
    } else {
      LCD_display(lcd1, 3, 0, "UTC time  ");
      LCD_display(lcd1, 3,12, GPStime); 
    }
    
    /* 
     * Read and display temperature(s)
     */
    unsigned long currentMillis = millis();
    if(currentMillis - tempPreviousMillis > tempInterval) {
      tempPreviousMillis = currentMillis;         // save the last time we requested temps
      /* 
       * Requesting temperatures, only at interval milli-seconds
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
    }
    
    /* 
     * Requesting GPS data... 
     */
    requestGPS();
    
  }
  
}


/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void requestGPS() {
  unsigned long currentMillis = millis();

  if (gps.getUBX_packet())                  // If a valid GPS UBX data packet is received...
  {
    gps.parse_ubx_data();                   // Parse new GPS data
    
    if (gps.utc_time.valid)                 // Valid utc_time data passed ?
    {

      debugln("GPS time received successfully");

      /* 
       * Form UTC date from GPS 
       */
      GPSdate = "";
      
      if (gps.utc_time.day < 10) {
        GPSdate.concat("0");
      }
      GPSdate.concat(gps.utc_time.day);
      
      GPSdate.concat("-");
      if (gps.utc_time.month < 10) {
        GPSdate.concat("0");
      }
      GPSdate.concat(gps.utc_time.month);
      GPSdate.concat("-");
      GPSdate.concat(gps.utc_time.year);
      
      /* 
       * Form UTC time from GPS 
       */
      GPStime = "";
      
      if (gps.utc_time.hour < 10) GPStime.concat("0");
      GPStime.concat(gps.utc_time.hour);
      
      GPStime.concat(":");

      if (gps.utc_time.minute < 10) GPStime.concat("0");
      GPStime.concat(gps.utc_time.minute);
      
      GPStime.concat(":");

      if (gps.utc_time.second < 10) GPStime.concat("0");
      GPStime.concat(gps.utc_time.second);
      
      /* 
       * Calculate and form local time from GPS time
       * Adding offset for summer or wintertime,
       * I know, it's a very crude method...
       */
      zuluTime = "";
      int zuluHour = 0;
      if (boolSumWint) {
        zuluHour = gps.utc_time.hour + winterTimeOffset;
      } else {
        zuluHour = gps.utc_time.hour + summerTimeOffset;
      }
      if (zuluHour < 10) zuluTime.concat("0");
      zuluTime.concat(zuluHour);
      zuluTime.concat(GPStime.substring(2,10));
      
      /*
       * Look for latitude /longtitude
       */
      GPS_latitude = float(gps.location.latitude);
      GPS_longitude = float(gps.location.longitude);
      
      if(currentMillis - latlongPreviousMillis > latLongInterval) {
        latlongPreviousMillis = currentMillis;         // save the last time we displayed
        
        /* 
         * Fill in lat/long in template on display 2
         */
        LCD_display(lcd2, 2,10, String(GPS_latitude, 6));
        LCD_display(lcd2, 3,10, String(GPS_longitude,6));
        
      }

    } else {

      debugln("error receiving GPS time");

      GPStime = "  :  :  ";
      zuluTime = "  :  :  ";
    }
  }
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
  LCD_display(lcd1, 1, 0, "     HAM-gadget     ");
  LCD_display(lcd1, 2, 0, "  Displaying stuff  ");
  LCD_display(lcd1, 3, 0, "Software vs.        ");
  LCD_display(lcd1, 3, 15, progVersion);

  LCD_display(lcd2, 0, 0, "   --- NL14080 ---  ");
  LCD_display(lcd2, 1, 0, "     HAM-gadget     ");
  LCD_display(lcd2, 2, 0, "  Displaying stuff  ");
  LCD_display(lcd2, 3, 0, "Software vs.        ");
  LCD_display(lcd2, 3, 15, progVersion);

  delay(3000);
  
  /* 
   * Put template text on LCD 1 
   */
  LCD_display(lcd1, 0, 0, "NL14080 --- (PD1GAW)");
  LCD_display(lcd1, 1, 0, "Temp  .....  ..... C");
  LCD_display(lcd1, 2, 0, "Date        -  -    ");
  LCD_display(lcd1, 3, 0, "                    ");
  /* 
   * Put template on LCD 2 
   */
  LCD_display(lcd2, 0, 0, "Station      NL14080");
  LCD_display(lcd2, 1, 0, "Op Gerard QTH JO33di");
  LCD_display(lcd2, 2, 0, "Latitude :          ");
  LCD_display(lcd2, 3, 0, "Longitude:          ");
  
}


/* ------------------------------------------------------------------------- *
 *       One time setup routine, initial housekeeping
 * ------------------------------------------------------------------------- */
void setup()
{
  /* 
   * Start debugging when so defined
   */
  debugstart(9600);
  debug("HAM-gadget version ");
  debug(progVersion);
  debugln(" - debugging start");
  
  /* 
   * Initialize several objects
   */                                               
  lcd1.init();                              // Initialize LCD Screen 1
  lcd2.init();                              // Initialize LCD Screen 1

  lcd1.backlight();                         // Backlights on by default
  lcd2.backlight();                         // Backlights on by default
  
  doInitialScreen();                        // Paint initial screen
  
  /* 
   * Initialize libraries
   */
  Wire.begin();                             // Initialize Wire library
  sensors.begin();                          // Initialize Temp sensors

  /* 
   * Start GPS communication
   */
  gps.begin(GPSbaud);                       // Set up GPS module to communicate over serial
  gps.setUBXNav();                          // Enable UBX navigation messages from the GPS module

}

/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8....+....9...+*/
