[back](documentation.md)

Version 4.00<br>
September 2019<br>
Chris Martin<br>

This is a handy quick-reference for functions and configuration options, but the authoritative and up-to-date documentation on the api and configuration interface is always in the `lconfig.h` and `lctools.h` headers themselves.

## <a name="ref:top"></a> Reference
- [Table of configuration parameters](#params)
- [LConfig data types](#types)
- [LConfig functions](#functions)
- [LConfig compiler constants](#constants)


## <a name="params"></a> Table of configuration parameters

These are the parameters recognized by LCONFIG.  The valid values list the values or classes of values that each parameter can legally be assigned.  The scope indicates where those values are applied.  Global scope parameters are always overwritten if they are repeated, but AI and AO parameters are written to which ever channel is active, and there may be many.

| Parameter   | Valid Values                            | Scope        | Description
|:------------|:---------------------------------------:|:------------:|:--|
| connection  | eth,usb,any                             | Global       | Starts configuration of a new device and specifies over what interface that device should be reached.
| serial      | integer                                 | Global       | Specifies a device by its serial number (ignored over eth when ip is also specified)
| name	| string					| Global		| Specifies a device y its name (or alias).
| ip          | XXX.XXX.XXX.XXX                         | Global       | Over an ETH connection, specifies the devices to configure, but over a USB connection, IP is used to set the devices IP address.
| gateway     | XXX.XXX.XXX.XXX                         | Global       | Used to set the T7's default TCP/IP gateway
| subnet      | XXX.XXX.XXX.XXX                         | Global       | Used to set the T7's TCP/IP subnet mask
| samplehz    | floating point                          | Global       | The sample rate per channel in Hz
| settleus    | floating point                          | Global       | The settling time per sample in microseconds. If less than 5, the T7 will choose automatically.
| nsample     | integer                                 | Global       | How many samples per channel should be included in each stream packet?
| aichannel   | integer [0-13]                          | Analog Input | The physical analog input channel number
| ainegative  | [0-13], 199, ground, differential       | Analog Input | The physical channel to use as the negative side of the measurement.  199 and ground both indicate a single-ended measurement.  The T7 requires that negative channels be the odd channel one greater than the even positive counterpart (e.g. 0+ 1- or 8+ 9-).  Specify differential to make that selection automatic.
| ailabel       | string                                              | Analog Input | This is a text label that can be used to identify the channel.
| aicalslope | float                                                                 | Analog Input | Used with `aicalzero` to form a linear calibration.
| aicalzero | float | Analog Input | calibrated value = (AICALSLOPE)*(voltage - AICALZERO)
| aicalunits | string | Analog Input | Used to name the calibrated nunits ("V" by default)
| airange     | float [.01, .1, 1., 10.]                | Analog Input  | The bipolar (+/-) voltage range for the measurement.  The T7 can accept four values.
| airesolution| integer 					| Analog Input | The resolution index to be used for the channel.  (See [Resolution Indexes](https://labjack.com/support/datasheets/t-series/ain#resolution))
| diostream | integer					| Global | A 16-bit mask of DIO channels (FIO/EIO only) to include in a data stream. (1=include, 0=exclude).  If `diostream` is 0, then no digital streaming is included.
| aochannel   | integer [0,1]                           | Analog Output | The physical analog output (DAC) channel to use for pseudo function generator output.
| aolabel | string | Analog Output | A string used to name the analog output channel.
| aosignal    | constant, sine, square, triangle, noise | Analog Output | The type of cyclic signal to generate
| aoamplitude | floating point                          | Analog Output | Voltage amplitude of the AC component of the signal (ignored for constant signal).
| aooffset    | floating point                          | Analog Output | Offset (DC) voltage to add to the oscillating (AC) signal.
| aoduty      | floating point [0-1]                    | Analog Output | The duty cycle of triangle and square waves. This skews the signal to spend more time on one part of the wave than the other.
| aofrequency | floating point                          | Analog Output | Specifies the rate the wave will repeat in Hz.
| trigchannel | integer [0-13]                          | Global        | The analog input to use for a software trigger.  This is NOT the physical channel, but the index of the ordered list of configured inputs.
| triglevel   | floating point [-10. - 10.]             | Global        | Software trigger threshold voltage.
| trigedge    | rising, falling, all                    | Global        | Software trigger edge type.
| trigpre     | non-negative integer [>=0]              | Global        | The "pretrigger" is the minimum number of samples per channel PRIOR to the triger event that will be included in the streamed data.
| effrequency | floating point                         | Global        | Specifies the CLOCK0 rollover frequency.
| efchannel  | integer [0-7]                           | Flexible IO   | The physical channel to use for a digital IO operation.
| eflabel | string | Flexible IO | A string used to name the EF channel.
| efsignal   | pwm, count, frequency, phase, quadrature | Flexible IO  | The EF signal/feature to use on this EF channel.
| efdirection | input, output                          | Flexible IO   | Is this channel an input or an output? (input is default)
| efedge     | rising, falling, all                    | Flexible IO   | Specifies the behavior of EF features that depend on edge direction (count, phase, PWM).
| efdebounce | none, fixed, reset, minimum             | Flexible IO   | The debounce filter to be used for a counter input.
| efusec     | floating point [>0]                     | Flexible IO   | A generic time parameter for configuring EF channels.
| efdegrees  | floating point                          | Flexible IO   | A generic phase parameter for configuring EF channels.
| efduty     | floating point [0-1]                    | Flexible IO   | Duty cycle for a PWM output.
| comchannel | uart, spi, i2c, 1wire, sbus             | COM           | The digital communications channel type.
| comin      | integer                                 | COM           | The digital communications input DIO pin number.
| comout     | integer                                 | COM           | The digital communications output DIO pin number.
| comclock   | integer                                 | COM           | The digital communications clock DIO pin number.
| comrate    | floating point > 0                      | COM           | The digital communications data rate in bits per second.
| comoptions | string                                  | COM           | The digital communications channel options; a specialized string that depends on the channel type.  (e.g. UART: 8N1 bits parity stop)
| meta        | flt/float, int/integer, str/string, stop/end/none | Meta | Starts a meta parameter stanza.  Any unrecognized parameter name after this directive will be treated as a new meta parameter of the specified type.  If stop, end, or none is specified, the meta stanza is ended, and unrecognized parameters will cause errors again.
| flt:param   | floating point                          | Meta | Specifies a single floating point meta parameter.
| int:param   | integer                                 | Meta | Specifies a single integer parameter
| str:param   | string                                  | Meta | Specifies a single string parameter

[top](#ref:top)

## <a name="types"></a> LCONFIG data types

These are the data types provided by `lconfig.h`.

### lc_aiconf_t Struct

Holds data relevant to the configuration of a single analog input channel
```C
// Analog input configuration
// This includes everyting the DAQ needs to configure and AI channel
typedef struct {
    unsigned int    channel;     // channel number (0-13)
    unsigned int    nchannel;    // negative channel number (0-13 or 199)
    double          range;       // bipolar input range (0.01, 0.1, 1.0, or 10.)
    unsigned int    resolution;  // resolution index (0-8) see T7 docs
    double          calslope;   // calibration slope
    double          calzero;    // calibration offset
    char            calunits[LCONF_MAX_STR];   // calibration units
    char            label[LCONF_MAX_STR];   // channel label
} lc_aiconf_t;
```

[top](#ref:top)


### lc_aoconf_t Struct

A structure that holds data relevant to the configuration of a single analog output channel

```C
// Analog output configuration
// This includes everything we need to know to construct a signal for output
typedef struct {
    unsigned int    channel;      // Channel number (0 or 1)
    // What function type is being generated?
    enum {LC_AO_CONSTANT, LC_AO_SINE, LC_AO_SQUARE, LC_AO_TRIANGLE, LC_AO_NOISE} signal;
    double          amplitude;    // How big?
    double          frequency;    // Signal frequency in Hz
    double          offset;       // What is the mean value?
    double          duty;         // Duty cycle for a square wave or triangle wave
                                  // duty=1 results in all-high square and an all-rising triangle (sawtooth)
    char            label[LCONF_MAX_STR];   // Output channel label
} lc_aoconf_t;
```

[top](#ref:top)


### Enumerated types

The supported LabJack connection types mapped to their LJM header values.
```C
typedef enum {
    LC_CON_NONE=-1,
    LC_CON_USB=LJM_ctUSB, 
    LC_CON_ANY=LJM_ctANY, 
    LC_CON_ANY_TCP=LJM_ctANY_TCP,
    LC_CON_ETH=LJM_ctETHERNET_ANY,
    LC_CON_ETH_TCP = LJM_ctETHERNET_TCP,
    LC_CON_ETH_UDP = LJM_ctETHERNET_UDP,
    LC_CON_WIFI = LJM_ctWIFI_ANY,
    LC_CON_WIFI_TCP = LJM_ctWIFI_TCP,
    LC_CON_WIFI_UDP = LJM_ctWIFI_UDP
} lc_con_t;
```

The supported LabJack devices mapped to their LJM header values.
```C
typedef enum {
    LC_DEV_NONE=-1,
    LC_DEV_ANY=LJM_dtANY, 
    LC_DEV_T4=LJM_dtT4,
    LC_DEV_T7=LJM_dtT7,
    LC_DEV_TX=LJM_dtTSERIES,
    LC_DEV_DIGIT=LJM_dtDIGIT
} lc_dev_t;
```

An enumerated type defining an edge type for triggering and extended features.

```C
typedef enum {LC_EDGE_RISING, LC_EDGE_FALLING, LC_EDGE_ANY} lc_edge_t;
```

[top](#ref:top)


### lc_efconf_t Struct

Extended features configuration for a single channel

```C
// Flexible Input/Output configuration struct
// This includes everything needed to configure an extended feature EF channel
typedef struct {
    // Flexible IO mode enumerated type
    enum {  LC_EF_NONE,   // No extended features
            LC_EF_PWM,    // Pulse width modulation (input/output)
            LC_EF_COUNT,  // Counter (input/output)
            LC_EF_FREQUENCY,  // Frequency (input)
            LC_EF_PHASE,  // Line-to-line phase (input)
            LC_EF_QUADRATURE  // Encoder quadrature (input)
        } signal;

    lc_edge_t edge;

    enum {  LC_EF_DEBOUNCE_NONE,      // No debounce
            LC_EF_DEBOUNCE_FIXED,     // Fixed debounce timer (rising, falling)
            LC_EF_DEBOUNCE_RESET,     // Self restarting timer (rising, falling, all)
            LC_EF_DEBOUNCE_MINIMUM    // Minimum pulse width (rising, falling)
        } debounce;

    enum {  LC_EF_INPUT, LC_EF_OUTPUT } direction;
    int channel;
    double time;        // Time parameter (us)
    double duty;        // PWM duty cycle (0-1)
    double phase;       // Phase parameters (degrees)
    unsigned int counts; // Pulse count
    char label[LCONF_MAX_STR];
} lc_efconf_t;

```

[top](#ref:top)


### lc_comconf_t Struct

```C
// Digital Communications Configuration Structure
//
typedef struct {
    enum {LC_COM_NONE, LC_COM_UART, LC_COM_1WIRE, LC_COM_SPI, LC_COM_I2C, LC_COM_SBUS} type;
    char label[LCONF_MAX_STR];
    double rate;                // Data rate in bits/sec
    int pin_in;                 // Physical input DIO pin
    int pin_out;                // Physical output DIO pin
    int pin_clock;              // Physical clock DIO pin
    // All interface-specific configuration options are included in the
    // "options" union.  There is a separate member for each supported interface
    union {
        struct {
            unsigned int bits;
            enum {LC_PARITY_NONE=0, LC_PARITY_ODD=1, LC_PARITY_EVEN=2} parity;
            unsigned int stop;
        } uart;
    } options;
} lc_comconf_t;
```

[top](#ref:top)


### lc_meta_t Struct

```C
// The lc_meta_t is a struct for user-defined flexible parameters.
// These are not used to configure the DAQ, but simply data of record
// relevant to the measurement.  They may be needed by the parent program
// they could hold calibration information, or they may simply be a way
// to make notes about the experiment.
typedef struct {
    char param[LCONF_MAX_STR];      // parameter name
    union {
        char svalue[LCONF_MAX_STR];
        int ivalue;
        double fvalue;
    } value;                        // union for flexible data types
    char type;                      // reminder of what type we have
} lc_meta_t;
```
[top](#ref:top)


### lc_ringbuf_t Struct

This structure is responsible for managing the data stream behind the scenes.  This ring buffer struct is designed to allow data to stream continuously in chunks called "blocks."  These are nothing more than the size of the data blocks sent by `LJM_eReadStream()`, and LCONFIG always sets them to LCONF_SAMPLES_PER_READ samples per channel.

```C
// Ring Buffer structure
// The LCONF ring buffer supports reading and writing in R/W blocks that mimic
// the T7 stream read block.  
typedef struct {
    unsigned int size_samples;      // length of the buffer array (NOT per channel)
    unsigned int blocksize_samples; // size of each read/write block
    unsigned int samples_per_read;  // samples per channel in each block
    unsigned int samples_read;      // number of samples read since streaming began
    unsigned int samples_streamed;  // number of samples streamed from the T7
    unsigned int channels;          // channels in the stream
    unsigned int read;              // beginning index of the next read block
    unsigned int write;             // beginning index of the next write block
    double* buffer;                 // the buffer array
} lc_ringbuf_t;
```
[top](#ref:top)


### lc_devconf_t Struct

This structure contains all of the information needed to configure a device.  This is the workhorse data type, and usually it is the only one that needs to be explicitly used in the host application.

```C
typedef struct devconf {
    // Global configuration
    lc_con_t connection;                 // requested connection type index
    lc_con_t connection_act;             // actual connection type index
    lc_dev_t device;                     // The requested device type index
    lc_dev_t device_act;                 // The actual device type
    char ip[LCONF_MAX_STR];         // ip address string
    char gateway[LCONF_MAX_STR];    // gateway address string
    char subnet[LCONF_MAX_STR];     // subnet mask string
    char serial[LCONF_MAX_STR];     // serial number string
    char name[LCONF_MAX_NAME];      // device name string
    int handle;                     // device handle
    double samplehz;                // *sample rate in Hz
    double settleus;                // *settling time in us
    unsigned int nsample;           // *number of samples per read
    // Analog input
    lc_aiconf_t aich[LCONF_MAX_NAICH];    // analog input configuration array
    unsigned int naich;             // number of configured analog input channels
    // Digital input streaming
    unsigned int distream;          // input stream mask
    // Analog output
    lc_aoconf_t aoch[LCONF_MAX_NAOCH];   // analog output configuration array
    unsigned int naoch;             // number of configured analog output channels
    // Flexible Input/output
    double effrequency;            // flexible input/output frequency
    lc_efconf_t efch[LCONF_MAX_NEFCH]; // flexible digital input/output
    unsigned int nefch;            // how many of the EF are configured?
    // Communication
    lc_comconf_t comch[LCONF_MAX_COMCH]; // Communication channels
    unsigned int ncomch;            // how many of the com channels are configured?
    // Trigger
    int trigchannel;                // Which channel should be used for the trigger?
    unsigned int trigpre;           // How many pre-trigger samples?
    unsigned int trigmem;           // Persistent memory for the trigger
    double triglevel;               // What voltage should the trigger seek?
    lc_edge_t trigedge; // Trigger edge
    enum {LC_TRIG_IDLE, LC_TRIG_PRE, LC_TRIG_ARMED, LC_TRIG_ACTIVE} trigstate; // Trigger state
    // Meta & filestream
    lc_meta_t meta[LCONF_MAX_META];  // *meta parameters
    lc_ringbuf_t RB;                  // ring buffer
} lc_devconf_t;
```

[top](#ref:top)

## <a name="functions"></a> The LCONFIG functions

| Function | Description
|:---:|:---
| [**Interacting with Configuration Files**](api.md#fun:config) ||
|load_config| Parses a configuration file and encodes the configuration on an array of DEVCONF structures
|write_config| Writes a configuration file based on the configuration of a DEVCONF struct
| [**Device Interaction**](api.md#fun:dev) ||
|open_config| Opens a connection to the device identified in a DEVCONF configuration struct.  The handle is remembered by the DEVCONF struct.
|close_config| Closes an open connection to the device handle in a DEVCONF configuration struct
|upload_config| Perform the appropriate read/write operations to implement the DEVCONF struct settings on the T7
|download_config| Populate a fresh DEVCONF structure to reflect a device's current settings.  Not all settings can be verified, so only some of the settings are faithfully represented.
| [**Configuration Diagnostics**](api.md#fun:diag) ||
|show_config| Calls download_config and automatically generates an item-by-item comparison of the T7's current settings and the settings contained in a DEVCONF structure.
|ndev_config | Returns the number of configured devices in a DEVCONF array
|nistream_config| Returns the number of configured input channels in a DEVCONF device structure.  This includes analog AND digital streaming.
|nostream_config| Returns the number of configured output channels in a DEVCONF device
|aichan_config| Returns the range of legal analog input channels on the actual device type (`device_act`)
|aochan_config| Returns the range of legal anaog output channels on the actual device type
|efchan_config| Returns the range of legal digital extended feature channels on the actual device type
|status_data_stream| Returns the number of samples streamed from the T7, to the application, and waiting in the buffer
|iscomplete_data_stream| Returns a 1 if the number of samples streamed into the buffer is greater than or equal to the NSAMPLE configuration parameter
|isempty_data_stream| Returns a 1 if the buffer has no samples ready to be read
| [**Data Collection**](api.md#fun:data) ||
|start_data_stream | Checks the available RAM, allocates the buffer, and starts the acquisition process
|service_data_stream | Collects new data from the T7, updates the buffer registers, tests for a trigger event, services the trigger state
|read_data_stream | Returns a pointer into the buffer with the next available data to be read
|stop_data_stream | Halts the T7's data acquisition process
|clean_data_stream| Frees the buffer memory
|init_file_stream | Writes a header to a data file
|write_file_stream | Calls read_data_stream and writes formatted data to a data file
|status_data_stream | Retrieves information on the data streaming process
|update_ef | Update all flexible I/O measurements and output parameters in the EFCONF structs
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

[top](#ref:top)

## <a name="constants"></a> Table of LCONFIG constants

These are the compiler constants provided by `lconfig.h`.

| Constant | Value | Description
|--------|:-----:|-------------|
| TWOPI  | 6.283185307179586 | 2*pi comes in handy for signal calculations
| LCONF_VERSION | 3.06 | The floating point version number
| LCONF_MAX_STR | 80  | The longest character string permitted when reading or writing values and parameters
| LCONF_MAX_NAME | 49 | LJM's maximum device name length
| LCONF_MAX_META | 32 | Maximum number of meta parameters
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

[top](#ref:top)
