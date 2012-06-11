Tide_controller
===============

Arduino code and associated files for building an aquarium tide height controller.

Installation:

To simply calculate real-time tide height for a site, install the following:

Copy the RTClib folder into Arduino/libraries/

Copy the folder with the library for your site (i.e. TideMontereyHarborlib) into Arduino/libraries/

Copy the Tide_calculator or Tide_controller folder (most recent version) into your Arduino folder where 
your other sketches are normally stored (not in the libraries folder).

Open the Tide_calculator sketch in the Arduino IDE, and make sure the correct tide site
library is referenced with a line like:

\#include "TideMontereyHarborlib.h"

Upload the program to the Arduino. Open the serial monitor to view the output. 

------------------------------
If the real time clock attached to the Arduino is not yet set, you need to set it 
using the RealTimeClock_reset sketch found in the folder of the same name. Upload that
to the Arduino, and it should automatically reset the real time clock. Make sure your
computer's clock is set to local Standard Time, not Daylight Savings time (which runs Mar-Nov
in most places). The tide prediction routine relies on the time being set to local 
standard time for your site, otherwise you won't get the current tide height out. After running
the RealTimeClock_reset sketch, before unplugging the Arduino, immediately upload a different 
sketch to the Arduino so that the clock doesn't try to reset itself repeatedly when the Arduino
restarts.

-------------------------------
If there is no folder containing a tide prediction library for your desired site, it
will be necessary to generate a library using the R scripts found in the 
Generate_new_site_libraries directory. The harmonic data for NOAA sites are all in
the Harmonics_20120302.Rdata file. With these data, you can generate a new library
by running the R script tide_harmonics_library_generator.R. Inside that file, you must
enter a name for the site you're interested in on the line
stationID = 'Monterey Harbor'
Change the value inside the quote marks to match your site, save the file, and run the
script in R. It will create a new directory with the necessary library files inside that
can be copied into the Arduino/libraries/ folder. Find the name for your site by looking 
through the XTide website http://www.flaterco.com/xtide/locations.html 
XTide only produces harmonics for 635 reference tide stations (all others are approximated 
from these tide stations), so you need to find the nearest station listed as "Ref" on that 
page, rather than "Sub" stations.

The other R scripts in the Generate_new_site_libraries directory could be used to make an
updated Rdata file when XTide updates the original harmonics database and after you
convert it to a text file using the libtcd library from the XTide site (see the notes inside
the R scripts). 