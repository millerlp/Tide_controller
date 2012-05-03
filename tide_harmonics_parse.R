# Filename: tide_harmonics_parse.R
# 
# Author: Luke Miller  May 1, 2012
###############################################################################
# This is essentially a one-time use script to call the read_harmonicsfile.R 
# functions and parse the tidal harmonics file from XTide. The resulting data 
# for the ~666 reference stations can be stored in a Rdata file 
# The harmonics file must be a text format, not the binary 
# tcd format that is generally distributed with XTide. To generate this text 
# version of the harmonics.tcd file, you must use the command line tool 
# restore_tide_db found in the tcd-utils package distributed on the XTide site.
# This will require a Linux or Mac (not Windows) machine.
# See http://www.flaterco.com/xtide/files.html for downloads.


# Load the functions from read_harmonicsfile.R
source('./read_harmonicsfile.R')

# Create a connection to the harmonics text file for reading.
#fid = file('W:/xtide/harmonics-dwf-20120302-free.txt', open = 'r')
fid = file(file.choose(), open = 'r')

# Call the read_harmonicsfile function (found in read_harmonicsfile.R) to parse
# the harmonics file into a usable format.
harms = read_harmonicsfile(fid)

# Close file connection when finished.
close(fid)

# Save the results to a Rdata file, since there's no need to re-parse the 
# harmonics file once you've done it once. 
save(harms, file = 'Harmonics_20120302.Rdata')

# To extract site-specific data for a single tide station, see the script
# tide_extract_1site_harmonics.R