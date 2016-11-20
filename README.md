# LAB

Headers and codes for laboratory measurements and machine control with the 
[LabJack T7](https://labjack.com/products)

By Chris Martin [crm28@psu.edu](mailto:crm28@psu.edu)

Version 1.2

## About
In the summer of 2016, I realized I needed to be able to adjust my data 
acquisition parameters day-by-day as I was tweaking and optimizing an 
experiment.  I love my LabJack, but weeks later, pouring over data files, it's 
difficult to remember what data was taken with which parameters.

This is my little Laboratory CONFIGuration system.  It parses text configuration 
files and data files, generates data files, connects to and configures the T7, 
and automates the jobs I've done most.  As of version 1.2, that includes analog 
input streaming and analog output simulating a funciton generator.

## Getting Started
An overview of the LCONFIG system is laid out in LCONFIG_README.  It isn't an 
exhaustive document; it's just a way to get started.  The absolute authoritative
documentation for LCONFIG is the comments in the prototype section of the header
itself.  It is always updated with each version change.

Once you produce your data, there's a good chance that you'll want to analyze 
it.  If you're a Python user, the lconfig.py and loadall.py modules will get
you up and running quickly.  Their documentation is inline.

## Future Improvements
One day, LCONFIG will include automatic digital configuration and a layer for
writing the some of the T7 modbus registers.  As things stand, you have to go
grab the device connection handle and do that yourself.

<<<<<<< HEAD
    int err, handle;
    handle = dconf[0].handle;
    err = LJM_eWriteName(handle, "RegisterName", ..value..);
=======
  int err, handle;
  handle = dconf[0].handle;
  err = LJM_eWriteName(handle, "RegisterName", ..value..);
>>>>>>> e8b43052956511746b855510501f47edae23b0dc


## Known Bugs
As of Version 1.2, the analog output streaming only behaves properly when a 
single channel is configured at a time.  The discussion is 
[here](https://labjack.com/forums/t7/clarity-t7-output-streaming).