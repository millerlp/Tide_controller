/* Tide_controller_v1.3 

  This version is designed to work with a Pololu 37D geared motor
  and encoder, along with a Pololu 2s9v1 serial motor controller. 
  This code still misses encoder counts though, so it is not ideal. 
  
  TODO: implement limit switches
  TODO: switch to PWM motor control
  TODO: Move integer harmonic constants into PROGMEM
  
  Copyright (C) 2012 Luke Miller
  
      This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/.
  
  
  This program is designed to calculate the current tide height
  and control a motor that changes the water level in a tank.

  Written under v1.0 of the Arduino IDE.
  
  The harmonics constants for the tide prediction are taken from 
  the XTide harmonics file. The original harmonics.tcd file is 
  available at 
  http://www.flaterco.com/xtide/files.html
  As with XTide, the predictions generated by this program should
  not be used for navigation, and no accuracy or warranty is given
  or implied for these tide predictions. Hell, the chances are 
  pretty good that the tide predictions generated here are 
  completely wrong.
*/
//********************************************************************************


// Initial setup

//*******************************
// Header files for dealing with external interrupts
// Note that the digitalWriteFast header file needs to be updated to work with
// Arduino1.0. The change is simple, just open digitalWriteFast.h and change 
// the initial line from #include "WProgram.h" to #include "Arduino.h"
// digitalWriteFast is available from 
// http://code.google.com/p/digitalwritefast/
#include "Arduino.h"
#include <digitalWriteFast.h> 
//*******************************
// Header files for using Pololu serial motor controller
#include <mySoftwareSerial.h>
#include <PololuQik.h>

//*******************************
// Header files for talking to real time clock
#include <Wire.h>
#include <RTClib.h>
//----------------------------------------------------------------------------------
// Initialize harmonic constant arrays. These each hold 37 values for
// the tide site that was extracted using the R scripts. If you wish
// to make predictions for a different site, it will be necessary to
// replace the Amp and Kappa values with values for your site. 
// These are available from NOAA's http://tidesandcurrent.noaa.gov site.
// Kappa here is referred to as "Phase" on NOAA's site. The order of the
// constants is shown below in the names. Unfortunately this does not match
// NOAA's order, so you will have to rearrange NOAA's values if you want to 
// put new site values in here.
// The Speed, Equilarg and Nodefactor arrays can all stay the same for any 
// site. 
// All tide predictions are output in Greenwich Mean Time. 

// Selected station:  Monterey, Monterey Harbor, California 
// The 'datum' printed here is the difference between mean sea level and 
// mean lower low water for the NOAA station. These two values can be 
// found for NOAA tide reference stations on the tidesandcurrents.noaa.gov
//  site under the datum page for each station.
const float Datum = 2.8281 ; // units in feet
// Harmonic constant names: J1, K1, K2, L2, M1, M2, M3, M4, M6, M8, N2, 2N2, O1, OO1, P1, Q1, 2Q1, R2, S1, S2, S4, S6, T2, LDA2, MU2, NU2, RHO1, MK3, 2MK3, MN4, MS4, 2SM2, MF, MSF, MM, SA, SSA
// These names match the NOAA names, except LDA2 here is LAM2 on NOAA's site
// Amp scaled by 1000, so divide by 1000 to convert to original float value
const unsigned int Amp[] = {71,1199,121,23,38,1616,0,0,0,0,368,44,753,36,374,134,16,3,33,428,0,0,22,11,41,72,26,0,0,0,0,0,0,0,0,157,90};
// Kappa scaled by 10, so divide by 10 to convert to original float value
const unsigned int Kappa[] = {2334,2198,1720,2202,2259,1811,0,0,0,0,1546,1239,2034,2502,2156,1951,1994,1802,3191,1802,0,0,1678,1807,1146,1611,1966,0,0,0,0,0,0,0,0,2060,2839};
// Speed is unscaled, stored as the original float values
const float Speed[] = {15.58544,15.04107,30.08214,29.52848,14.49669,28.9841,43.47616,57.96821,86.95231,115.9364,28.43973,27.89535,13.94304,16.1391,14.95893,13.39866,12.85429,30.04107,15,30,60,90,29.95893,29.45563,27.96821,28.51258,13.47151,44.02517,42.92714,57.42383,58.9841,31.0159,1.098033,1.015896,0.5443747,0.0410686,0.0821373};
// Equilarg scaled by 100. Divide by 100 to get original value.
const unsigned int Equilarg[4][37] = { 
{17495,1851,21655,15782,23131,19425,29137,2850,22275,5700,4193,24962,17162,5364,34993,1930,22699,17692,18000,0,0,0,308,2959,2660,17891,15628,21276,999,23618,19425,16575,3101,16575,15232,28007,20013},
{27537,1770,21457,34814,15957,27019,22529,18038,9058,77,1609,12198,24892,33361,34919,35482,10072,17765,18000,0,0,0,235,28737,17891,7302,5175,28789,16269,28627,27019,8981,31235,8981,25410,28081,20162},
{2,1486,20897,18274,7278,1035,19553,2071,3106,4142,2753,4470,35316,22123,34943,1033,2751,17739,18000,0,0,0,261,19806,1982,265,34546,2521,585,3788,1035,34965,20404,34965,34283,28057,20115},
{8338,1130,20240,366,32299,11042,16563,22084,33126,8168,3886,32732,9858,10509,34967,2703,31549,17714,18000,0,0,0,286,10865,22064,29219,28036,12172,20954,14929,11042,24958,9325,24958,7155,28033,20067} 
 };

// Nodefactor scaled by 10000. Divide by 10000 to get original float value.
const unsigned int Nodefactor[4][37] = { 
{9491,9602,8878,11647,11232,10169,10257,10343,10519,10698,10169,10169,9349,7887,10000,9349,9349,10000,10000,10000,10000,10000,10000,10169,10169,10169,9349,9765,9931,10343,10169,10169,8592,10169,10577,10000,10000},
{8928,9234,8156,12048,8780,10271,10411,10552,10839,11134,10271,10271,8748,6334,10000,8748,8748,10000,10000,10000,10000,10000,10000,10271,10271,10271,8748,9485,9743,10552,10271,10271,7448,10271,10936,10000,10000},
{8492,8957,7680,10216,13201,10344,10520,10699,11067,11448,10344,10344,8290,5315,10000,8290,8290,10000,10000,10000,10000,10000,10000,10344,10344,10344,8290,9265,9583,10699,10344,10344,6642,10344,11190,10000,10000},
{8278,8824,7472,8780,15575,10377,10571,10768,11173,11594,10377,10377,8068,4868,10000,8068,8068,10000,10000,10000,10000,10000,10000,10377,10377,10377,8068,9156,9501,10768,10377,10377,6271,10377,11307,10000,10000} 
 };

// The currYear array will be used as a reference for which row of the
// Equilarg and Nodefactor arrays we should be pulling values from.
const int currYear[] = {2012,2013,2014,2015};

// Define unixtime values for the start of each year
//                               2012        2013        2014        2015
unsigned long startSecs[] = {1325376000, 1356998400, 1388534400, 1420070400};

// 1st year in the Equilarg/Nodefactor arrays
const unsigned int startYear = 2012;

// Define some variables that will hold float-converted versions of the constants above
float currAmp;
float currSpeed;
float currNodefactor;
float currEquilarg;
float currKappa;
//------------------------------------------------------------------------------------------------
// Real Time Clock setup
RTC_DS1307 RTC;
unsigned int YearIndx = 0;    // Used to index rows in the Equilarg/Nodefactor arrays
float currHours = 0;          // Elapsed hours since start of year
const int adjustGMT = 8;     // Time zone adjustment to get time in GMT. Make sure this is
                             // correct for the local standard time of the tide station. 
                             // No daylight savings time adjustments should be made. 
                             // 8 = Pacific Standard Time (America/Los_Angeles)

int secs = 0; // Keep track of previous seconds value in main loop
int currMinute; // Keep track of current minute value in main loop
//------------------------------------------------------------------------------------------------
// Motor controller setup section
/*
Required connections between Arduino and qik 2s9v1:

      Arduino    qik 2s9v1
           5V -> VCC
          GND -> GND
Digital Pin 8 -> TX pin on 2s9v1 (optional if you don't need talk-back from the unit) 
Digital Pin 9 -> RX pin on 2s9v1
Digital Pin 10 -> RESET
*/
PololuQik2s9v1 qik(8, 9, 10);

//-----------------------------------------------
// External interrupt setup for motor position encoder
// Code adapted from
// http://www.billporter.info/sagar-update-new-motor-controller-and-motors/
#define c_EncoderPinInterrupt 0    // interrupt 0 (digital pin 2 on Ard328)
#define c_EncoderPinA 2            // digital pin 2
#define c_EncoderPinB 4            // digital pin 4 on Ard328
//#define EncoderIsReversed        // uncomment if encoder counts the wrong direction
volatile bool _EncoderBSet;
volatile long _EncoderTicks = 0;
//-----------------------------------------------

long Total = 0;  // Total turns during this actuation
float TotalTurns = 0; // Total turns overall (i.e. current position)
float currPos = 6.0; // Current position, based on limit switch height. Units = ft.
float results = currPos;

// Conversion factor, feet per encoder count
// Divide desired travel (in ft.) by this value
// to calculate the number of encoder counts that
// must occur.
const float countConv = 0.00036979;   // Value for 1.130" diam spool
/*  1.13" diam spool x pi = 3.55" per output shaft revolution
    3.55" per rev / 12" = 0.2958333 ft per output shaft revolution
    I use a 50:1 gear reduction Pololu motor, so 1 output shaft rev
    takes 50 motor armature revolutions. If the encoder interrupt only
    triggers (counts) on rising signals on line A, there will be 16 
    triggers per motor armature revolution or (16 x 50 = 800) counts 
    per output shaft revolution.
    0.295833 ft per rev / 800 counts per rev = 0.00036979 ft per count
    16 counts per revolution = 360/16 = 22.5° resolution on motor 
    armature position, or 0.45° resolution on output shaft position.
*/

float heightDiff;    // Float variable to hold height difference, in ft.                                    
long countVal = 0;     // Store the number of encoder counts needed
                                // to achieve the new height
long counts = 0;       // Store the number of encoder counts that have
                                // gone by so far.

const int motorSpeed = 40; // Specify motor rotation speed (0 to 127) for
                           // qik2s9v1 motor controller
//---------------------------------------------------------------------------


//**************************************************************************
// Welcome to the setup loop
void setup(void)
{  
  Wire.begin();
  RTC.begin();
  //--------------------------------------------------
  //Quadrature encoders
  pinMode(c_EncoderPinA, INPUT);  // sets encoder pin A as input
  digitalWrite(c_EncoderPinA, LOW);  // turn on pullup resistor
  pinMode(c_EncoderPinB, INPUT);  // sets encoder pin B as input
  digitalWrite(c_EncoderPinB, LOW); // turn on pullup resistor
  attachInterrupt(c_EncoderPinInterrupt, HandleInterruptA, RISING);
  //--------------------------------------------------
  // For debugging output to serial monitor
  Serial.begin(115200);
  //************************************
  DateTime now = RTC.now();
  currMinute = now.minute(); // Store current minute value
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  if (now.minute() < 10) {
    Serial.print("0");
    Serial.print(now.minute());
  }
  else if (now.minute() >= 10) {
    Serial.print(now.minute());
  }
  Serial.print(':');
  if (now.second() < 10) {
    Serial.print("0");
    Serial.println(now.second());
  }
  else if (now.second() >= 10) {
    Serial.println(now.second());
  }
  delay(2000);
  
  // Initialize qik 2s9v1 serial motor controller
  // The value in parentheses is the serial comm speed
  // for the 2s9v1 controller. 38400 is the maximum.
  qik.init(38400);

  
  // TODO: Create limit switch routine for initializing tide height value
  //       after a restart. 
  // TODO: Have user select tide height limits outside of which the motor
  //       won't turn any further.

}

//**************************************************************************
// Welcome to the Main loop
void loop(void)
{
  // Get current time, store in object 'now'
  DateTime now = RTC.now();
  
  // If it is the start of a new minute, calculate new tide height and
  // adjust motor position
  if (now.minute() != currMinute) {
    // If now.minute doesn't equal currMinute, a new minute has turned
    // over, so it's time to update the tide height. We only want to do
    // this once per minute. 
    currMinute = now.minute();                   // update currMinute
    
    // Calculate difference between current year and starting year.
    YearIndx = startYear - now.year();
    // Calculate hours since start of current year. Hours = seconds / 3600
    currHours = (now.unixtime() - startSecs[YearIndx]) / float(3600);
    // Shift currHours to Greenwich Mean Time
    currHours = currHours + adjustGMT;
    
    Serial.print("Previous tide ht: ");
    Serial.print(results);
    Serial.println(" ft.:");   
    // *****************Calculate current tide height*************
    results = Datum; // initialize results variable, units of feet.
    for (int harms = 0; harms < 37; harms++) {
      // Many of the constants are stored as unsigned integers to save space. These
      // steps convert them back to their real values.
      currNodefactor = Nodefactor[YearIndx][harms] / float(10000);
      currAmp = Amp[harms] / float(1000);
      currEquilarg = Equilarg[YearIndx][harms] / float(100);
      currKappa = Kappa[harms] / float(10);
      currSpeed = Speed[harms]; // Speed was not scaled to integer
      
    // Calculate each component of the overall tide equation 
    // The currHours value is assumed to be in hours from the start of the
    // year, in the Greenwich Mean Time zone, not the local time zone.
    // There is no daylight savings time adjustment here.  
      results = results + (currNodefactor * currAmp * cos( (currSpeed * currHours + currEquilarg - currKappa) * DEG_TO_RAD));
    }
    //******************End of Tide Height calculation*************
     
     // Calculate height difference between currPos and
     // new tide height. Value may be positive or negative depending on
     // direction of the tide. 
     heightDiff = currPos - results;       // Units of feet.
     // Convert heightDiff into number of encoder counts that must pass
     // to achieve the new height. The result is cast as an unsigned 
     // long integer.
     countVal = (long)(heightDiff / countConv);
     
     
    //********************************
    // For debugging
    // Print current time to serial monitor
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    if (now.minute() < 10) {
      Serial.print("0");
      Serial.print(now.minute());
    }
    else if (now.minute() >= 10) {
      Serial.print(now.minute());
    }
    Serial.print(':');
    if (now.second() < 10) {
      Serial.print("0");
      Serial.println(now.second());
    }
    else if (now.second() >= 10) {
      Serial.println(now.second());
    }
     Serial.print("Height diff: ");
     Serial.println(heightDiff);
     Serial.print("countVal calc: ");
     Serial.println(countVal);
     Serial.print("Target height: ");
     Serial.println(results);
     //*********************************
     
     _EncoderTicks = 0;  // Reset _EncoderTicks value before moving motor
     // ************** Lower drain height to lower tide level ***********
     // TODO: check if drain is above or below physical height limits and
     // skip this section if so. 
     if (heightDiff > 0) // Positive value means drain is higher than target value
     {
       Serial.println("Turning motor forward to lower drain");
//       counts = 0;
//       qik.setM0Speed(20);  // turn motor on in forward direction

       /*  The nested while loops here are necessary to compensate for a weird
           condition where the inner while loop will occasionally quit before
           _EncoderTicks actually hits the target value. If that happens, the 
           outer while loop runs again to finish the last few missing turns.
      */
       while(_EncoderTicks <= countVal) {
         //  Turn motor 0 on forward
         qik.setM0Speed(motorSpeed);
         while (1) {
           if (_EncoderTicks >= countVal) {
             qik.setM0Speed(0);  // shut off motor
             break;  // break out of while loop
           }
         }
       }
       Serial.print("Encoder Ticks: ");
       Serial.print(_EncoderTicks);
       Serial.print(", Target: ");
       Serial.println(countVal);
       
       // Calculate any overshoot of the desired position
       counts = _EncoderTicks - countVal;
       // Subtract the overshoot to 'results' to save the actual currPos
       currPos = results - (counts * countConv);
       
//       while (counts < countVal) {
//         counts = counts + encoder.getCountsAndResetM1();
//       }
//       qik.setM0Speed(0);  // Turn motor off
//       currPos = results;  // Update current position (units of feet).
     }
     // **************** Raise drain height to raise tide level ************
     else if (heightDiff < 0) // Negative value means drain is lower than target value
     {
       Serial.println("Turning motor reverse to raise drain");
       countVal = countVal * -1;  // Switch countVal sign
       while(_EncoderTicks > countVal) {
         //  Turn motor 0 on reverse (reverse should be negative value)
         qik.setM0Speed((motorSpeed * -1));
         while (1) {
           if (_EncoderTicks <= countVal) {
             qik.setM0Speed(0);  // shut off motor
             break;  // break out of while loop
           }
         }
       }
       // Calculate any overshoot of the desired position
       // _EncoderTicks should be a negative value
       counts = _EncoderTicks - countVal;
       
       Serial.print("Encoder Ticks: ");
       Serial.print(_EncoderTicks);
       Serial.print(", Target: ");
       Serial.println(countVal);
       
       // Switch sign of counts
       counts = counts * -1;
       // Add the overshoot to 'results' to save the actual currPos
       currPos = results + (counts * countConv);
       
     }  // end of else if statement
     Serial.print("Current Position: ");
     Serial.println(currPos);
     
//       countVal = countVal * -1; // Change countVal direction
//       counts = 0;
//       qik.setM0Speed(-40);  // turn motor on in reverse direction
//       while (counts > countVal) {
//         counts = counts + encoder.getCountsAndResetM1();
//       }
//       qik.setM0Speed(0);  // turn motor off
//       currPos = results;  // Update current position (units of feet).
//     }
      
//    Serial.print("Tide: ");
//    Serial.print(results, 3);
//    Serial.println(" ft.");
//
//    Serial.print("Counts to turn: ");
//    Serial.print(countVal);
    // end of debugging stuff
    //********************************   
  }    // end of if (now.minute() != currMinute) statement

 // TODO: include limit switch checking routines
 
} // end of main loop

//-----------------------------------------------------------------
//  Welcome to the interrupt service routine for counting
//  motor encoder triggers (including rotation direction).
//  Interrupt service routine should trigger each time
//  digital pin 2 (interrupt 0) get a RISING signal.
void HandleInterruptA()
{
  //Test transition of pin B, we already know pin A just went high
  _EncoderBSet = digitalReadFast(c_EncoderPinB); // read pin B

#ifdef EncoderIsReversed
    _EncoderTicks += _EncoderBSet ? -1 : +1;
#else
  _EncoderTicks -= _EncoderBSet ? -1 : +1;    
#endif
}
