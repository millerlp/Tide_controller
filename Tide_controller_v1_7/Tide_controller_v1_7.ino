/* Tide_controller_v1.7
  Copyright (C) 2012 Luke Miller
 This version is set up to work on a lead-screw driven rack that has
 a limited travel range. There should be a limit switch at each end 
 of the rack's travel, and the distance between the values for 
 upperPos and lowerPos must be equal to the distance between those 
 limit switches. 
 
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
 
 Written under v1.0.1 of the Arduino IDE.
 
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
// Header files for talking to real time clock
#include <Wire.h>
#include <SPI.h>  // Required for RTClib to compile properly
#include <RTClib.h> // From https://github.com/MrAlvin/RTClib
// Real Time Clock setup
RTC_DS3231 RTC;      
// RTC_DS1307 RTC;  // Uncomment this version if you use the older DS1307 clock

// Tide calculation library setup
//#include "TideMontereyHarborlib.h"
#include "TideSanDiegoSanDiegoBaylib.h"
TideCalc myTideCalc;  // Create TideCalc object called myTideCalc 

int currMinute; // Keep track of current minute value in main loop
//---------------------------------------------------------------------------------------
/*  Stepper motor notes
 This uses a Big Easy Driver to control a unipolar stepper motor. This
 takes two input wires: a direction wire and step wire. The direction is
 set by pulling the direction pin high or low. A single step is taken by 
 pulling the step pin high. 
 By default, when you leave MS1, MS2, and MS3 high on the Big Easy Driver, the 
 driver defaults to 1/16 microstep mode. If a full step is 1.8° (200 steps
 per revolution), then microstep mode moves 1/16 of that, 
 (1.8° * 1/16 = 0.1125° per step), or 3200 microsteps per full revolution 
 of the motor. 
 
 */
const int stepperDir = 8;  // define stepper direction pin. Connect to 
// Big Easy Driver DIR pin
const int stepperStep = 9; // define stepper step pin. Connect to 
// Big Easy Driver STEP pin.

//*******************************

//-----------------------------------------------------------------------------
float upperPos = 5.3; // Upper limit, located at upperLimitSwitch. Units = ft.
float lowerPos = 2.4; // Lower limit, located at lowerLimitSwitch. Units = ft.
float currPos;  // Current position, within limit switch range.    Units = ft.
float results;  // results holds the output from the tide calc.    Units = ft.
// The value for upperPos is taken to be the "home" position, so when ever the
// upperLimitSwitch is activated, the motor is at exactly the value of upperPos.
// In contrast, the value of lowerPos can be less precise, it is just close to 
// the position of where the lowerLimitSwitch will activate, and is used as a 
// backup check to make sure the motor doesn't exceed its travel limits. The 
// lowerLimitSwitch is the main determinant of when the motor stops downward
// travel. This is mainly to accomodate the use of magnetic switches where the
// precise position of activation is hard to determine without lots of trial 
// and error.

//-----------------------------------------------------------------------------
// Conversion factor, feet per motor step
// Divide desired travel (in ft.) by this value
// to calculate the number of steps that
// must occur.
const float stepConv = 0.000002604;   // Value for 10 tpi lead screw
/*  10 tooth-per-inch lead screw = 0.1 inches per revolution
 0.1 inches per rev / 12" = 0.008333 ft per revolution
 0.008333 ft per rev / 3200 steps per rev = 0.000002604 ft per step
 Assumes a 200 step per rev stepper motor being controlled in 1/16th
 microstepping mode ( = 3200 steps per revolution).
 We're using feet here because the tide prediction routine outputs
 tide height in units of feet. 
 */

float heightDiff;    // Float variable to hold height difference, in ft.                                    
long stepVal = 0;     // Store the number of steps needed
// to achieve the new height


//---------------------------------------------------------------------------
// Define the digital pin numbers for the limit switches. These will be 
// wired to one lead from a magentic reed switch. The 2nd lead from each reed 
// switch will be wired to ground on the Arduino.
const int lowerLimitSwitch = 10;
const int upperLimitSwitch = 11;
// Define digital pin number for the homeButton
// Pressing this button will run the motor until it hits the upper limit switch
const int homeButton = 12; 

//**************************************************************************
// Welcome to the setup loop
void setup(void)
{  
  Wire.begin();
  RTC.begin();
  //--------------------------------------------------
  pinMode(stepperDir, OUTPUT);   // direction pin for Big Easy Driver
  pinMode(stepperStep, OUTPUT);  // step pin for Big Easy driver. One step per rise.
  digitalWrite(stepperDir, LOW);
  digitalWrite(stepperStep, LOW);

  // Set up switches and input button as inputs. 
  pinMode(lowerLimitSwitch, INPUT);
  digitalWrite(lowerLimitSwitch, HIGH);  // turn on internal pull-up resistor
  pinMode(upperLimitSwitch, INPUT);
  digitalWrite(upperLimitSwitch, HIGH);  // turn on internal pull-up resistor
  pinMode(homeButton, INPUT);
  digitalWrite(homeButton, HIGH);        // turn on internal pull-up resistor
  // When using the internal pull-up resistors for the switches above, the
  // default state for the input pin is HIGH (+5V), and goes LOW (0V) when
  // the switch/button connects to ground. Thus, a LOW value indicates that the
  // button or switch has been activated. 


  // For debugging output to serial monitor
  Serial.begin(115200);
  //************************************
  DateTime now = RTC.now();
  currMinute = now.minute(); // Store current minute value
  printTime(now);  // Call printTime function to print date/time to serial
  Serial.println("Calculating tides for: ");
  Serial.println(myTideCalc.returnStationID());
  delay(4000);

  //************************************
  // Spin motor to position slide at the home position (at upperLimitSwitch)
  // The while loop will continue until the upperLimitSwitch is activated 
  // (i.e. driven LOW). 
  Serial.println("Returning to upper limit");
  while (digitalRead(upperLimitSwitch) != LOW) {
    // Set motor direction, HIGH = counterclockwise
    digitalWrite(stepperDir, HIGH);
    // Move stepper a single step
    digitalWrite(stepperStep, HIGH);
    delayMicroseconds(100);
    digitalWrite(stepperStep, LOW);
  }
  currPos = upperPos; // currPos should now equal upperPos
  Serial.print("Current position: ");
  Serial.print(currPos,2);
  Serial.println(" ft.");
  delay(2000);
}  // End of setup loop.

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

    
    Serial.println();
    printTime(now);
    Serial.print("Previous tide ht: ");
    Serial.print(results,3);
    Serial.println(" ft.");   
    
    // Calculate new tide height based on current time
    results = myTideCalc.currentTide(now);
    
    // Calculate height difference between currPos and
    // new tide height. Value may be positive or negative depending on
    // direction of the tide. Positive values mean the water level
    // needs to be raised to meet the new predicted tide level.
    heightDiff = results - currPos;       // Units of feet.
    // Convert heightDiff into number of steps that must pass
    // to achieve the new height. The result is cast as an unsigned 
    // long integer.
    stepVal = (long)(heightDiff / stepConv);
    stepVal = abs(stepVal);  // remove negative sign if present


    //********************************
    // For debugging
    Serial.print("Height diff: ");
    Serial.print(heightDiff, 3);
    Serial.println(" ft.");
    Serial.print("stepVal calc: ");
    Serial.println(stepVal);
    Serial.print("Target height: ");
    Serial.print(results, 3);
    Serial.println(" ft.");
    Serial.println(); // blank line
    //******************************************************************
    // Motor movement section
    // ************** Lower water level to new position ****************
    // If the heightDiff is negative, AND the target level is less than 
    // the upperPos limit, AND the target level is greater than the 
    // lowerPos limit AND the lowerLimitSwitch 
    // hasn't been activated, then the motor can be moved downward. 
    if ( (heightDiff < 0) & (results < upperPos) & 
      (results > lowerPos) & (digitalRead(lowerLimitSwitch) == HIGH) )
    {
      // Set motor direction to move downward
      digitalWrite(stepperDir, LOW);       
      // Run motor the desired number of steps
      for (int steps = 0; steps < stepVal; steps++) {
        digitalWrite(stepperStep, HIGH);
        delayMicroseconds(100);
        digitalWrite(stepperStep, LOW);
        // check lowerLimitSwitch each step, quit if it is activated
        if (digitalRead(lowerLimitSwitch) == LOW) {
          // Calculate how far the motor managed to turn before
          // hitting the lower limit switch, record that as the new
          // currPos value.
          currPos = currPos - (steps * stepConv);
          Serial.println("Hit lower limit switch");
          break;  // break out of for loop
        }
      }
      if (digitalRead(lowerLimitSwitch) == HIGH) {
        // If the lower limit wasn't reached, then the currPos should
        // be equal to the new water level stored in 'results'.
        currPos = results;
      }       
    }
    // ************Raise water level to new position******************
    // If the heightDiff is positive, AND the target level is greater 
    // than the lowerPos limit, AND the target level is less than the 
    // upperPos limit (plus a 0.025ft buffer), AND the upperLimitSwitch 
    // hasn't been activated, then the motor can be moved.
    else if ( (heightDiff > 0) & (results > lowerPos) & 
      (results < (upperPos + 0.025)) & (digitalRead(upperLimitSwitch) == HIGH) )
    {
      // Set motor direction in reverse
      digitalWrite(stepperDir, HIGH);
      // Run motor the desired number of steps
      for (int steps = 0; steps < stepVal; steps++) {
        digitalWrite(stepperStep, HIGH);
        delayMicroseconds(100);
        digitalWrite(stepperStep, LOW);
        // check upperLimitSwitch each step, quit if it is activated
        if (digitalRead(upperLimitSwitch) == LOW) {
          // Since the upper limit switch is the "home" position
          // we assume that the currPos = upperPos when the
          // upperLimitSwitch is activated.
          currPos = upperPos;
          Serial.println("Hit upper limit switch");
          break;  // break out of for loop
        }
      }
      if (digitalRead(upperLimitSwitch) == HIGH) {
        // If the upper limit wasn't reached, then currPos should
        // be equal to the new water level stored in 'results'.
        currPos = results;
      }     
    }
    // End of motor movement section. If the current tide ht is outside
    // either of the travel limits, then there should be no motor 
    // movement, and the currPos value should not be changed. 
    //*******************************************************************
    if (digitalRead(upperLimitSwitch) == LOW) {
      Serial.println("At upper limit switch, no movement");
      Serial.println();
    }
    if (digitalRead(lowerLimitSwitch) == LOW) {
      Serial.println("At lower limit switch, no movement");
      Serial.println();
    }
  }    // End of if (now.minute() != currMinute) statement

  // TODO: implement return-to-home-position routine.

} // End of main loop


//******************************************************
// Function for printing the current date/time to the 
// serial port in a nicely formatted layout.
void printTime(DateTime now) {
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
}
//********************************************************

