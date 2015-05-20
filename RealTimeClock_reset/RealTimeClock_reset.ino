/* ReatTimeClock_reset
  Use this sketch to reset the DS1307 or DS3231 Real Time Clock. 
  It will grab the computer's current time during compilation and 
  send that value to the real time clock. 
  
  Afterwards, immediately upload a different sketch to the Arduino
  so that it doesn't try to constantly reset the clock when it 
  powers up the next time. If it does reset, the Arduino will reset 
  the clock with the old compile time stamp, which will be out of
  date.
  
  This version includes code to talk to a Sparkfun 7-segment LED
  4 digit display connected on pins 7 + 4.
  https://www.sparkfun.com/products/11442
  The sketch should run fine without the display if you don't have
  it. 
*/

#include <Wire.h>
#include <SPI.h> // Required for RTClib to compile properly
#include <RTClib.h> // From https://github.com/MrAlvin/RTClib
#include <SoftwareSerial.h>

RTC_DS1307 RTC;

boolean resetFlag = false; // flag for clock reset

//****** 7 Segment LED display setup
#define txPin 7
#define rxPin 4 // not used, but must be defined
SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);


void setup(void)
{
  delay(2000);
  Wire.begin();
  RTC.begin();  
  RTC.adjust(DateTime(__DATE__, __TIME__));


  DateTime now = RTC.now();
  Serial.begin(9600);
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
  Serial.println(now.second(), DEC);  
  sevenSegDisplayTime(now); // print date + time to 7-seg display
  delay(2000);
}

void loop(void)
{
  DateTime now = RTC.now();
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
  Serial.println(now.second(), DEC);
  sevenSegDisplayTime(now); // print date + time to 7-seg display
  delay(2000);
  

}
  
//********************************************************
// sevenSegDisplayTime function
// A function to show the year, month, day and time on 
// the Sparkfun serial 7-segment LED display. 
// https://www.sparkfun.com/products/11442 

void sevenSegDisplayTime(DateTime now){
	mySerial.write(0x76); // clear display
	mySerial.write(now.year()); //write year
	delay(1000);
	mySerial.write(0x76); // clear display
	mySerial.write(now.month()); // write month
	delay(1000);
	mySerial.write(0x76); // clear display
	if (now.day() < 10) {
		mySerial.write(0x79); // send Move Cursor Command
		mySerial.write(0x03); // position cursor at 4th digit
		mySerial.write(now.day());
	} else {
		mySerial.write(0x79); // send Move Cursor Command
		mySerial.write(0x02); // position cursor at 3rd digit
		mySerial.write(now.day()); // write day
	}
	delay(1000);
	mySerial.write(0x76); // clear display
	mySerial.write(0x77); // Decimal Control command byte
	mySerial.write(0x08); // turn on colon
	if (now.hour() < 10) {
		mySerial.write(0x79); // send Move Cursor Command
		mySerial.write(0x01); // position cursor at 2nd digit
		mySerial.write(now.hour()); // print hour
	} else {
		mySerial.write(now.hour()); // print hour
	}
	mySerial.write(now.hour()); // display hour
	if (now.minute() < 10) { // for minute values less than 10
		mySerial.write(0x00); // print a zero
		mySerial.write(now.minute()); // print minute value
	} else {
		mySerial.write(now.minute()); // print minute value
	}
	delay(2000);
}