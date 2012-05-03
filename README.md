Tide_controller
===============

Arduino code and associated files for building an aquarium tide height controller.

The R script files found here were originally used for parsing XTide's harmonics file
to extract the equilibrium conditions, lunar node factors, speeds, and harmonic 
constituents for a chosen site. Those scripts culminate by producing output code that
can be copied and pasted into the Arduino .ino program so that tide predictions can be
made for the chosen site. In lieu of doing all of that in the future, it would also be 
possible to extract the relevant Amplitude and kappa (a.k.a Phase) values for a new
NOAA tide site. There are generally 37 such values, and they can be put into the 
Arduino code, replacing the current Amp and Kappa values. Note that NOAA's ordering of
harmonic constituents is different than that used in the Arduino programs, so you will 
need to re-order the NOAA values when substituting them into the Arduino program. 