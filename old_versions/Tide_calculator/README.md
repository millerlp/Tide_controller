Tide_calculator.ino
Luke Miller, June 2012

This Arduino sketch is an example of a stand-alone tide height calculator. It doesn't 
do anything besides calculate a new tide height once per minute and output that
value to the Arduino's serial monitor (set your baud rate to 115200). This can 
be used as the basis for creating more complicated projects that need an 
accurate tide height calculation.

The calculations are only made for a single pre-programmed site, so if you need 
projections for a different NOAA tide site, you need to modify the Amplitude (Amp)
and Kappa arrays, preferrably using the R script files provided in the github 
repository. 

In addition to the basic Arduino, the only other required piece of hardware is a 
real time clock, based on the DS1307 or DS3231 chips, connected to analog pins
4 + 5 for the I2C interface (on traditional Arduinos such as the Uno). The routine
needs a current date and time to correctly make predictions. If used for sites outside
of the Pacific Standard Time zone, you'll also need to modify the value of adjustGMT 
inside the sketch.