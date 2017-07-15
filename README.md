# LCONFIG

Headers and utilities for laboratory measurements and machine control with the 
[LabJack T7](https://labjack.com/products)

By Chris Martin [crm28@psu.edu](mailto:crm28@psu.edu)

Version 3.00

- [About](#about)
- [Getting started](#start)
- [Anatomy of an LCONFIG configuration file](#config)
- [Anatomy of a data file](#data)
- [Table of configuration parameters](#params)
- [The LCONFIG data types](#types)
- [The LCONFIG functions](#functions)
- [The LCONFIG header constants](#constants)

## <a name="about"></a> About
In the summer of 2016, I realized I needed to be able to adjust my data 
acquisition parameters day-by-day as I was tweaking and optimizing an 
experiment.  I love my LabJack, and the folks at LabJack do a great job of documenting their intuitive interface.  HOWEVER, the effort involved in tweaking DAQ properties on the fly AND documenting them with each experiment can be cumbersome.

This little **L**aboratory **CONFIG**uration system reads and writes configuration files and automatically writes a header with the current configuration in each data file.  LCONFIG connects to and configures the T7 to perform analog input and output streaming, and automatically configures the flexible digital IO extended features.  

### Features
LCONFIG allows automatic configuration and control of most of the T7's advanced features.

- Stream sample rate configuration
- Analog input streaming
    - Input range
    - Settling time and resolution
    - Single-ended vs differential
- Analog output streaming
    - Constant value
    - Sine wave offset, amplitude, frequency
    - Square wave offset, amplitude, frequency, duty cycle
    - Sawtooth offset, amplitude, frequency, duty cycle
    - White noise offset, amplitude, repetition frequency
- Flexible Input/Output extended features
    - Two-channel encoder quadrature
    - Frequency input
    - Digital counter w/ configurable debounce filter
    - Line-to-line phase measurement
    - Pulse width measurement
    - Pulse width output with configurable phase
- Simple text-based configuration files
- Automatic generation of data files
- Error handling

## <a name="start"></a> Getting started
To get the repository,

```
    $ git clone http://github.com/chmarti1/lab
    $ cd lab
    $ make drun
    $ make dburst
    $ ./drun -h   # prints help
    $ ./dburst -h # prints help
```
`drun` and `dburst` are binaries built on the LCONFIG system.  `drun` continuously streams data to a file, but the read/write times for most hard drives prevents realizing the full speed of the T7.  `dburst` collects a high speed burst of data and then writes it to a file.  Both executables use the LCONFIG library to load a configuration file, open the appropriate device connections, initiate the data transfer, and write data files automatically.

For writing your own executables, a quick tutorial on programming with the LCONFIG system is laid out in LCONFIG_README with the gritty details in this README.  The absolute authoritative documentation for LCONFIG is the comments in the prototype section of the header itself.  It is always updated with each version change.  Versions 2.03 and older used a different system for streaming than version 3.00 and later, so older branches are still available on the git page.

Once you produce your data, there's a good chance that you'll want to analyze it.  If you're a Python user, the lconfig.py module will get you up and running quickly.  Its documentation is inline.

## <a name="config"></a> Anatomy of an LCONFIG configuration file
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

## <a name="params"></a> Table of Parameters
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
| airesolution| integer 
| aochannel   | integer [0,1]                           | Analog Output | The physical analog output (DAC) channel to use for pseudo function generator output.
| aosignal    | constant, sine, square, triangle, noise | Analog Output | The type of cyclic signal to generate
| aoamplitude | floating point                          | Analog Output | Voltage amplitude of the AC component of the signal (ignored for constant signal).
| aooffset    | floating point                          | Analog Output | Offset (DC) voltage to add to the oscillating (AC) signal.
| aoduty      | floating point [0-1]                    | Analog Output | The duty cycle of triangle and square waves. This skews the signal to spend more time on one part of the wave than the other.
| trigchannel | integer [0-13]                          | Global        | The analog input to use for a software trigger.  This is NOT the physical channel, but the index of the ordered list of configured inputs.
| triglevel   | floating point [-10. - 10.]             | Global        | Software trigger threshold voltage.
| trigedge    | rising, falling, all                    | Global        | Software trigger edge type.
| trigpre     | non-negative integer [>=0]              | Global        | The "pretrigger" is the minimum number of samples per channel PRIOR to the triger event that will be included in the streamed data.
| aofrequency | floating point                          | Analog Output | Specifies the rate the wave will repeat in Hz.
| fiofrequency | floating point                         | Global        | Specifies the CLOCK0 rollover frequency.
| fiochannel  | integer [0-7]                           | Flexible IO   | The physical FIO channel to use for a digital IO operation.
| fiosignal   | pwm, count, frequency, phase, quadrature | Flexible IO  | The FIO signal/feature to use on this FIO channel.
| fiodirection | input, output                          | Flexible IO   | Is this channel an input or an output? (input is default)
| fioedge     | rising, falling, all                    | Flexible IO   | Specifies the behavior of FIO features that depend on edge direction (count, phase, PWM).
| fiodebounce | none, fixed, reset, minimum             | Flexible IO   | The debounce filter to be used for a counter input.
| fiousec     | floating point [>0]                     | Flexible IO   | A generic time parameter for configuring FIO channels.
| fiodegrees  | floating point                          | Flexible IO   | A generic phase parameter for configuring FIO channels.
| fioduty     | floating point [0-1]                    | Flexible IO   | Duty cycle for a PWM output.
| meta        | flt/float, int/integer, str/string, stop/end/none | Meta | Starts a meta parameter stanza.  Any unrecognized parameter name after this directive will be treated as a new meta parameter of the specified type.  If stop, end, or none is specified, the meta stanza is ended, and unrecognized parameters will cause errors again.
| flt:param   | floating point                          | Meta | Specifies a single floating point meta parameter.
| int:param   | integer                                 | Meta | Specifies a single integer parameter
| str:param   | string                                  | Meta | Specifies a single string parameter

## <a name="types"></a> LCONFIG data types
These are the data types provided by `lconfig.h`.

### AICONF Struct
Holds data relevant to the configuration of a single analog input channel

| Member | Type | Description
|------|:---:|------
| channel  | `unsigned int` | The physical channel number
| nchannel | `unsigned int` | The physical number of the negative channel
| range    | `double`       | The bipolar range (.01, .1, 1, 10)
| resolution | `unsigned int` | The channel's resolution index

### AOCONF Struct
A structure that holds data relevant to the configuration of a single analog output channel

| Member | Type | Valid for | Default | Description
|---|:---:|:---:|:---:|:---
|channel | `unsigned int` | all | - | Specifies the physical DAC channel
|signal | `enum AOSIGNAL` | all | - | Specifies the type of signal: `AO_CONSTANT` (a DC signal with no AC), `AO_SINE`, `AO_SQUARE`, `AO_TRIANGLE`, `AO_NOISE` (a series of pseudo-random samples)
|amplitude | `double` | sine, square, triangle, noise | `LCONF_DEF_AO_AMP` | Specifies the amplitude (max-mean) of the AC signal component in volts
|offset | `double` | all | `LCONF_DEF_AO_OFF` | Specifies a DC offset to superimpose with the signal in volts
|frequency | `double` | all | - | Specifies the rate at which the signal should repeat in Hz.  Determines the lowest frequency content in a `NOISE` signal.  `CONSTANT` signals should always be specified with a large frequency.
|duty | `double` | square, triangle | 0.5 | For a square wave, `duty` indicates the fractional time spent at the high value.  For a triangle wave, `duty` indicates the fractional time spent rising.

### FIOCONF Struct
A structure that holds data relevant to the configuration of a single flexible I/O channel

| Member | Type | Valid for | Default | Description
|---|:---:|:---:|:---:|:---
|channel | `int` | all | - | Specifies the physical FIO channel [0-7]
|direction | `enum` | all | `FIO_INPUT` | Specifies input/output direction: `FIO_INPUT`, `FIO_OUTPUT`
|signal | `enum` | all | - | Specifies the behavior for the channel: `FIO_NONE`, `FIO_PWM`, `FIO_COUNT`, `FIO_FREQUENCY`, `FIO_PHASE`, `FIO_QUADRATURE`
|edge | `enum` | pwm, phase, count | `FIO_RISING` | Specifies which edge should be interpreted by the channel: `FIO_RISING`, `FIO_FALLING`, `FIO_ALL`
|debounce | `enum` | count | `FIO_DEBOUNCE_NONE` | Specifies which debounce filter should be used by the interrupt counter: `FIO_DEBOUNCE_NONE`, `FIO_DEBOUNCE_FIXED`, `FIO_DEBOUNCE_RESTART`, `FIO_DEBOUNCE_MINIMUM`
|time  | `double` | count, pwm, frequency, phase | 0. | In microseconds, specifies the debounce duration for counters, indicates the measured period in PWM and frequency modes, and indicates the measured delay in phase mode.
|phase | `double` | pwm | 0. | In degrees, indicates the phase of a PWM output.  This can be useful for motor driver applications.
|duty | `double` | pwm | 0.5 | Indicates the duty cycle (measured or commanded) of a PWM signal.
|counts | `unsigned int` | count, pwm, phase, frequency | 0 | Indicates the status of a counter input, the measured period of a PWM or frequency input (in ticks), or the measured delay of a phase input (in ticks).

### RINGBUFFER Struct
This structure is responsible for managing the data stream behind the scenes.  This ring buffer struct is designed to allow data to stream continuously in chunks called "blocks."  These are nothing more than the size of the data blocks sent by `LJM_eReadStream()`, and LCONFIG always sets them to LCONF_SAMPLES_PER_READ samples per channel.

| Member | Subordinate | Type | Description 
|---|---|:---:|:---
|size_samples | | `unsigned int` | The total size of the buffer (NOT per channel)
|blocksize_samples | | `unsigned int` | The total size of each read/write block (NOT per channel)
|samples_per_read | | `unsigned int` | Samples per channel in each read/write block
|samples_read | | `unsigned int` | A record of the samples per channel read from the buffer by the application.
|samples_streamed | | `unsigned int` | A record of the samples per channel read into the buffer from the T7.
|channels | | `unsigned int` | The number of channels currently in the data stream.
|read | | `unsigned int` | The index in the data buffer to start reading.  When `read == write`, the buffer is empty.  When the buffer is emptied, `read` is forced to be equal to `size_samples`.
|write | | `unsigned int` | The index in the data buffer where new samples should be written.
|buffer | | `double *` | The starting address of the buffer.

### METACONF Struct
A structure that defines a single meta parameter

| Member | Subordinate | Type | Description 
|---|---|:---:|:---
|param | | `char[LCONF_MAX_STR]` | The string name of the meta parameter
|value | | `union` | A union for containing the value as an integer, double, or string
| | svalue | `char[LCONF_MAX_STR]` | A string value
| | ivalue | `int` | An integer value
| | fvalue | `double` | A floating point value
|type | | `char` | A character indicating the meta type: `'f'` for float, `'i'` for int, or `'s'` for a string


### DEVCONF Struct
This structure contains all of the information needed to configure a device.  This is the workhorse data type, and usually it is the only one that needs to be explicitly used in the host application.

| Member | Type | Valid values | Description
|---|:---:|:---:|---
|connection | `int` | `LJM_ctUSB`, `LJM_ctETH`, `LJM_ctANY` | Integer corresponding to the connection type.  The values are those recognized by LabJack's `LJM_Open` function.
|ip | `char[LCONF_MAX_STR]` | XXX.XXX.XXX.XXX | The T7's IP address.  Used to WRITE the IP address in USB mode, and used to find the device in ETH mode.
|gateway | `char[LCONF_MAX_STR]` | XXX.XXX.XXX.XXX | The default TCP/IP gateway.
|subnet | `char[LCONF_MAX_STR]` | XXX.XXX.XXX.XXX | The TCP/IP subnet mask.
|serial | `char[LCONF_MAX_STR]` | decimal characters | The device serial number. Used to find the device in USB mode and only if an IP address is not supplied in ETH mode.
|aich | `AICONF[LCONF_MAX_NAICH]` | - | An array of `AICONF` structs for configuring input channels.
|naich | `unsigned int` | - | The number of aich entries that have been configured
|aoch | `AOCONF[LCONF_MAX_NAOCH]` | - | An array of `AOCONF` structs for configuring output channels
|naoch | `unsigned int` | - | The number of aoch entries that have been configured
|trigchannel |  `int` | -1 | The `aich` channel to test for a trigger event
|trigmem |  `unsigned int` | 0 | Memory used in the trigger test
|triglevel | `double` | 0. | The trigger threshold in volts
|trigedge | `enum{TRIG_RISING, TRIG_FALLING, TRIG_ALL}` | `TRIG_RISING` | Trigger edge type
|trigstate | `enum{TRIG_IDLE, TRIG_PRE, TRIG_ARMED, TRIG_ACTIVE}`, | `TRIG_IDLE` | Trigger operational state
|fiofrequency | `double` | -1 | The flexible digital input/output rollover frequency
|fioch | `FIOCONF[LCONF_MAX_NFIOCH]` | - | An array of `FIOCONF` structs for configuring flexible digital I/O channels
|nfioch| `unsigned int` | - | The number of fioch entries that have been configured
|handle | `int` | - | The device handle returned by the `LJM_Open` function
|samplehz | `double`| positive frequency | The sample rate in Hz. This value will be overwritten with the ACTUAL device sample rate once an acquisition process has begun.
|settleus | `double`| positive time | The multiplexer settling time for each sample in microseconds
|nsample | `unsigned int` | - | Number of samples to read per each stream read operation
|meta | `METACONF[LCONF_MAX_META]` | - | An array of meta configuration parameters
|RB | `RINGBUFFER` | - | The data stream's buffer

## <a name="functions"></a> The LCONFIG functions

| Function | Description
|:---:|:---
| [**Interacting with Configuration Files**](#fun:config) ||
|load_config| Parses a configuration file and encodes the configuration on an array of DEVCONF structures
|write_config| Writes a configuration file based on the configuration of a DEVCONF struct
| [**Device Interaction**](#fun:dev) ||
|open_config| Opens a connection to the device identified in a DEVCONF configuration struct.  The handle is remembered by the DEVCONF struct.
|close_config| Closes an open connection to the device handle in a DEVCONF configuration struct
|upload_config| Perform the appropriate read/write operations to implement the DEVCONF struct settings on the T7
|download_config| Populate a fresh DEVCONF structure to reflect a device's current settings.  Not all settings can be verified, so only some of the settings are faithfully represented.
| [**Configuration Diagnostics**](#fun:diag) ||
|show_config| Calls download_config and automatically generates an item-by-item comparison of the T7's current settings and the settings contained in a DEVCONF structure.
|ndev_config | Returns the number of configured devices in a DEVCONF array
|nistream_config| Returns the number of configured input channels in a DEVCONF device structure
|nostream_config| Returns the number of configured output channels in a DEVCONF device
|status_data_stream| Returns the number of samples streamed from the T7, to the application, and waiting in the buffer
|iscomplete_data_stream| Returns a 1 if the number of samples streamed into the buffer is greater than or equal to the NSAMPLE configuration parameter
|isempty_data_stream| Returns a 1 if the buffer has no samples ready to be read
| [**Data Collection**](#fun:data) ||
|start_data_stream | Checks the available RAM, allocates the buffer, and starts the acquisition process
|service_data_stream | Collects new data from the T7, updates the buffer registers, tests for a trigger event, services the trigger state
|read_data_stream | Returns a pointer into the buffer with the next available data to be read
|stop_data_stream | Halts the T7's data acquisition process
|clean_data_stream| Frees the buffer memory
|init_file_stream | Writes a header to a data file
|write_file_stream | Calls read_data_stream and writes formatted data to a data file
|update_fio | Update all flexible I/O measurements and output parameters in the FIOCONF structs
| [**Meta Configuration**](#fun:meta) ||
| get_meta_int, get_meta_flt, get_meta_str | Returns a meta parameter integer, floating point, or string value.  
| put_meta_int, put_meta_flt, put_meta_str | Write an integer, floating point, or a string value to a meta parameter.
| **Helper Functions** | **(Not needed by most users)** |
|clean_file_stream | Clean up after a file stream process; close the file and deallocate the buffer.  This is done automatically by `stop_file_stream`.
|str_lower | Modify a string to be all lower case
|airegisters | Returns the modbus registers relevant to configuring an analog input channel
|aoregisters | Returns the modbus registers relevant to configuring an analog output channel
|print_color | Used by the `show_config` function to print colored text to the terminal
|read_param | Used by the `load_config` function to strip the whitespace around parameter/value pairs
|init_config | Writes sensible defaults to a configuration struct

<a name='fun:config'></a>
### Interacting with Configuration Files

```C
int load_config(          DEVCONF* dconf, 
                const unsigned int devmax, 
                       const char* filename)
```
`load_config` loads a configuration file into an array of DEVCONF structures.  Each instance of the `connection` parameter signals the configuration a new device.  `dconf` is an array of DEVCONF structs with `devmax` elements that will contain the configuration parameters found in the data file, `filename`.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
void write_config(        DEVCONF* dconf, 
                const unsigned int devnum, 
                             FILE* ff)
```
`write_config` writes a configuration stanza to an open file, `ff`, for the parameters of device number `devnum` in the array, `dconf`.  A DEVCONF structure written by `write_config` should result in an identical structure after being read by `load_config`.

Rather than accept file names, `write_config` accepts an open file pointer so it is convenient for creating headers for data files (which don't need to be closed after the configuration is written).  It also means that the write operation doesn't need to return an error status.

<a name='fun:dev'></a>
### Device Interaction
```C
int open_config(          DEVCONF* dconf, 
                const unsigned int devnum)
```
`open_config` opens a connection to the device described in the `devnum` element of the `dconf` array.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int close_config(         DEVCONF* dconf, 
                const unsigned int devnum)
```
`close_config` closes a connection to the device described in the `devnum` element of the `dconf` array.  If a ringbuffer is still allocated to the device, it is freed, so it is important NOT to call `close_config` before data is read from the buffer.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int upload_config(        DEVCONF* dconf, 
                const unsigned int devnum)
```
`upload_config` writes to the relevant modbus registers to assert the parameters in the `devnum` element of the `dconf` array.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int download_config(      DEVCONF* dconf, 
                const unsigned int devnum, 
                          DEVCONF* out)
```
`download_config` pulls values from the modbus registers of the open device connection in the `devnum` element of the `dconf` array to populate the parameters of the `out` DEVCONF struct.  This is a way to verify that the device parameters were successfully written or to see what operation was performed last.

It is important to note that some of the DEVCONF members configure parameters that are not used until a data operation is executed (like `nsample` or `samplehz`).  These parameters can never be downloaded because they do not reside persistently on the T7.

<a name='fun:diag'></a>
### Configuration Diagnostics
```C
int ndev_config(          DEVCONF* dconf, 
                const unsigned int devmax)
                
int nistream_config(      DEVCONF* dconf, 
                const unsigned int devnum)
                
int nostream_config(      DEVCONF* dconf, 
                const unsigned int devnum)
```
The `ndev_config` returns the number of devices configured in the `dconf` array, returning a number no greater than `devmax`.  `nistream_config` and `nostream_config` return the number of input and ouptut stream channels configured.

```C
void show_config(         DEVCONF* dconf, 
                const unsigned int devnum)
```
The `show_config` function calls `download_config` and prints a detailed color-coded comparison of live parameters against the configuration parameters in the `devnum` element of the `dconf` array.  Parameters that do not match are printed in red while parameters that agree with the configuration parameters are printed in green.

<a name='fun:data'></a>
### Data Collection
```C
int start_data_stream(    DEVCONF* dconf, 
                const unsigned int devnum,
                               int samples_per_read)

int service_data_stream(  DEVCONF* dconf, 
                const unsigned int devnum)

int read_data_stream(     DEVCONF* dconf, 
                const unsigned int devnum, 
                            double **data, 
                      unsigned int *channels, 
                      unsigned int *samples_per_read)

int stop_data_stream(     DEVCONF* dconf, 
                const unsigned int devnum)
```
There are four steps to a data acquisition process; start, service, read, and stop.  This constitutes a substantial change from version 2.03 and earlier, which had no service function.  Version 3 uses an automatically configured ring buffer, so the application never needs to perform memory management.  The addition of a service function means that the process of streaming new data into the buffer can be separated from the process of reading data from the buffer.  It also means that data streaming might not actually result in new data being made available (for example if a trigger event has not registered).

`start_data_stream` configures the LCONFIG buffer, and starts the T7's data collection process on the device `devnum` in the `dconf` array.  The device will be configured to transfer packets with `samples_per_read` samples per channel.  If `samples_per_read` is negative, then the LCONF_SAMPLES_PER_READ constant will be used instead.  This information is recorded in the `RB` ringbuffer struct.  The buffer can be substantial since RAM checks are performed prior to allocating memory.

The `service_data_stream` function retrieves a new block of data into the ring buffer.  If the trigger is configured, the service function is responsible for managing the pretrigger buffering, testing for whether a trigger event has occurred, and managing the ringbuffer registers.

When data is available in the ring buffer, the `read_data_stream` function returns a double precision array, `data` representing a 2D array with shape `samples_per_read` x `channels`.  The `s` sample of `c` channel can be retrieved by `data[s*channels + c]`, and the total size of the `data` array is `samples_per_read * channels`.  If there is no data available, then `data` will be `NULL`.

The data acquisition process is halted by the `stop_data_stream` function, but the buffer is left intact.  As a result, data can be slowly read from the buffer long after the data collection is complete.  Before a new stream process can be started, the `clean_data_stream` function should be called to free the buffer.  In applications where only one stream operation will be executed, it may be easier to allow `close_config` to clean up the buffer instead.

```C
void status_data_stream( DEVCONF* dconf, 
               const unsigned int devnum,
                     unsigned int *samples_streamed, 
                     unsigned int *samples_read,
                     unsigned int *samples_waiting);
                     
int iscomplete_data_stream(DEVCONF* dconf, 
                 const unsigned int devnum);
                 
int isempty_data_stream( DEVCONF* dconf, 
               const unsigned int devnum);
```
These functions are handy tools for monitoring the progress of a data collection process.  The `status_data_stream` function returns the per-channel stream counts streamed into, read out of, and waiting in the ring buffer.  Authors should keep in mind that the `service_data_stream` function adjusts the `samples_streamed` value to exclude data that was thrown away in the triggering process.

`iscomplete_data_stream` returns a 1 or 0 to indicate whether the total number of `samples_streamed` per channel has exceeded the `nsample` parameter found in the configuration file.

`isempty_data_stream` returns a 1 or 0 to indicate whether the ring buffer has been exhausted by read operations.

```C
int init_file_stream(    DEVCONF* dconf, 
                const unsigned int devnum,
                             FILE* FF)
                        
int write_file_stream(     DEVCONF* dconf, 
                const unsigned int devnum,
                             FILE* FF)

```
These are tools for automatically writing data files from the stream data.  They accept a file pointer from an open `iostream` file.  In versions 2.03 and older, LCONFIG was responsible for managing the file opening and closing process, but as of version 3.00, it is up the application to provide an open file.

`init_file_stream` writes a configuration file header to the data file.  It also adds a timestamp indicating the date and time that `init_file_stream` was executed.  It should be emphasized that (especially where triggers are involved) substantial time can pass between this timestamp and the availability of data.  Care should be taken if precise absolute time values are needed.

`write_file_stream` calls `read_data_stream` and prints an ascii formatted data array into the open file provided.


```C
int update_fio(DEVCONF* dconf, const unsigned int devnum)
```
The FIO channels represent the only features in LCONFIG that require users to interact manually with the DEVCONF struct member variables.  `update_fio` is called to command the T7 to react to changes in the FIO settings or to obtain new FIO measurements.  For example, the following code might be used to adjust flexible I/O channel 0's duty cycle to 25%.
```
dconf[devnum].fioch[0].duty = .25;
update_fio(dconf,devnum);
```
The `update_fio()` function also downloads the latest measurements into the relevant member variables.  The following code might be used to measure a pulse frequency from a flow sensor.
```C
double frequency;
int update_fio(dconf, devnum);
frequency = 1e6 / dconf[devnum].fioch[0].time;
```

The T7 supports streaming digital I/O, but `LCONFIG` does not support it as of version 3.00.  There are not currently plans to add this feature.

<a name='fun:meta'></a>
### Meta Configuration
```C
int get_meta_int(          DEVCONF* dconf, 
                 const unsigned int devnum,
                        const char* param, 
                               int* value)
                               
int get_meta_flt(          DEVCONF* dconf, 
                 const unsigned int devnum,
                        const char* param, 
                            double* value)

int get_meta_str(          DEVCONF* dconf, 
                 const unsigned int devnum,
                        const char* param, 
                              char* value)
```
The `get_meta_XXX` functions retrieve meta parameters from the `devnum` element of the `dconf` array by their name, `param`.  If the parameter does not exist or if it is of the incorrect type, 
The values are written to target of the `value` pointer, and the function returns the `LCONF_ERROR` or `LCONF_NOERR` error status based on whether the parameter was found.

```C
int put_meta_int(          DEVCONF* dconf, 
                 const unsigned int devnum, 
                        const char* param, 
                                int value)
                                
int put_meta_flt(          DEVCONF* dconf, 
                 const unsigned int devnum, 
                        const char* param, 
                             double value)

int put_meta_str(          DEVCONF* dconf, 
                 const unsigned int devnum, 
                        const char* param, 
                              char* value)
```
The `put_meta_XXX` functions retrieve meta parameters from the `devnum` element of the `dconf` array by their name, `param`.  The values are written to target of the `value` pointer, and the function returns the `LCONF_ERROR` or `LCONF_NOERR` error status based on whether the parameter was found.

## <a name="constants"></a> Table of LCONFIG constants
These are the compiler constants provided by `lconfig.h`.

| Constant | Value | Description
|--------|:-----:|-------------|
| TWOPI  | 6.283185307179586 | 2*pi comes in handy for signal calculations
| LCONF_VERSION | 3.00 | The floating point version
| LCONF_MAX_STR | 32   | The longest character string permitted when reading or writing values and parameters
| LCONF_MAX_READ | "%32s" | Format string used for reading parameters and values
| LCONF_MAX_STCH | 15     | Maximum number of stream channels (both input and output  permitted in the DEVCONF struct
| LCONF_MAX_AOCH | 1      | Highest analog output channel number allowed
| LCONF_MAX_NAOCH | 2     | Maximum number of analog output channels allowed
| LCONF_MAX_AICH | 13     | Highest analog input channel number allowed
| LCONF_MAX_NAICH | 14    | Maximum number of analog input channels allowed
| LCONF_MAX_AIRES | 8     | Maximum resolution index allowed
| LCONF_MAX_AOBUFFER | 512 | Maximum analog output buffer for repeating functions
| LCONF_BACKLOG_THRESHOLD | 1024 | Raise a warning if LJM reports a stream backlog greater than this value
| LCONF_CLOCK_MHZ | 80.0 | The T7 clock frequency in MHz
| LCONF_SAMPLES_PER_READ | 64 | The default samples_per_read value in streaming operations
| LCONF_DEF_NSAMPLE | 64 | Default value for the `nsample` parameter; used when it is unspecified
| LCONF_DEF_AI_NCH | 199 | Default value for the `ainegative` parameter
| LCONF_DEF_AI_RANGE | 10. | Default value for the `airange` parameter
| LCONF_DEF_AI_RES | 0. | Default value for the `airesolution` parameter
| LCONF_DEF_AO_AMP | 1. | Default value for the `aoamplitude` parameter
| LCONF_DEF_AO_OFF | 2.5 | Default value for the `aooffset` parameter
| LCONF_DEF_AO_DUTY | 0.5 | Default value for the `aoduty` parameter
| LCONF_NOERR | 0 | Value returned on successful exit
| LCONF_ERROR | 1 | Value returned on unsuccessful exit


## Known Bugs
No persisting known bugs.
