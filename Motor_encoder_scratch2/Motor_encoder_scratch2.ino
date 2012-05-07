// Luke Miller May 2012
/* See notes at the bottom of the sketch about the changes that
need to be made to the SoftwareSerial, PololuQik, and PololuWheelEncoders
libraries in order to make this sketch work.

The PololuQik library is used to drive the qik 2s9v1 serial 
motor controller. It is available here:
https://github.com/kevin-pololu/qik-arduino

The PololuWheelEncoders library is available at
http://www.pololu.com/docs/0J20/2.a
Unzip this folder and go to /libpololu-avr/src
to find the PololuWheelEncoders folder. Copy this folder
to Arduino-1.0/libraries/, and follow the instructions 
at the bottom of this sketch to modify the .cpp and .h files
for use in this sketch.

*/
#include <mySoftwareSerial.h>
#include <PololuQik.h>
#include <myPololuWheelEncoders.h>
#include <Wire.h>
#include <RTClib.h>

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

PololuWheelEncoders encoder;

RTC_DS1307 RTC;

long Total = 0;  // Total turns during this actuation
float TotalTurns = 0; // Total turns overall (i.e. current position)
int secs = 0; // Keep track of previous seconds value in main loop


void setup() {
  
  Wire.begin();
  RTC.begin();
  // Reset the real time clock to current computer time
  RTC.adjust(DateTime(__DATE__, __TIME__));

  // Initialize qik 2s9v1 serial motor controller
  // The value in parentheses is the serial comm speed
  // for the 2s9v1 controller
  qik.init(38400);
  
  //  encoder.init(); 255 refers to non-existing pins
  // Requires two pins per encoder, here on pins 2,3
  // Take the motor's encoder lines A and B (yellow and white on my motor)
  // and connect them to pins 2 and 3 on the Arduino.
  // Additionally, supply the encoder +5V from the Arduino's 5V line
  encoder.init(2,3,255,255);

  Serial.begin(38400);
  delay(1000);
  // Query real time clock for current time
  DateTime now = RTC.now();
  // Print current time to serial monitor
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.print("Test motor encoder...");
  Serial.println();
}
// ************Main Loop*********************************
void loop() {
  
  // Get current time
  DateTime now = RTC.now();
  // Check if current seconds value is larger than secs value
  
  if (now.second() > secs) {
    Serial.print("Current seconds: ");
    Serial.println(now.second());
    // Update secs value
    secs = now.second();
  }
  
  // If the current time falls on 10-second interval, run the motor
  if ( now.second() % 10 == 0) 
  {
    // Turn motor 0 on, forward
    qik.setM0Speed(40);
    while (Total < 3200) {
      Total = Total + encoder.getCountsAndResetM1();
      if (Total >= 3200) {
        qik.setM0Speed(0);
      }
    }
    // Calculate the new total number of turns made
    TotalTurns = TotalTurns + (Total / 3200);
    Serial.print("Total Turns: ");
    Serial.println(TotalTurns);
    // Reset Total back to 0
    Total = 0;
    delay(1000);
  }
}
//*************** End of Main Loop ***************************

/* ******************************************************************
// Notes about setup
/* 
Both the SoftwareSerial library (found in Arduino-1.0/libraries) and
the PololuWheelEncoders library need to be modified to work together.
The basic concept is explained nicely here:
http://baldwisdom.com/we-interrupt-our-regular-programming/

SoftwareSerial and PololuWheelEncoders clash with each other in their 
stock forms because they both try to set up pin change interrupts on 
all three ports of the Arduino. To make these two libraries play nice
together, you need to modify both to use separate ports. For both 
libraries, I made a copy of the original library folder, renamed it,
and placed the new folder in Arduino/libraries/
Inside each folder, I changed the name of the .cpp and .h files to add
the 'my' to the start of each to differentiate them from the original
libraries. 

There are no changes made to the code inside mySoftwareSerial.h.
 
Inside mySoftwareSerial.cpp, two sets of changes need to be made.
Near the top of the .cpp file (line 44), change the line
#include "SoftwareSerial.h" 

to read
#include "mySoftwareSerial.h"

Further down in the .cpp file, make the 2nd set of changes. Comment 
out the lines:

/*
#if defined(PCINT1_vect)
ISR(PCINT1_vect)
{
  SoftwareSerial::handle_interrupt();
}
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect)
{
  SoftwareSerial::handle_interrupt();
}
#endif

#if defined(PCINT3_vect)
ISR(PCINT3_vect)
{
  SoftwareSerial::handle_interrupt();
}
#endif
*/

/*
so that mySoftwareSerial doesn't interfere with other routines trying to
use pin change interrupts on ports C and D. mySoftwareSerial retains use 
of pin change interrupts on port B (digital pins 8-13).
This leaves ports C and D available for routines like the PololuWheelEncoders.
See http://baldwisdom.com/we-interrupt-our-regular-programming/

The myPololuWheelEncoders.h file does not need to be modified (except for 
having its name changed).

In the PololuWheelEncoders.cpp file, near the top (line 29), the line
#include "PololuWheelEncoders.h"

should be changed to read
#include "myPololuWheelEncoders.h"

Further down in the code of the file, the line
ISR(PCINT0_vect) 
should be changed to read ISR(PNINT2_vect), and the two lines

ISR(PCINT1_vect,ISR_ALIASOF(PCINT0_vect));
ISR(PCINT2_vect,ISR_ALIASOF(PCINT0_vect));

below that should be commented out. This will make it so that the encoder
functions only work on digital lines 0-7 (Arduino PortB).

Finally, one change needs to be made the PololuQik.h file in the PololuQik
library. In that file, change the line
#include "../SoftwareSerial/SoftwareSerial.h"
to read:
#include "../mySoftwareSerial/mySoftwareSerial.h"

You could rename the PololuQik library files to myPololuQik if you want to,
but I don't bother.

Once these changes have been made, make sure both of the modified libraries
are in the Arduino-1.0/libraries folder. You must then remove the 
SoftwareSerial library folder from the libraries/ directory, since its 
presence seems to interfere with compiling this sketch. Then restart the 
Arduino program so that it finds the newly modified libraries. 
*/

