/* ------------------------------------------------------------------------- *
 * Name   : HAM-gadget
 * Author : Gerard Wassink
 * Date   : July 22, 2023
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
 *   1.4    Screeens improved
 *          Some code improvements
 *          updated Readme
 *   1.5    made a start with menu screens
 *          Some memory management by using the F() function
 *              storing strings in program memory
 *   1.6    Facilitate storing setting to EEPROM 
 *              and reading them at startup as defaults
 *   1.7    Little errors in menu screen corrected
 *          Menu structure expanded
 *          Saving / retrieving Settings improved
 *   1.8    Display issue solved with potentially negative offsets
 *          Added QTH locator JO33di
 *   1.8a   correction of version number...
 *   1.9    Read defaults at startup with yes/no question
 *   1.10   Time out built in for startup question
 *          Screen change for settings display
 *   
 *   2.0    version 1.10 is the basis for release 2.0
 *   
 *   2.1    Built in possibility for adjustment for summer and wintertime 
 *              relative to UTC
 *   2.2    Cleaned op the code
 *   2.3    Built in Maidenhead locator code calculation:
 *              now being derived from GPS Latitude / Longitude
 *   
 *   3.0    version 2.3 is the basis for release 3.0
 *   3.1    Stripped back to displaying only one temperature
 *   3.2    Passed Novice exam, channged NL14080 to my call sign, PD1GAW
 *   3.3    Backlight on/off time window
 *          Backlight on/off using zulu time now
 *   3.4    Preperations for bringing back to one screen
 *   
 * ------------------------------------------------------------------------- */
#define progVersion "3.4"                   // Program version definition
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
 *       Copyright (C) November 2021 Gerard Wassink
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 *       Switch debugging on / off (compiler directives)
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
 *       Include libraries for peripherals & Arduino stuff
 * ------------------------------------------------------------------------- */
#include <OneWire.h>                        // Onewire comms library
#include <DallasTemperature.h>              // Temperature library

#include <Wire.h>                           // I2C comms library
#include <LiquidCrystal_I2C.h>              // LCD library

#include <VMA430_GPS.h>                     // GPS module library
#include <SoftwareSerial.h>                 // Software serial library

#include <Keypad.h>                         // Keypad library

#include <EEPROM.h>                         // EEPROM library

/* ------------------------------------------------------------------------- *
 *       Pin definitions for GPS and temp sensors
 * ------------------------------------------------------------------------- */
#define RXpin           2                   // TX & RX pins
#define TXpin           3                   //    to GPS
#define ONE_WIRE_BUS    4                   // Data wire plugged into this pin

/* ------------------------------------------------------------------------- *
 *       Other definitions
 * ------------------------------------------------------------------------- */
#define GPSbaud 9600                        // Baud rate to/from GPS
#define tempInterval 5000                   // time between temp requests
#define latLongInterval 1000                // time between lat/long displays
#define bootQuestionInterval 9000           // wait time for bootup question
#define maidenheadInterval 11000            // time between Maidenhaid calculations
#define timeInterval 1300                   // time between time calculations

#define ON true                             // Values determining
#define OFF false                           //   true or false

#define LOCAL true                          // Values determining
#define UTC false                           //   disaply of time type

#define WINTER true                         // Values determining
#define SUMMER false                        //   disaply of time type

/* ------------------------------------------------------------------------- *
 *       Definitions for the keyboard
 * ------------------------------------------------------------------------- */
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
// LiquidCrystal_I2C lcd2(0x26,20,4);          // Initialize display 2

/* ------------------------------------------------------------------------- *
 *       Create objects for GPS module
 * ------------------------------------------------------------------------- */
SoftwareSerial ss(TXpin, RXpin);            // TX, RX
VMA430_GPS gps(&ss);                        // Pass object to GPS library

/* ------------------------------------------------------------------------- *
 *       Create structure and object for settings to store them to EEPROM
 * ------------------------------------------------------------------------- */
struct Settings {
  bool localUTC;                            // to store UTC/Local time
  bool summerWinter;                        // to store DST switch
  int  timeOffsetDST;                       // to store local time offset
  int  timeOffsetNoDST;                     // to store local time offset
  int  backlightOnHour;                     // to store backlight On value
  int  backlightOffHour;                    // to store backlight Off value
};
Settings mySettings;                        // Create the object

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
  float temp1;                              // Temperature read from sensor
  
  bool signalReceived = OFF;                // Do we have a signal already?
  
  bool boolBacklight    = ON;               // Indicate backlight desired on/off
  int  backlightOnHour  = 7;                // Backlight switched on hour
  int  backlightOffHour = 17;               // Backlight switched off hour
  int  currentHour      = 0;                // Hour from current UTC time
  int  prevCurrentHour  = 0;
  
  bool boolTimeSwitch = UTC;                // Indicate UTC (0) or QTH (1) time
  bool boolSumWint = WINTER;                // Indicate summer / winter time
  
  String GPSdate;                           // Date from GPS
  String GPStime;                           // Time from GPS
  String zuluTime;                          // Local time derived from GPS time
  
  double GPS_latitude;                       // Latitude from GPS
  double GPS_longitude;                      // Longitude from GPS
  
  long tempPreviousMillis = 5000;           // Make timeouts work first time
  long latlongPreviousMillis = 1000;        // Make timeouts work first time
  long bootQuestionPreviousMillis = 3000;   // Make timeouts work first time
  long maidenheadPreviousMillis = 5000;     // Make timeouts work first time
  long timePreviousMillis = timeInterval + 1; // Make timeouts work first time
  int summerTimeOffset = +2;                // Dutch summer time offset from UTC
  int winterTimeOffset = +1;                // Dutch winter time offset from UTC
  
  String msg = "                    ";      // Initial value for screen message
  
  String locatorCode = "";                  // Maidenhead locator code
  
  
/* ------------------------------------------------------------------------- *
 *       Main routine, repeating loop                                 loop()
 * ------------------------------------------------------------------------- */
void loop()
{
  /* 
   * Read key from keypad
   */
  char key = keypad.getKey();                   // get key press
  
  if (key != NO_KEY){                           // did we receive one?
    
    switch (key) {
      case 'A': {
        boolBacklight = !boolBacklight;         // Switch backlight on / off
        break;
      }
      case '*': {
        mainMenu();                             // Go menu structure and do things
        doTemplates();                          //  on return: restore screens
        break;
      }
      default: {
        break;
      }
    }
    
  } else {                                      // no key received:
    
    /* --------------------------------------------------------------------- *
     * Display UTC / QTH date & time according to status
     * --------------------------------------------------------------------- */
    if (signalReceived) {                       // GPS sat in the picture yet?
      LCD_display(lcd1, 2,10, GPSdate);         // Display date
      
      if (boolTimeSwitch == LOCAL){             // determine  time to display
        LCD_display(lcd1, 3, 0, F("Local time  "));
        LCD_display(lcd1, 3,12, zuluTime); 
//        LCD_display(lcd2, 1, 0, F("Local time  "));
//        LCD_display(lcd2, 1,12, zuluTime); 
      } else {
        LCD_display(lcd1, 3, 0, F("UTC time    "));
        LCD_display(lcd1, 3,12, GPStime); 
//        LCD_display(lcd2, 1, 0, F("UTC time    "));
//        LCD_display(lcd2, 1,12, GPStime); 
      }
      
      LCD_display(lcd1, 0,13,locatorCode);      // display when signal present
    } else {                                    // No GPS signal yet
      LCD_display(lcd1, 3, 0, F("Waiting for GPS sat."));
//      LCD_display(lcd2, 1, 0, F("Waiting for GPS sat."));
    }
    
    /* --------------------------------------------------------------------- *
     *                                                          Timed events
     * --------------------------------------------------------------------- */
    
    /*                                    Establish current time in millis() */
    unsigned long currentMillis = millis();
    
    /*        Switch backlight on / off according to time and desired status */
    if(currentMillis - timePreviousMillis > timeInterval) {
      timePreviousMillis = currentMillis;
      
      if (currentHour != prevCurrentHour) {     // We have an hour-change
        
        if (currentHour == backlightOnHour) {
          boolBacklight = ON;                   // Daylight, default on
        }
        if (currentHour == backlightOffHour) {
          boolBacklight = OFF;                  // At night, default off
        }
        prevCurrentHour = currentHour;
      }
    }
    
    /*                         Switch backlight on / off according to status */
    if (boolBacklight) {
      lcd1.backlight();
//      lcd2.backlight();
    } else {
      lcd1.noBacklight();
//      lcd2.noBacklight();
    }
    
    /*                             Calculate 6 digit Maidenhead locator code */
    if(currentMillis - maidenheadPreviousMillis > maidenheadInterval) {
      maidenheadPreviousMillis = currentMillis;
      calcMaidenhead();
    }
    
    /*                   Read and display temperature(s), but not every time */
    if(currentMillis - tempPreviousMillis > tempInterval) {
      tempPreviousMillis = currentMillis;

      /*         Requesting temperatures, only at tempInterval milli-seconds */
      sensors.requestTemperatures();

      /*  Why "byIndex"? You can have more than one DS18B20 on the same bus, */
      /*                            and 0 refers to the first IC on the wire */
      temp1 = sensors.getTempCByIndex(0);

      /*                        Fill in temperature in template on display 1 */
      LCD_display(lcd1, 1, 13, String(temp1));
    }
    
    /*                             Requesting GPS data every time around...  */
    requestGPS();
  }
}


/* ------------------------------------------------------------------------- *
 *       Routine to calculate 6 digit Maidenhead locator    calcMaidenhead()
 * ------------------------------------------------------------------------- */
void calcMaidenhead() {

  /*                                                 Calculate Field Letters */
  double lonplus = GPS_longitude + 180;
  int numFirstLetter = lonplus / 20;
  double remFirstLetter = lonplus - (numFirstLetter * 20);
  
  double latplus = GPS_latitude + 90;
  int numSecondLetter = latplus / 10;
  double remSecondLetter = latplus - (numSecondLetter * 10);

  /*                                                Calculate Square numbers */
  int digOneSquare = (int)(remFirstLetter / 2);
  double remOneSquare = remFirstLetter - (digOneSquare * 2);
  
  int digTwoSquare = (int)(remSecondLetter / 1);
  double remTwoSquare = remSecondLetter - (digTwoSquare * 1);
  
  /*                                            Calculate Sub-Square Letters */
  int digOneSubSquare = remOneSquare / 0.083333;
  int digTwoSubSquare = remTwoSquare / 0.0416;
  
  /*                                                    Display Locator code */
  locatorCode = String((char)('A' + numFirstLetter));
  locatorCode.concat(String((char)('A' + numSecondLetter)));
  locatorCode.concat(String(digOneSquare));
  locatorCode.concat(String(digTwoSquare));
  locatorCode.concat(String((char)('a' + digOneSubSquare)));
  locatorCode.concat(String((char)('a' + digTwoSubSquare)));
}


/* ------------------------------------------------------------------------- *
 *       Get info from the satellite and decode / format it     requestGPS()
 * ------------------------------------------------------------------------- */
void requestGPS() {
  unsigned long currentMillis = millis();
  
  if (gps.getUBX_packet())                  // valid GPS UBX packet received
  {
    gps.parse_ubx_data();                   // Parse new GPS data
    signalReceived = true;
    
    if (gps.utc_time.valid)                 // Valid utc_time data passed ?
    {
      debugln(F("GPS time received successfully"));
      
      /*                                           Form UTC date from GPS    */
      GPSdate = "";
      if (gps.utc_time.day < 10) {          // Leading zero?
        GPSdate.concat("0");
      }
      GPSdate.concat(gps.utc_time.day);
      
      GPSdate.concat("-");
      if (gps.utc_time.month < 10) {        // Leading zero?
        GPSdate.concat("0");
      }
      GPSdate.concat(gps.utc_time.month);
      GPSdate.concat("-");
      GPSdate.concat(gps.utc_time.year);
      
      /*                                           Form UTC time from GPS    */
      GPStime = "";

      if (gps.utc_time.hour < 10)           // Leading zero?
        GPStime.concat("0");
      GPStime.concat(gps.utc_time.hour);
      
      GPStime.concat(":");

      if (gps.utc_time.minute < 10)         // Leading zero?
        GPStime.concat("0");
      GPStime.concat(gps.utc_time.minute);
      
      GPStime.concat(":");

      if (gps.utc_time.second < 10)         // Leading zero?
        GPStime.concat("0");
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
      
      /*                           use local time to switch display on / off */
      currentHour = zuluHour;
      
      /*                                       Look for latitude /longtitude */
      if(currentMillis - latlongPreviousMillis > latLongInterval) {
        latlongPreviousMillis = currentMillis;         // save the last time we displayed
        
        GPS_latitude  = gps.location.latitude;
        GPS_longitude = gps.location.longitude;
        
        /*                         Fill in lat/long in template on display 2 */
// QQQQQQQQ
//        LCD_display(lcd2, 2,10, String(GPS_latitude, 6));
//        LCD_display(lcd2, 3,10, String(GPS_longitude, 6));
        
      }
    
    } else {
    
      debugln(F("error receiving GPS time"));
      signalReceived = false;
      GPStime = "  :  :  ";
      zuluTime = "  :  :  ";
    }
  }
}
  

/* ------------------------------------------------------------------------- *
 *       Show initial screen, then paste template          doInitialScreen()
 * ------------------------------------------------------------------------- */
void doInitialScreen(int s) {
  
  debugln("Entering doInitialScreen");
  
  LCD_display(lcd1, 0, 0, F("PD1GAW ---- (JO33di)"));
  LCD_display(lcd1, 1, 0, F("HAM-gadget vs.      "));
  LCD_display(lcd1, 1, 15, progVersion);
  LCD_display(lcd1, 2, 0, F("(c) Gerard Wassink  "));
  LCD_display(lcd1, 3, 0, F("GNU public license  "));

//  LCD_display(lcd2, 0, 0, F("Displaying:         "));
//  LCD_display(lcd2, 1, 0, F("Room temperature    "));
//  LCD_display(lcd2, 2, 0, F("GPS time, UTC/Local "));
//  LCD_display(lcd2, 3, 0, F("Latitude / Longitude"));
  
  delay(s * 1000);
  
  doTemplates();
}


/* ------------------------------------------------------------------------- *
 *       Paste templates                                       doTemplates()
 * ------------------------------------------------------------------------- */
void doTemplates()
{
  
  debugln("Entering doTemplates");
  
  /* 
   * Put template text on LCD 1 
   */
  LCD_display(lcd1, 0, 0, F("PD1GAW ---- (      )"));
  LCD_display(lcd1, 1, 0, F("Shack temp   _____ C"));
  LCD_display(lcd1, 2, 0, F("Date        -  -    "));
  LCD_display(lcd1, 3, 0, F("                    "));
  /* 
   * Put template on LCD 2 
   */
//  LCD_display(lcd2, 0, 0, F("Operator      Gerard"));
//  LCD_display(lcd2, 1, 0, F("                    "));
//  LCD_display(lcd2, 2, 0, F("Latitude            "));
//  LCD_display(lcd2, 3, 0, F("Longitude           "));
  
}


/* ------------------------------------------------------------------------- *
 *       Show main menu                                           mainMenu()
 * ------------------------------------------------------------------------- */
void mainMenu()
{
  char choice = ' ';
  bool endLoop = false;
  
  debugln("Entering mainMenu");
  
  displayMainMenu();
  
  while (!endLoop) {
    choice = keypad.getKey();
    switch (choice) {
      case '1': {
        doTimeMenu();
        displayMainMenu();
        break;
      }
      case '2': {
        doSettingsMenu();
        displayMainMenu();
        break;
      }
      case '3': {
        doPowerSaveMenu();
        displayMainMenu();
        break;
      }
      case '4': {
        doInitialScreen(5);                     // Credits - description
        displayMainMenu();
        break;
      }
      case '#': {
        endLoop = true;
        break;
      }
      default: {
        break;
      }
    }
    delay(100);
  }
}


/* ------------------------------------------------------------------------- *
 *       Perform Time menu                                      doTimeMenu()
 * ------------------------------------------------------------------------- */
void doTimeMenu()
{
  char choice = ' ';
  bool endLoop = false;
  
  debugln("Entering doTimeMenu");

  displayTimeMenu();

  while (!endLoop) {
    choice = keypad.getKey();
    switch (choice) {
      case '1': {
        boolTimeSwitch = UTC;
        LCD_display(lcd1, 0, 0, F("OKAY :: UTC Time    "));
        delay(500);
        displayTimeMenu();
        break;
      }
      case '2': {
        boolTimeSwitch = LOCAL;
        boolSumWint    = WINTER;
        LCD_display(lcd1, 1, 0, F("OKAY :: Wintertime  "));
        delay(500);
        displayTimeMenu();
        break;
      }
      case '3': {
        boolTimeSwitch = LOCAL;
        boolSumWint    = SUMMER;
        LCD_display(lcd1, 2, 0, F("OKAY :: Summertime  "));
        delay(500);
        displayTimeMenu();
        break;
      }
      case '4': {
        timeAdjustMenu();
        displayTimeMenu();
        break;
      }
      case '#': {
        endLoop = true;
        break;
      }
      default: {
        break;
      }
    }
    delay(100);
  }
}


/* ------------------------------------------------------------------------- *
 *       Perform PowerSave menu                            doPowerSaveMenu()
 * ------------------------------------------------------------------------- */
void doPowerSaveMenu()
{
  char choice = ' ';
  bool endLoop = false;
  int v = 0;
  
  debugln("Entering doPowerSaveMenu");

  displayPowerSaveMenu();

  while (!endLoop) {
    choice = keypad.getKey();
    switch (choice) {
      case '1': {
        v = enterIntValue();
        // check for proper value
        if ((v < 0) || (v >23)) {
          msg = "wrong value ";
          msg.concat(String(v));
        } else {
          backlightOnHour = v;
        }
        
        delay(500);
        displayPowerSaveMenu();
        break;
      }
      case '2': {
        v = enterIntValue();
        // check for proper value
        if ((v < 0) || (v >23)) {
          msg = "wrong value ";
          msg.concat(String(v));
        } else {
          backlightOffHour = v;
        }
        
        delay(500);
        displayPowerSaveMenu();
        break;
      }
      case '#': {
        endLoop = true;
        break;
      }
      default: {
        break;
      }
    }
    delay(100);
  }
}


/* ------------------------------------------------------------------------- *
 *       Screen to adjust time offsets                      timeAdjustMenu()
 * ------------------------------------------------------------------------- */
void timeAdjustMenu() {
  char choice = ' ';
  bool endLoop = false;
  int val = 0;
  
  debugln("Entering doAdjustMenu");

  displayAdjustMenu();

  while (!endLoop) {
    choice = keypad.getKey();
    switch (choice) {
      case '1': {
        winterTimeOffset = enterOffset(WINTER);
        delay(500);
        displayAdjustMenu();
        break;
      }
      case '2': {
        summerTimeOffset = enterOffset(SUMMER);
        delay(500);
        displayAdjustMenu();
        break;
      }
      case '#': {
        endLoop = true;
        break;
      }
      default: {
        break;
      }
    }
    delay(100);
  }
}


/* ------------------------------------------------------------------------- *
 *       Enter an integer value from keyboard                   enterOfset()
 * ------------------------------------------------------------------------- */
int enterOffset(bool season) {
  char choice = ' ';
  bool endLoop = false;
  bool negative = false;
  int v = 0;
  int col;
  
  // display options
  LCD_display(lcd1, 0, 0, F("Adjusting offset for"));
  if (season == WINTER) {
    LCD_display(lcd1, 1, 0, F("wintertime          "));
  } else {
    LCD_display(lcd1, 1, 0, F("summertime          "));
  }
  LCD_display(lcd1, 2, 0, F("use *for minus sign:"));
  LCD_display(lcd1, 3, 0, msg);
  
  // read and echo value (* for minus sign, # for enter)
  col = 0;
  while (!endLoop) {
    choice = keypad.getKey();
    switch (choice) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        LCD_display(lcd1, 3, col++, String(choice));
        v = 10 * v + choice - '0';
        break;
        
      case '*':
        negative = !negative;
        LCD_display(lcd1, 3, col++, "-");
        break;
        
      case '#':
        endLoop = true;
        break;
        
      default:
        break;
    }
    delay(100);
  }
  
  // Calculate sign
  if (negative) v = v * -1;
  
  // check for proper value
  if ((v > -13) && (v < 13)) {
    msg = (season == WINTER ? "winter" : "summer");
    msg.concat("offset changed");
  } else {
    msg = " wrong value ";
    msg.concat(String(v));
  }
  return(v);
}


/* ------------------------------------------------------------------------- *
 *       Enter an integer value from keyboard                enterIntValue()
 * ------------------------------------------------------------------------- */
int enterIntValue() {
  char choice = ' ';
  bool endLoop = false;
  bool negative = false;
  int v = 0;
  int col;
  
  // display options
  LCD_display(lcd1, 0, 0, F("Enter value         "));
  LCD_display(lcd1, 1, 0, F("use *for minus sign:"));
  LCD_display(lcd1, 2, 0, F("    #for ENTER      "));
  LCD_display(lcd1, 3, 0, msg);
  
  // read and echo value (* for minus sign, # for enter)
  col = 0;
  while (!endLoop) {
    choice = keypad.getKey();
    switch (choice) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        LCD_display(lcd1, 3, col++, String(choice));
        v = 10 * v + choice - '0';
        break;
        
      case '*':
        negative = !negative;
        LCD_display(lcd1, 3, col++, "-");
        break;
        
      case '#':
        endLoop = true;
        break;
        
      default:
        break;
    }
    delay(100);
  }
  
  // Calculate sign
  if (negative) v = v * -1;

  return(v);
}


/* ------------------------------------------------------------------------- *
 *       Perform Settings menu                              doSettingsMenu()
 * ------------------------------------------------------------------------- */
void doSettingsMenu()
{
  char choice = ' ';
  bool endLoop = false;
  
  debugln("Entering doSettingsMenu");

  displaySettingsMenu();

  while (!endLoop) {
    choice = keypad.getKey();
    switch (choice) {
      case '1': {
        showSettings();
        displaySettingsMenu();
        break;
      }
      case '2': {
        storeSettings();
        LCD_display(lcd1, 1, 0, F("OK, Settings stored "));
        delay(500);
        displaySettingsMenu();
        break;
      }
      case '3': {
        getSettings();
        LCD_display(lcd1, 2, 0, F("OK, Got Settings    "));
        delay(500);
        displaySettingsMenu();
        break;
      }
      case '4': {
        break;
      }
      case '#': {
        endLoop = true;
        break;
      }
      default: {
        break;
      }
    }
    delay(100);
  }
}


/* ------------------------------------------------------------------------- *
 *       Show settings screen                                 showSettings()
 * ------------------------------------------------------------------------- */
void showSettings() {
  bool endLoop = false;
  char choice;
  
  if (boolTimeSwitch == UTC) {
    
    LCD_display(lcd1, 0, 0, F("Showing: UTC time   "));
    LCD_display(lcd1, 1, 0, F("                    "));
    LCD_display(lcd1, 2, 0, F("                    "));
    LCD_display(lcd1, 3, 0, F("                    "));
    
  } else {
    
    if (boolSumWint == WINTER) {
      LCD_display(lcd1, 0, 0, F("Showing: winter time"));
    } else {
      LCD_display(lcd1, 0, 0, F("Showing: summer time"));
    }
  }
    
  LCD_display(lcd1, 1, 0, F("Offset wintertim    "));
  LCD_display(lcd1, 1,17, String(winterTimeOffset));
  
  LCD_display(lcd1, 2, 0, F("Offset summertim    "));
  LCD_display(lcd1, 2,17, String(summerTimeOffset));
  
  LCD_display(lcd1, 3, 0, F("                    "));
  
  while (!endLoop) {                        // Keep showing until '#' is pressed
    choice = keypad.getKey();
    if (choice == '#') endLoop = true;
    delay(100);
  }
}


/* ------------------------------------------------------------------------- *
 *       Store settings to EEPROM                            storeSettings()
 * ------------------------------------------------------------------------- */
void storeSettings() {
  /*
   * Store settings in mySettings structure
   */
  mySettings.localUTC         = boolTimeSwitch;
  mySettings.summerWinter     = boolSumWint;
  mySettings.timeOffsetDST    = winterTimeOffset;
  mySettings.timeOffsetNoDST  = summerTimeOffset;
  mySettings.backlightOnHour  = backlightOnHour;
  mySettings.backlightOffHour = backlightOffHour;

  /*
   * Store mySettings structure to EEPROM
   */
  EEPROM.put(0, mySettings);
}


/* ------------------------------------------------------------------------- *
 *       Retrieve settings from EEPROM                         getSettings()
 * ------------------------------------------------------------------------- */
void getSettings() {
  /*
   * Store mySettings structure to EEPROM
   */
  EEPROM.get(0, mySettings);

  /*
   * Store settings in mySettings structure
   */
  boolTimeSwitch    = mySettings.localUTC;
  boolSumWint       = mySettings.summerWinter;
  winterTimeOffset  = mySettings.timeOffsetDST;
  summerTimeOffset  = mySettings.timeOffsetNoDST;
  backlightOnHour   = mySettings.backlightOnHour;
  backlightOffHour  = mySettings.backlightOffHour;
}


/* ------------------------------------------------------------------------- *
 *       Show the main menu screen                         displayMainMenu()
 * ------------------------------------------------------------------------- */
void displayMainMenu()
{
  LCD_display(lcd1, 0, 0, F("1. Time Menu        "));
  LCD_display(lcd1, 1, 0, F("2. Settings         "));
  LCD_display(lcd1, 2, 0, F("3. Adjust PowerSave "));
  LCD_display(lcd1, 3, 0, F("4. Credits / info   "));
}


/* ------------------------------------------------------------------------- *
 *       Show the time menu screen                         displayTimeMenu()
 * ------------------------------------------------------------------------- */
void displayTimeMenu() {
  LCD_display(lcd1, 0, 0, F("1. Show UTC Time    "));
  LCD_display(lcd1, 1, 0, F("2. Local wintertime "));
  LCD_display(lcd1, 2, 0, F("3. Local summertime "));
  LCD_display(lcd1, 3, 0, F("4. Adjust offsets   "));
}


/* ------------------------------------------------------------------------- *
 *       Show the adjsut time offest menu screen         displayAdjustMenu()
 * ------------------------------------------------------------------------- */
void displayAdjustMenu() {
  LCD_display(lcd1, 0, 0, F("Adjust Time Offsets "));
  LCD_display(lcd1, 1, 0, F("1. Adjust wintertime"));
  LCD_display(lcd1, 2, 0, F("2. Adjust summertime"));
  LCD_display(lcd1, 3, 0, msg);
  msg = F("                    ");
}


/* ------------------------------------------------------------------------- *
 *       Show the settings menu screen                 displaySettingsMenu()
 * ------------------------------------------------------------------------- */
void displaySettingsMenu() {
  LCD_display(lcd1, 0, 0, F("1. Show settings    "));
  LCD_display(lcd1, 1, 0, F("2. Store settings   "));
  LCD_display(lcd1, 2, 0, F("3. Retrieve settings"));
  LCD_display(lcd1, 3, 0, F("                    "));
}
  

/* ------------------------------------------------------------------------- *
 *       Show the power savings menun                 displayPowerSaveMenu()
 * ------------------------------------------------------------------------- */
void displayPowerSaveMenu() {
  LCD_display(lcd1, 0, 0, F("Adjust PowerSave    "));
  LCD_display(lcd1, 1, 0, F("1. OnHour           "));
  LCD_display(lcd1, 1,11, F("         "));
  LCD_display(lcd1, 1,11, String(backlightOnHour));
  LCD_display(lcd1, 2, 0, F("2. OffHour          "));
  LCD_display(lcd1, 2,11, F("         "));
  LCD_display(lcd1, 2,11, String(backlightOffHour));
  LCD_display(lcd1, 3, 0, F("                    "));
  LCD_display(lcd1, 3, 0, msg);
  msg = F("                    ");
}
  

/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
    screen.setCursor(col, row);
    screen.print(text);
}


/* ------------------------------------------------------------------------- *
 *       One time setup routine, initial housekeeping
 * ------------------------------------------------------------------------- */
void setup()
{
  bool endLoop = false;
  char choice;
  unsigned long currentMillis = millis();
  
  /*                                         Start debugging when so defined */
  debugstart(115200);
  debug("HAM-gadget version ");
  debug(progVersion);
  debugln(" - debugging start");
  
  /*                                              Initialize several objects */
  
  lcd1.init();                              // Initialize LCD Screen 1
//  lcd2.init();                              // Initialize LCD Screen 1

  lcd1.backlight();                         // Backlights on by default
//  lcd2.backlight();                         // Backlights on by default

  /*                        Does user want to retrieve settings from EEPROM? */
  LCD_display(lcd1, 0, 0, F("Load settings?      "));
  LCD_display(lcd1, 1, 0, F(" (*) yes   (default)"));
  LCD_display(lcd1, 2, 0, F(" (#) no             "));
  LCD_display(lcd1, 3, 0, F("Your choice please: "));
  
  bootQuestionPreviousMillis = currentMillis;
  while (!endLoop) {
    currentMillis = millis();    
    
    choice = keypad.getKey();
    switch (choice) {
      case '*': {
        getSettings();                      // Get settings from EEPROM
        endLoop = true;
        break;
      }
      case '#': {
        endLoop = true;
        break;
      }
      default: {
        break;
      }
    }
    delay(100);
    
    /*             User gets maximum of bootQuestionInterval time to react   */
    /*     if no answer, default is NOT to retreive the settings from EEPROM */
    if(currentMillis - bootQuestionPreviousMillis > bootQuestionInterval) {
      bootQuestionPreviousMillis = currentMillis;
      
      getSettings();                        // Get settings from EEPROM
      endLoop = true;                       // End of wait time, default is to retrieve 
                                            //   the settings from EEPROM
    }
    
  }
  
  doInitialScreen(3);                       // Paint initial screen (3 seconds)
  
  /*                                                    Initialize libraries */
  Wire.begin();                             // Initialize Wire library
  sensors.begin();                          // Initialize Temp sensors

  /*                                                 Start GPS communication */
  
  gps.begin(GPSbaud);                       // Set up GPS to communicate over serial
  gps.setUBXNav();                          // Enable UBX navigation messages from GPS
  
}
