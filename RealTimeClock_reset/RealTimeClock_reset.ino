/* ReatTimeClock_reset
  Use this sketch to reset the DS1307 Real Time Clock. 
  To accomplish this, you must power down the Arduino (unplug USB)
  and remove the backup battery from the DS1307. Let it sit for a 
  few seconds to ensure that the clock stops. Reinstall the 
  backup battery and plug the USB cable back into the Arduino.
  Upload this sketch to the Arduino, and the clock should be reset
  to your current computer time. 
*/

#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 RTC;

boolean resetFlag = false; // flag for clock reset

void setup(void)
{
  delay(2000);
  Wire.begin();
  RTC.begin();  
  // If the Real Time Clock has begun to drift, you can reset it by pulling its
  // backup battery, powering down the Arduino, replacing the backup battery
  // and then compiling/uploading this sketch to the Arduino. It will only reset
  // the time if the real time clock is halted due to power loss.
  if (! RTC.isrunning()) {
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
    resetFlag = true; // set the reset flag true
  }

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
  if (resetFlag) {
    Serial.println("Clock was reset.");
  } 
  else {
    Serial.println("Clock was NOT reset.");
    Serial.println("Pull backup battery and try again.");
  }
  
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
  delay(2000);
  

}
  
