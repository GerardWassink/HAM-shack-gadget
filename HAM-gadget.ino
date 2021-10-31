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
 * ------------------------------------------------------------------------- */
#define progVersion "0.9"                   // Program version definition
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
 *       Debugging switch
 * ------------------------------------------------------------------------- */
//#define DEBUG                               // To debug or not to debug?

/* ------------------------------------------------------------------------- *
 *       Include libraries for peripherals 
 * ------------------------------------------------------------------------- */
#include <OneWire.h>                        // Onewire comms library
#include <DallasTemperature.h>              // Temperature library

#include <Wire.h>                           // I2C comms library
#include <LiquidCrystal_I2C.h>              // LCD library

#include <VMA430_GPS.h>                     // GPS module library
#include <SoftwareSerial.h>                 // Software serial library

/* ------------------------------------------------------------------------- *
 *       Pin definitions
 * ------------------------------------------------------------------------- */
#define RXpin           2                   // TX pin to GPS
#define TXpin           3                   // RX pin from GPS
#define ONE_WIRE_BUS    4                   // Data wire plugged into this pin
#define PIN_BACKLIGHT   5                   // Pin switch backlight on / off
#define PIN_UTC_QTH     6                   // Pin switch UTC and QTH time
#define PIN_BL_ON       7                   // Pin switch backlight on temporarily
#define PIN_SUM_WINT    8                   // Pin switch summer / wintertime

/* ------------------------------------------------------------------------- *
 *       Other definitions
 * ------------------------------------------------------------------------- */
#define GPSbaud 9600                        // Baud rate to/from GPS
#define tempInterval 30000                  // time between temp requests
#define BL_OnTime 3000                      // time backlight on after activation
#define summerTimeOffset +2                 // Dutch summer time offset from UTC
#define winterTimeOffset +1                 // Dutch winter time offset from UTC

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
VMA430_GPS gps(&ss);                        // Pass SoftwareSerial object to 
                                            //   GPS module library

/* ------------------------------------------------------------------------- *
 *       Define global variables
 * ------------------------------------------------------------------------- */
  float temp1, temp2;                       // Temperatures read from sensors
  bool boolTimeSwitch;                      // Indicate UTC 0) or QTH (1) time
  bool boolBacklight;                       // Indicate backlight on / off

  String GPSdate;                           // Date from GPS
  String GPStime;                           // Time from GPS
  String zuluTime;                          // Local time NL

  float GPS_latitude;                       // Latitude from GPS
  float GPS_longitude;                      // Longitude from GPS
  
  long previousMillis = 30000;              // last time temps were requested
  
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
  displayTime();
  
  /* 
   * Read and display temperature(s)
   */
  displayTemp();
  
  /* 
   * Requesting GPS data... 
   */
  requestGPS();
  
  delay(10);
}


/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void requestGPS() {

  if (gps.getUBX_packet())                  // If a valid GPS UBX data packet is received...
  {
    gps.parse_ubx_data();                   // Parse new GPS data
    
    if (gps.utc_time.valid)                 // Valid utc_time data passed ?
    {

#ifdef DEBUG
  Serial.println("GPS time received successfully");
#endif

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
       * Just add 2 to the hour for now,
       * I know, it's a very crude method...
       */
      zuluTime = "";
      int zuluHour = 0;
      if (!digitalRead(PIN_SUM_WINT)) {
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
      
      LCD_display(lcd2, 2, 6, String(GPS_latitude, 8));
      LCD_display(lcd2, 3, 6, String(GPS_longitude,8));
      
    } else {
#ifdef DEBUG
      Serial.println("error receiving GPS time");
#endif
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
  LCD_display(lcd1, 1, 0, "  HAM contraption   ");
  LCD_display(lcd1, 2, 0, "  Displaying stuff  ");
  LCD_display(lcd1, 3, 0, "Software version    ");
  LCD_display(lcd1, 3, 17, progVersion);

  LCD_display(lcd2, 0, 0, "   --- NL14080 ---  ");
  LCD_display(lcd2, 1, 0, "  HAM contraption   ");
  LCD_display(lcd2, 2, 0, "  Displaying stuff  ");
  LCD_display(lcd2, 3, 0, "Software version    ");
  LCD_display(lcd2, 3, 17, progVersion);

  delay(3000);
  
  /* 
   * Put template text on LCD 1 
   */
  LCD_display(lcd1, 0, 0, "NL14080 --- Sats: nn");
  LCD_display(lcd1, 1, 0, "Temp  .....  ..... C");
  LCD_display(lcd1, 2, 0, "Date        -  -    ");
  LCD_display(lcd1, 3, 0, "                    ");
  /* 
   * Put template on LCD 2 
   */
  LCD_display(lcd2, 0, 0, "Station      NL14080");
  LCD_display(lcd2, 1, 0, "Op Gerard QTH JO33di");
  LCD_display(lcd2, 2, 0, "Lat :               ");
  LCD_display(lcd2, 3, 0, "Long:               ");
  
}


/* ------------------------------------------------------------------------- *
 *       Read timeSelect switch into boolean                    timeSelect()
 * ------------------------------------------------------------------------- */
void displayTime() {
  boolTimeSwitch = digitalRead(PIN_UTC_QTH);
  
  /* 
   * Fill in date and time in template on display 1
   */
  LCD_display(lcd1, 2,10, GPSdate); 
  
  if (boolTimeSwitch == 0){
    LCD_display(lcd1, 3, 0, "Local time          ");
    LCD_display(lcd1, 3,12, zuluTime); 
  } else {
    LCD_display(lcd1, 3, 0, "UTC time            ");
    LCD_display(lcd1, 3,12, GPStime); 
  }
}
  

/* ------------------------------------------------------------------------- *
 *       Display temperature every interval milliseconds       displayTemp()
 * ------------------------------------------------------------------------- */
void displayTemp() {
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis > tempInterval) {
    previousMillis = currentMillis;         // save the last time we requested temps
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
}


/* ------------------------------------------------------------------------- *
 *       Read backlight pin & backlight on / off          switchBacklights()
 * ------------------------------------------------------------------------- */
void switchBacklights() {
  boolBacklight = digitalRead(PIN_BACKLIGHT);
  
  if (!digitalRead(PIN_BL_ON)) {
    lcd1.backlight();
    lcd2.backlight();
  } else {
    if (boolBacklight == 0) {
  //    lcd1.noDisplay(); 
      lcd1.noBacklight();
  //    lcd2.noDisplay(); 
      lcd2.noBacklight();
    } else {
  //    lcd1.display(); 
      lcd1.backlight();
  //    lcd2.display(); 
      lcd2.backlight();
    }
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
#ifdef DEBUG
  Serial.begin(9600);
  Serial.print("HAM-gadget version ");
  Serial.print(progVersion);
  Serial.println(" - debugging start");
#endif
  
  /* 
   * Initialize pins
   */
  pinMode(PIN_BACKLIGHT, INPUT_PULLUP);     // BackLight pin
  pinMode(PIN_UTC_QTH, INPUT_PULLUP);       // timeSelect pin
  pinMode(PIN_BL_ON, INPUT_PULLUP);         // temp backlight pin
  pinMode(PIN_SUM_WINT, INPUT_PULLUP);      // summer / wintertime pin
  
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
