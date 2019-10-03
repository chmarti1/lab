# LCONFIG README

Headers and utilities for laboratory measurements and machine control with the [LabJack T4 and T7](https://labjack.com/products)

By Christopher R. Martin<br>
Assistant Professor of Mechanical Engineering<br>
Penn State University<br>
 [crm28@psu.edu](mailto:crm28@psu.edu)

Version 4.00

### Foreword

This README is a preliminary introduciton intended to help users decide if LConfig is right for them and to get started.  The detailed documentation is contained in the `docs` folder and is linked below.

For detailed documentation on the LCONFIG package, use these links:

- [Documentation](docs/documentation.md)
- [The LConfig API](docs/api.md)
- [Reference](docs/reference.md)

---

### Contents 

- [About](#about)
- [License](#license)
- [Getting started](#start)
- [Anatomy of an LCONFIG configuration file](#config)
- [Anatomy of a data file](#data)

## <a name="about"></a> About

In the summer of 2016, I realized I needed to be able to adjust my data acquisition parameters day-by-day as I was tweaking and optimizing an experiment.  I love my LabJacks, and the folks at LabJack do a great job of documenting their intuitive interface.  HOWEVER, the effort involved in tweaking DAQ properties on the fly AND documenting them with each experiment can be cumbersome.

This little **L**aboratory **CONFIG**uration system is designed to work with text-based configuration files using an intuitive human readable format.  The philosophy is that most (admittedly not all) laboratory jobs that require data acquisition are quite similar and can be configured using simple text without Labview or other expensive (computationally or monitarily) software.

All of the acquisition tools are written in C, and only the top-level binaries are specifically written for Linux.  Some minor tweaks should make the base system suitable for Windows.  Still, unless you are ready to dive into the source, I don't recommend it.

I have included two binaries, `lcrun` and `lcburst`, built on the LConfig system that do most of the jobs I need in the lab that also serve as examples for how to use LConfig.  `lcburst` collects high speed data in short bursts, and `lcrun` streams data directly to a data file until I tell it to stop.  It will run as long as you have hard drive space, so tests that last for days, months, or years become possible.  If these binaries don't work for you, the back-end configuration is contained in `lconfig.c`, `lconfig.h`, `lctools.c`, and `lctools.h`.  I occasionally build specialized codes on them, but for the most part, the binaries are good enough.

While I do my best to keep this README up to date, the authoritative documentation for the behavior of LCONFIG is contained in the headers, `lconfig.h` and `lctools.h`.  Each function has comments that defines its behavior, and there are even some examples that indicate how the functions should be used.  The comments to the `lc_load_config()` function give a detailed description for each configuration parameter.  There is also a changelog commented at the top of the header.

## Features
LCONFIG allows automatic configuration and control of most of the T4/T7's advanced features with a few added features for easier post-processing of the data.

- Stream sample rate configuration
- Analog input streaming
    - Input range
    - Settling time and resolution
    - Single-ended vs differential
    - Channel labeling
    - Linear calibration to engineering units (e.g. Volts to psi for pressure sensors)
    - Built-in buffer (no dynamic data management required)
- Digital input streaming
- Analog output function generator
    - Constant value
    - Sine wave offset, amplitude, frequency
    - Square wave offset, amplitude, frequency, duty cycle
    - Sawtooth offset, amplitude, frequency, duty cycle
    - White noise offset, amplitude, repetition frequency
    - Channel labeling
- Flexible Input/Output extended features
    - Two-channel encoder quadrature
    - Frequency input
    - Digital counter w/ configurable debounce filter
    - Line-to-line phase measurement
    - Pulse width measurement
    - Pulse width output with configurable phase
    - Channel labeling
- Software trigger
    - Rising/falling/any edge on any AI channel
    - Can use the high-speed counter to identify a digital trigger
    - Supports pretrigger buffering
- Digital Communication Interface
    - UART interface
- Automatic generation of data files
    - The active configuratino is embedded in the data
    - Stream directly to a file
- Post processing in Python
    - `lconfig' package loads configurations and data automatically
    - Applies calibrations
    - Index by channel number or channel label
- Error handling
    - Messages to STDERR include the funciton's name and a description of the failure
    - Each error message uniquely identifies a location in the code

## <a name="license"></a> License
This software is released under the [GNU General Public License version 3.0](https://www.gnu.org/licenses/gpl-3.0.en.html).  LCONFIG is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

## <a name="start"></a> Getting started

To get the repository,

```bash
$ git clone http://github.com/chmarti1/lconfig
$ cd lconfig
$ sudo make install	# Installs binaries to /usr/local/bin
$ lcrun -h   # Prints help
$ lcburst -h
```
`lcrun` and `lcburst` are binaries built on the LCONFIG system.  `lcrun` continuously streams data to a file, but the read/write times for most hard drives prevents realizing the full speed of the T7.  `lcburst` collects a high speed burst of data and then writes it to a file.  Both executables use the LCONFIG library to load a configuration file, open the appropriate device connections, initiate the data transfer, and write data files automatically.

For writing your own executables, a quick tutorial on programming with the LCONFIG system is laid out in LCONFIG_README with the gritty details in this README.  The absolute authoritative documentation for LCONFIG is the comments in the prototype section of the header itself.  It is always updated with each version change.  Versions 2.03 and older used a different system for streaming than version 3.00 and later, so older branches are still available on the git page.

Once you produce your data, there's a good chance that you'll want to analyze it.  If you're a Python user, the lconfig.py module will get you up and running quickly.  Its documentation is inline.

## <a name="config"></a> Anatomy of an LCONFIG configuration file

A configuration file has MANY optional contents, but all configuration files will have a few things in common.  ALL configuration directives have the same format; a parameter followed by a value separated by whitespace.
```bash
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
# supported yet
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
# The exception is when quotes are used.  Inside of quotes, whitespace
# and case are treated verbatim.
ailabel "Ambient Temperature"
aiunits "degC"

##
Configuration files end when the file ends or when the load_config() 
function encounters ##.  This text isn't commented because
it will never be read by load_config()
```

## <a name="data"></a> Anatomy of a data file

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
## Future Improvements

Some of the parameters have not been thoroughly tested; especailly the extended features.  I have done some basic testing, but I would not be surprised to learn that there are bugs that need attention there.  Please contact me if you find any!

As of version 4.00, only the UART digital COM interface is implemented.  It is not currently a high priority to implement the other interfaces, but one day...

## Known Bugs

No persisting known bugs.
