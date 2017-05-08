# LCONFIG

Headers and utilities for laboratory measurements and machine control with the 
[LabJack T7](https://labjack.com/products)

By Chris Martin [crm28@psu.edu](mailto:crm28@psu.edu)

Version 2.02

## About
In the summer of 2016, I realized I needed to be able to adjust my data 
acquisition parameters day-by-day as I was tweaking and optimizing an 
experiment.  I love my LabJack, and the folks at LabJack do a great job of documenting their intuitive interface.  HOWEVER, most of us don't memorize dozens of modbus registers, and weeks after an experiment it's often difficult to remember which data file was taken with which DAQ parameters.

This little **L**aboratory **CONFIG**uration system reads and writes configuration and data files, connects to and configures the T7, and automates the jobs I've done most.  As of version 2.02, that includes analog input streaming and analog output simulating a function generator.

## Getting Started
If you just want to be able to take some quick data, then `drun` and `dburst` will probably work.  
```
    $ make drun
    $ make dburst
    $ ./drun -h   # prints help
    $ ./dburst -h # prints help
```
They load a configuration file, open the appropriate device connections, initiate the data transfer, and write data files automatically.

For writing your own executables, an overview of the LCONFIG system is laid out in LCONFIG_README.  It isn't an exhaustive document, but it's a great way to get started.  The absolute authoritative documentation for LCONFIG is the comments in the prototype section of the header itself.  It is always updated with each version change.

Once you produce your data, there's a good chance that you'll want to analyze it.  If you're a Python user, the lconfig.py module will get you up and running quickly.  Its documentation is inline.

## Anatomy of an LCONFIG configuration file
A configuration file has MANY optional contents, but all configuration files will have a few things in common.  ALL configuration directives have the same format; a parameter followed by a value separated by whitespace.
```
# Configuration files can contain comments
# The first directive always has to be a conneciton type. The connection
# directive tells LCONFIG what interface to use to reach the T7.  It 
# also tells LCONFIG to start configuring a new device.  This allows a
# single configuration file to specify connections to multiple devices.
connection eth

# If the connection is over tcp/ip, then the IP address is used to 
# specify which device is being configured.  If the connection is over
# USB, then the ip address is treated like any other parameter, and will
# be written to the device.  
ip 192.168.10.10

# We can also set some global acquisition options
samplehz 100
nsample 32
settleus 100

# Just like the connection parameter indicates that we've started new
# device connection, aichannel indicates that we're talking about a new
# analog input channel.  This indicates the physical port from which
# analog data should be collected.  MUX80 channel numbers are not 
# supported in version 2.02.
aichannel 0

# To put the channel in differential mode, the negative can be specified
# to the neighboring odd channel number.
ainegative 1

# Now that we specify aichannel 2, all following analog input directives
# will be applied to this analog input and not the last one. Oh, by the 
# way, LCONFIG is case insensitive.
AIchAnnEl 2
ainegative 3
AIRANGE 0.1

##
Configuration files end when the file ends or when the load_config() 
function encounters ##.  This text isn't commented because
it will never be read by load_config()
```

## Anatomy of a data file
Data files are written with the device configuration that was used to collect the data embedded in the file's header.  Because ## ends the configuration section, `load_config()` can open a data file just as easily as it can a configuration file.  That means an experiment can be run to repeat prior data simply by specifying the prior data file as the configuration file.

Below are the first 26 lines of a data file.  The configuration section is terminated by the ## flag, and the date and time at which the measurement was started is recorded behind a #: prefix. 

From there, each line represents a scan of all the configured channels.  Data are separated by whitespace.  Time values are not listed since they can be reconstructed from the SAMPLEHZ parameter.
```
# Configuration automatically generated by WRITE_CONFIG()
connection eth
ip 192.168.0.11
samplehz 100.000000
settleus 1.000000
nsample 64

# Analog Inputs
aichannel 0
ainegative 199
airange 10.000000
airesolution 0

aichannel 2
ainegative 3
airange 0.100000
airesolution 0

# Analog Outputs

## End Configuration ##
#: Wed Apr 19 16:25:50 2017
3.988376e-01 -2.633701e-04
4.060992e-01 2.164717e-04
4.023106e-01 -3.075610e-04
4.019948e-01 -6.766737e-05
...
```

## Table of Parameters
These are the parameters recognized by LCONFIG.  The valid values list the values or classes of values that each parameter can legally be assigned.  The scope indicates where those values are applied.  Global scope parameters are always overwritten if they are repeated, but AI and AO parameters are written to which ever channel is active, and there may be many.

| Parameter   | Valid Values                            | Scope        | Description
|:------------|:---------------------------------------:|:------------:|:--|
| connection  | eth,usb,any                             | Global       | Starts configuration of a new device and specifies over what interface that device should be reached.
| serial      | integer                                 | Global       | Specifies a device by its serial number (ignored over eth when ip is also specified)
| ip          | XXX.XXX.XXX.XXX                         | Global       | Over an ETH connection, specifies the devices to configure, but over a USB connection, IP is used to set the devices IP address.
| gateway     | XXX.XXX.XXX.XXX                         | Global       | Used to set the T7's default TCP/IP gateway
| subnet      | XXX.XXX.XXX.XXX                         | Global       | Used to set the T7's TCP/IP subnet mask
| samplehz    | floating point                          | Global       | The sample rate per channel in Hz
| settleus    | floating point                          | Global       | The settling time per sample in microseconds. If less than 5, the T7 will choose automatically.
| nsample     | integer                                 | Global       | How many samples per channel should be included in each stream packet?
| aichannel   | integer [0-13]                          | Analog Input | The physical analog input channel number
| ainegative  | [0-13], 199, ground, differential       | Analog Input | The physical channel to use as the negative side of the measurement.  199 and ground both indicate a single-ended measurement.  The T7 requires that negative channels be the odd channel one greater than the even positive counterpart (e.g. 0+ 1- or 8+ 9-).  Specify differential to make that selection automatic.
| airange     | float [.01, .1, 1., 10.]                | Analog Input  | The bipolar (+/-) voltage range for the measurement.  The T7 can accept four values.
| aochannel   | integer [0,1]                           | Analog Output | The physical analog output (DAC) channel to use for pseudo function generator output.
| aosignal    | constant, sine, square, triangle, noise | Analog Output | The type of cyclic signal to generate
| aoamplitude | floating point                          | Analog Output | Voltage amplitude of the AC component of the signal (ignored for constant signal).
| aooffset    | floating point                          | Analog Output | Offset (DC) voltage to add to the oscillating (AC) signal.
| aoduty      | floating point [0-1]                    | Analog Output | The duty cycle of triangle and square waves. This skews the signal to spend more time on one part of the wave than the other.
| aofrequency | floating point                          | Analog Output | Specifies the rate the wave will repeat in Hz.
| meta        | flt/float, int/integer, str/string, stop/end/none | Meta | Starts a meta parameter stanza.  Any unrecognized parameter name after this directive will be treated as a new meta parameter of the specified type.  If stop, end, or none is specified, the meta stanza is ended, and unrecognized parameters will cause errors again.
| flt:param   | floating point                          | Meta | Specifies a single floating point meta parameter.
| int:param   | integer                                 | Meta | Specifies a single integer parameter
| str:param   | string                                  | Meta | Specifies a single string parameter

## Future Improvements
One day, LCONFIG will include automatic digital configuration and a layer for
writing the some of the T7 modbus registers.  As things stand, you have to go
grab the device connection handle and do that yourself.
```c
    int err, handle;
    handle = dconf[0].handle;
    err = LJM_eWriteName(handle, "RegisterName", ..value..);
```

## Known Bugs
No persisting known bugs.
