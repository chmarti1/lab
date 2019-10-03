
/*
  This file is part of the LCONFIG laboratory configuration system.

    LCONFIG is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LCONFIG is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <https://www.gnu.org/licenses/>.

    Authored by C.Martin crm28@psu.edu
*/


/*
Use the commands below to compile your code in a *nix environment
You're on your own in Windows.  If you're in Windows, just about
everything should work, but the show_config() function may give
strange results, and start_data_stream() may need some tweaking.

$gcc -c lconfig.c -o lconfig.o
$gcc your_code.c lconfig.o -lm -lLabJackM -o your_exec.bin
$chmod a+x your_exec.bin
*/

#ifndef __LCONFIG
#define __LCONFIG

#include <stdio.h>
#include <LabJackM.h>


#define LCONF_VERSION 4.00   // Track modifications in the header
/*
These change logs follow the convention below:
**LCONF_VERSION
Date
Notes

**NONE
?/?/?
Original copy
There was no LCONF_VERSION constant defined.  Included configuration file
analog input, calibration, and ascii data export.

**1.1
6/20/16
First implementation in The Lab
Defined LCONF_VERSION constant and added this log.
Improved commenting in the header file.  
Added support for analog output.
Removed calibration information from the lc_aiconf_t struct
Added meta configuration

**1.2
7/8/16
Added NSAMPLE and SETTLEUS parameters to the global configuration set.
NSAMPLE lets the configuration file specify the number of samples per read, with
a default of 64 if it is not specified.
SETTLEUS exposes the settling time to the user.  Before, a value of 0us was used
to prompt the T7 to choose for us.  Now, 1us is used by default, so the LJ will
still choose for us unless we specify a value greater than 5us.
Corrected a bug in get_met_flt() that tried to treat the value as an integer.

**1.3
11/22/16
Corrected a bug that prevented multiple analog output streams from working 
correctly.

**2.01
3/29/17
Split the original header file into a c file and a header for efficiency and 
readability.

**2.02
4/10/2017
Added ndev_config() to detect the number of devices loaded from a configuration
file.

**2.03
7/2017
- Transitioned to name-based addressing instead of explicit addressing for
adaptation to future modbus upgrades.
- Added EF extended feature support.

**3.00
7/2017
- Reorganized the data streaming system: transitioned away from parallel file
and data streaming.  Implemented the start, service, read, and stop_data_stream 
with init_data_file() and write_data_file() utilities.
- Implemented a ring buffer for automatic streaming.
- Added a software trigger.

**3.01
7/2017
- Added the HSC (high speed counter) digital trigger option

**3.02
9/2017
- Added labels to aichannels, aochannels, and efchannels
- Updated lconfig.py to work properly with 3.02
- Added the .bylabel dictionary to the dfile python objects

** 3.04
3/2019
- Forked off LCNOFIG from LAB to eliminate specialized files.
- Wrote and proofed drun and dburst generic data collection binaries
- Re-wrote python data loading support; shifted to dictionary-based config
- 3.03 added the AICAL parameters, but they were not fully tested, nor
    were they properly documented.  3.04 brings them in properly.
- Added support for string parsing; inside of "" parameters can be 
    capital letters, and whitespace is allowed.
- Lengthened the longest supported string to 80 characters.

** 3.05
3/2019
- Added mandatory quotes when writing string parameters to config and
    data files

** 3.06
6/2019
- Added support for the T4
    - NOTE: advanced EF features have NOT been tested with the T4
- Added support for digital input streaming
- Changed error handling algorithms
- Removed a clear_buffer() in stop_data_stream() that caused dburst to fail

** 4.00
9/2019
- BREAKS REVERSE COMPATIBILITY!!!
- Implemented a new funciton naming scheme
- Implemented digital communications interface
- Replaced configuration array/index pairs with a single configuration pointer
    in virtually all lconfig functions.
- Removed DOWNLOAD_CONFIG
- Changed "FIO" to "EF" to conform to LJ's extended features naming.
- Created the LCM mapping interface for consistent mappings between enumerated
    types, their configuration strings, and human-readable messgaes.  This ALSO
    corrected a bug in the extended feature debounce filter settings.
- Renamed the binaries from drun and dburst to lcrun and lcburst
- Added LCTOOLS tools for building simple UIs in terminals without Ncurses
- Rewrote LC_SHOW_CONFIG to no longer attempt to verify live parameters
- Added LC_COMMUNICATE, LC_COM_START, LC_COM_STOP, LC_COM_READ, and LC_COM_WRITE
    for digital communications support.

*/

#define TWOPI 6.283185307179586

#define LCONF_MAX_STR 80    // longest supported string
#define LCONF_MAX_NAME 49   // longest device name allowed
#define LCONF_MAX_META 32   // how many meta parameters should we allow?
#define LCONF_MAX_STCH  LCONF_MAX_AICH + LCONF_MAX_AOCH + 1 // Maximum streaming channels
#define LCONF_MAX_AOCH 1    // maximum analog output channel number allowed
#define LCONF_MAX_NAOCH 2   // maximum analog output channels to allow
#define LCONF_MAX_AICH 14   // highest analog input channel number allowed
#define LCONF_MAX_NAICH 15  // maximum analog input channels to allow
#define LCONF_MAX_AIRES 8   // maximum resolution index
#define LCONF_MAX_NDEV 32   // catch runaway cases if the user passes junk to devmax
#define LCONF_MAX_EFCH 22  // Highest flexible IO channel
#define LCONF_MAX_NEFCH 8  // maximum flexible IO channels to allow
#define LCONF_MAX_COMCH 22  // Highest digital communications channel
#define LCONF_MAX_UART_BAUD 38400   // Highest COMRATE setting when in UART mode
#define LCONF_MAX_NCOMCH 4  // maximum com channels to allow
#define LCONF_MAX_AOBUFFER  512     // Maximum number of buffered analog outputs
#define LCONF_BACKLOG_THRESHOLD 1024 // raise a warning if the backlog exceeds this number.
#define LCONF_CLOCK_MHZ 80.0    // Clock frequency in MHz
#define LCONF_SAMPLES_PER_READ 64  // Data read/write block size
#define LCONF_TRIG_HSC 3000

#define LCONF_SE_NCH 199    // single-ended negative channel number

#define LCONF_DEF_NSAMPLE 64
#define LCONF_DEF_AI_NCH LCONF_SE_NCH
#define LCONF_DEF_AI_RANGE 10.
#define LCONF_DEF_AI_RES 0
#define LCONF_DEF_AO_AMP 1.
#define LCONF_DEF_AO_OFF 2.5
#define LCONF_DEF_AO_DUTY 0.5
#define LCONF_DEF_EF_TIMEOUT 1000

#define LCONF_NOERR 0
#define LCONF_ERROR -1


/*
.   Typedef
*/


/*
.   *Entry descriptions beginning with a * are not explicitly used to configure 
.   the T7.  If they are specified, they are loaded and are available for
.   the application.  The data type is the only error checking performed.
.
.   The T7's AI registers start at address 0, and increase in 2s, so the 
.   register for a given channel will be twice the channel number.
*/

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
//  The calibration and zero parameters are used by the aical function

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

typedef enum {LC_EDGE_RISING, LC_EDGE_FALLING, LC_EDGE_ANY} lc_edge_t;

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


typedef enum {
    LC_DEV_NONE=-1,
    LC_DEV_ANY=LJM_dtANY, 
    LC_DEV_T4=LJM_dtT4,
    LC_DEV_T7=LJM_dtT7,
    LC_DEV_TX=LJM_dtTSERIES,
    LC_DEV_DIGIT=LJM_dtDIGIT
} lc_dev_t;

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


/*
.
.   Prototypes
.
*/


/* LC_NDEV
Return the number of configured device connections in a lc_devconf_t array.
Counts until it finds a device with a connection index less than 0 (indicating
that it was never configured) or until the devmax limit is reached.  That means
that LC_NDEV will be fooled if lc_devconf_t elements are not configured 
sequentially.  Fortunately, load_config always works sequentially.
*/
int lc_ndev(lc_devconf_t* dconf, // Array of device configuration structs
                const unsigned int devmax); // maximum number of devices allowed

/* LC_ISOPEN
Returns a 1 if the configuration struct has a non-negative handle value; 
indicating that the LC_OPEN function has been called to establish a connection.
Returns 0 otherwise.
*/
int lc_isopen(lc_devconf_t* dconf);

/* LC_NISTREAM
Returns the number of input stream channels configured. These will be the number 
of columns of data discovered in the data when streaming.  This is the number of
analog input channels plus the digital input stream (if configured).
*/
int lc_nistream(lc_devconf_t* dconf);

/* LC_NOSTREAM
Returns the number of output stream channels configured. These will be the number 
of columns of data discovered in the data when streaming.
*/
int lc_nostream(lc_devconf_t* dconf);

/* LC_AICHANNELS
Determine the range of valid analog input channels for the current device 
configuration.  MIN is the lowest valid channel number.  MAX is the highest 
valid channel number.  Valid channel numbers are presumed to be sequential.
*/
void lc_aichannels(const lc_devconf_t* dconf, int *min, int *max);

/* LC_AOCHANNELS
Determine the range of valid analog output channels for the current device
configuration.  MIN is the lowest valid channel number.  MAX is the highest 
valid channel number.  Valid channel numbers are presumed to be sequential.
*/
void lc_aochannels(const lc_devconf_t* dconf, int *min, int *max);

/* LC_EFCHANNELS
Determine the range of valid extended feature IO channels for the current device
configuration.  MIN is the lowest valid channel number.  MAX is the highest 
valid channel number.  Valid channel numbers are presumed to be sequential.
*/
void lc_efchannels(const lc_devconf_t* dconf, int *min, int *max);

/* DIOCHAN_CONFIG
Determine the range of valid digital IO channels for the current device
configuration.  MIN is the lowest valid channel number.  MAX is the highest 
valid channel number.  Valid channel numbers are presumed to be sequential.
*/
void lc_diochannels(const lc_devconf_t* dconf, int *min, int *max);


/* LC_LOAD_CONFIG
Load a file by its file name.
The file should be constructed with parameter-value sets separated by 
whitespace.  For example
.   connection eth
.   ip 192.168.10.2
defines a connection to the device at a particular ip address.  
.
Comments are initiated with a # character, and continue to the end of line.
The # character in the interior of a parameter or value will be ignored.
.
A double ## indicates the end of the configuration area, and LOAD_CONFIG will
halt when it encounters this token at the beginning of a parameter word.  This
is particularly useful when loading the configuration header from a data file.
.
All entries, both parameters and values, are forced to lower-case, so that 
directives are case insensitive.  For the most part, that is the preferred
behavior, but especially in labels, it is useful to use both case and white
space to form aesthetic channel labels.  All characters within quotation 
marks will NOT be interpreted - including whitespace.  For example,
.   ailabel Ambient Temperature (K)
causes an error since the "ailabel" parameter will be set to "Ambient", and
the LOAD_CONFIG algorithm will then attempt to set the "Temperature" parameter
to "(K)", which is, of course, nonsense.  The problem is addressed by using
quotes.
.   ailabel "Ambient Temperature (K)"
.
The following parameters are recognized:
-CONNECTION
.   Expects a string "eth", "usb", or "any" to specify the type of connection
.   The connection parameter flags the creation of a new connection to 
.   configure.  Every parameter-value pair that follows will be applied to the
.   preceeding connection.  As a result, the connection parameter must come 
.   before any other parameters.
-DEVICE
.   Determines the type of device to connect to.  Currently supported values
.   are "t7" or "t4".
-SERIAL
.   Identifies the device by its serial number.  Devices can be identified by
.   their serial number, NAME, or IP address.  When multiples of these are
.   defined simultaneously, the device is queried to be certain they are all
.   consistent.  Contradictions will result in an error from open_config().
.
.   The precedence rules change slightly based on the conneciton type:
.   ANY:    SERIAL, NAME
.   USB:    SERIAL, NAME
.   ETH:    IP, SERIAL, NAME
-NAME
.   Identifies the device by its name or alias.  See SERIAL for more about 
.   how LConfig finds devices.  Names must be 49 or fewer characters with no
.   periods (".").  To use spaces and upper-case characters, put the value in
.   quotes.
-IP
.   The static IP address to use for an ethernet connection.  If the connection
.   is ETH, then the ip address will be used to to identify the device.  If the
.   connection is set to USB, then the IP address will be treated like any 
.   other parameter, and will be written to the T7 with the upload_config()
.   function.  If the connection is ANY, a non-empty IP value causes a warning
.   and will be ignored.  See SERIAL for more about how LConfig identifies
.   devices.
-GATEWAY, SUBNET
.   These parameters are only used if the device connection is USB.  If the
.   conneciton is through ethernet, lconfig can not make changes to any of
.   the TCP parameters.  These are the IP address for the subnet's gateway
.   and the subnet mask respectively.
-SAMPLEHZ
.   This parameter is not explicitly used to configure the T7.  It is intended
.   to specify the sample rate in Hz for streaming applications run by the 
.   application.
-SETTLEUS
.   Settling time to be used during AI streaming, specified in microseconds.  
.   The minimum settling time supported is > 5us.  Any value <= 5us will prompt
.   the T7 to select a settling time automatically.  For applications where 
.   synchronous data sampling isn't important, long settling times are good.
-NSAMPLE
.   Number of samples to be collected per read in AI streaming.  Here, we use 
.   the word "sample" in the same way LabJack uses the word "scan".  In multi-
.   channel configurations NSAMPLE * NAICH individual data points will be read.
.
.   Unlike most parameters, the NSAMPLE value defaults to LCONF_DEF_NSAMPLE 
.   (64), which is intended to be a quasi-reasonable value in most cases.  This 
.   is so users shouldn't need to set the parameter unless they want to.
-AICHANNEL
.   Short for Analog Input CHannel, this specifies one of the analog inputs
.   for the creation of a new channel.  All analog input parameters that follow
.   will be applied to this channel.  As a result, this parameter must preceed
.   any other analog input parameters.  Valid values are integers 0-13.
-AIRANGE
.   Short for Analog Input Range, this specifies the amplitude of the expected
.   analog measurement in volts.  It is used by the T7 to configure the channel
.   gain.  While any positive floating point less than 10. is acceptable, the
.   LabJack software will automatically choose a range of 0.01, 0.1, 1.0, or 
.   10. volts.  
-AINEGATIVE
.   Short for Analog Input Negative channel, this parameter specifies whether
.   the measurement is intended to be single-ended (relative to ground) or
.   differential (relative to a negative input).  This parameter accepts four
.   possible values; strings "differential" or "ground", the integer 199, or
.   the integer channel number to use as the negative input.
.
.   "ground" and 199 are equivalent.  Both imply a single-ended measurement 
.   where the channel's positive input is measured relative to the T7's
.   internal ground.  Great care should be made to avoid ground loops.
.
.   When the channel being configured is even, "differential" will cause 
.   lconfig to automatically select the next (odd) channel for negative input.
.   For example, when configuring channel AI8, AI9 will be used for the 
.   negative input.  If the channel being configured is not even, a 
.   differential measurement is not allowed, and lconfig will raise an error.
.
.   When specifying the negative channel index explicitly, only the next higher
.   value is acceptable.  For example, when configuring channel AI8, asking for
.   the negative input to be AI3 will cause lconfig to raise an error.
-AIRESOLUTION
.   The resolution index is used by the T7 to decide how many bits of 
.   resolution should be resolved by the ADC.  When AIRESOLUTION is 0, the T7
.   will decide automatically.  Otherwise, AIRESOLUTION will indicate whether
.   to truncate the precision of the ADC process in the name of speed.  See the
.   T7 documentation for more information.  Unless you know what you're doing,
.   this value should be left at 0.
-AICALZERO, AICALSLOPE
.   The AICALZERO and AICALSLOPE directives form a linear calibration 
.   that can be applied to the voltage data in post-processing.  The 
.   calibrated data should be computed in the form
.       meas = (V - AICALZERO) * AICALSLOPE
.   where "meas" is the calibrated measurement value, and "V" is the raw 
.   voltage measurement.  For obvious reasons, these are floting point
.   parameters.
-AICALUNITS
.   This optional string can be used to specify the units for the 
.   calibrated measurement specified by AICALZERO and AICALSLOPE.
-DISTREAM
.   When enabled, the lowest 16 DIO bits (EF and EIO registers) are streamed
.   as an additional input stream as if the integer value were an extra analog 
.   input.  To enable DIstreaming, the DISTREAM parameter should be a non-zero
.   integer mask for which of the channels should be treated as inputs.  For 
.   example, to set DIO0 and DIO4 as streaming inputs, DISTREAM should be set
.   to 2^0 + 2^4 = 17.
.
.   When EF settings contradict DISTREAM settings, the EF settings are given
.   precedence.  Be careful, because LCONFIG does not currently check for this
.   type of contradiction.
-AOCHANNEL
.   This parameter indicates the channel number to be configured for cyclic 
.   dynamic output (function generator).  This will be used to generate a 
.   repeating analog output function while a stream operation is active.
.   All AO (analog output) configuration words that are encountered after
.   this parameter will be applied to this channel.  As a result, it the 
.   AOCH paramter must appear before any AO configuration parameters.
-AOSIGNAL
.   The Analog Output Signal is the type of signal to be generated.  Valid
.   options are CONSTANT, SINE, SQUARE, TRIANGLE, and NOISE.  In order to 
.   generate a sawtooth wave, select TRIANGLE, and adjust the duty cycle to 
.   1 or 0.  See AODUTY.
.
.   The NOISE parameter generates a series of random samples and repeats the
.   sequence at the frequency AOFREQUENCY.  This creates a signal with quasi-
.   even frequency content between SAMPLEHZ and AOFREQUENCY.
-AOFREQUENCY
.   The Analog Output Freqeuncy expects a floating point number in Hz.  Great
.   care should be taken to ensure that this is a number compatible with the
.   SAMPLEHZ parameter.  AO samples will be generated with the same sample
.   frequency used by the analog input system.
-AOAMPLITUDE
.   The Analog Output Amplitude indicates the size of the signal.  Amplitude
.   is specified in 1/2 peak-to-peak, so that the signal maximum will be the
.   AOOFFSET + AOAMPLITUDE, and the signal minimum will be 
.   AOOFFSET - AOAMPLITUDE.
.   Legal maximums must be below 5V and minimums must be greater than .01V.
-AOOFFSET
.   The Analog Output Offset indicates the dc offset of the signal.
.   See AOAMPLITUDE for more details.
-AODUTY
.   The Analog Output Duty cycle is used for square and triangle waves to 
.   specify a high-low-asymmetry (duty cycle).  It is ignored for sine waves.
.   Legal values are floating point values between and including 0.0 and 1.0.
.   A duty cycle of 0.75 will cause a square wave to be high for 3/4 of its
.   period.  For a triangle wave, the duty cycle indicates the percentage of
.   the period that will be spent rising, so a sawtooth wave can be created
.   with AODUTY values of 0.0 and 1.0.
-TRIGCHANNEL
.   Which analog input should be monitored to generate the software trigger?
.   This non-negative integer does NOT specify the physical analog channel. It
.   specifies which of the analog measurements should be monitored for a 
.   trigger.
.
.   For fast digital pulses, LCONFIG can be configured to watch the high speed
.   counter 2 by passing "HSC" instead of a trigger channel.  If the value in 
.   the counter increases, then a trigger event will be registered.
.
.   The read_data_stream() function is responsible for monitoring the number of
.   samples and looking for a trigger.  There are four trigger states indicated
.   by the TRIGSTATE member of the lc_devconf_t structure.
.       TRIG_IDLE - the trigger is inactive and data will be collected as 
.                   normal.
.       TRIG_PRE - data collection has begun recently, and TRIGPRE samples have 
.                   not yet been collected.  No trigger is allowed yet.
.       TRIG_ARMED - The pre-trigger buffer has been satisfied and 
.                   read_data_stream() is actively looking for a trigger. Data
.                   should continue streaming to a pre-trigger buffer.
.       TRIG_ACTIVE - A trigger event has been found, and normal data collection
.                   has begun.
.   read_data_stream() is responsible for maintaining this state variable, but
.   it does NOT deal with pre-trigger buffers automatically. read_file_stream()
.   does.
-TRIGLEVEL
.   The voltage threshold on which to trigger.  Accepts a floating point.
-TRIGEDGE
.   Accepts "rising", "falling", or "all" to describe which
-TRIGPRE
.   Pretrigger sample count.  This non-negative integer indicates how many 
.   samples should be collected on each channel before a trigger is allowed.
-EFFREQUENCY
.   Sets the frequency scale for all EF extended features.  This parameter
.   determines how often the EF clock updates.  Pulse and PWM outputs will
.   exhibit this frequency.  Unlike all other EF parameters, the frequency
.   is set globally, so there is only one EFFREQUENCY parameter, rather than
.   one for each channel.
-EFCHANNEL
.   Like the AOCHANNEL and AICHANNEL, this determines which of the EF channels
.   is being set.  Valid values are 0-7, but some features are not implemented
.   on all EF channels.  Check the T7 manual Digital IO section for details.
-EFLABEL
.   This optional string can be used to label the EF channel like AILABEL
.   or the other labels.
-EFSIGNAL
.   Sets the signal type used for this channel.  The following is a list of the
.   supported signal types and how their settings are handled.  Measurements
.   are collected by executing the update_ef() function and checking the 
.   member efch[] member variables, duty, counts, or time.  Their
.   meaning is listed below each signal type.
. * PWM - pulse width
.       Input: valid channels 0,1
.           EFEDGE - "rising" indicates duty cycle high, "falling" inverts.
.           duty - Holds the measured duty cycle
.           counts - Holds the signal period in counts
.           time - Holds the signal period in usec
.       Output: valid channels 0,2,3,4,5
.           EFFREQUENCY - Determines the waveform frequency
.           EFEDGE - "rising" indicates duty cycle high, "falling" invergs.
.           EFDUTY - Number 0-1 indicating % time high (low in "falling" mode).
.           EFDEGREES - Phase of the PWM waveform in degrees.
. * COUNT - edge counter
.       Input: valid channels 0,1,2,3,6,7
.           EFEDGE - Count rising, falling, or ALL edges.
.           EFDEBOUNCE - What debounce filter to use?
.           EFUSEC - What debounce interval to use?
.           counts - Holds the measured count.
.       Output: Not supported
. * FREQUENCY - period or frequency tracker
.       Input: valid channels 0,1
.           EFUSEC - Holds the measured period.
.       Output: Not supported
. * PHASE - Delay between edges on two channels
.       Input: valid channels are in pairs 0/1 (only one needs to be specified)
.           EFEDGE - rising/falling
.           EFUSEC - Holds the measured delay between edges
.       Output: Not supported
. * QUADRATURE - Two-channel encoder quadrature
.       Input: valid channels are in pairs 0/1, 2/3, 6/7
.       (only one needs to be specified)
.           EFCOUNT - Holds the measured encoder count
.       Output: Not supported
-EFEDGE
.   Used by the frequency and phase signals, this parameter determines whether
.   rising, falling, or both edges should be counted.  Valid values are "all",
.   "rising", or "falling".
-EFDEBOUNCE
.   Used by a COUNT input signal, this parameter indicates what type of filter
.   (if any) to emply to clean noisy (bouncy) edges.  The T7 does not implement
.   all of these filters on all edge types.  Valid edge types are listed in 
.   parentheses after each description.  Valid values are:
.       none - all qualifying edges are counted (rising, falling, all)
.       fixed - ignore all edges a certain interval after an edge 
.               (rising, falling)
.       reset - like fixed, but the interval resets if an edge occurrs during
.               the timeout period. (rising, falling, all)
.       minimum - count only edges that persist for a minimum interval
.               (rising, falling)
.   The intervals specified in these debounce modes are set by the EFUSEC
.   parameter.
-EFDIRECTION
.   Valid parameters "input" or "output" determine whether the signal should
.   be generated or measured.  EF outputs cannot be streamed, but are set
.   statically.
-EFUSEC
.   Specifies a time in microseconds.
-EFDEGREES
.   Specifies a phase angle in degrees.
-EFDUTY
.   Accepts a floating value from zero to one indicating a PWM duty cycle.
-EFCOUNT
.   An unsigned integer counter value.
-COMCHANNEL
.   Like the AICHANNEL, AOCHANNEL, and EFCHANNEL parameter, the COMCHANNEL 
.   signals the creation of a new communications channel, but unlike the other
.   scopes, COMCHANNELS do not specify a channel number, but a channel type.
.   Valid values are:
. * UART - Universal Asynchronous Receive-Transmit
. * 1WIRE - 1 Wire communication (NOT YET SUPPORTED)
. * SPI - Serial-periferal interface (NOT YET SUPPORTED)
. * I2C - Inter Integrated Circuit (pronounced "eye-squared-see") (NOT YET SUPPORTED)
. * SBUS - Sensiron Bus I2C derivative (NOT YET SUPPORTED)
.   
.   The interpretation of the COM parameters depends on which protocol is being
.   used.  For example, 1-wire and I2C do not require separate RX and TX lines
.   and the UART does not require a clock.
-COMRATE
.   The requested data rate in bits per second.  The available bit rates vary 
.   with the different COM interfaces.
.   * UART: 9600 is default, but LJ recommends < 38,400
-COMLABEL
.   This is a string label to apply to the COM channel.
-COMOUT
.   The data output line expects an integer DIO pin number.
.   * UART: This will be the TX pin.
-COMIN
.   The data input line expects an integer DIO pin number.
.   * UART: This will be the RX pin.
-COMCLOCK
.   The data clock line expects an integer DIO pin number.
.   * UART: This parameter is unusued.
-COMOPTIONS
.   The communication options is a coded string that defines the mode of the 
.   selected protocol.
.   * UART: "8N1" BITS PARITY STOP
.       The UART options string must be three characters long
.       BITS:   8 or fewer.  (no 9-bit codes)
.       PARITY: (N)one, (P)ositive, (O)dd, others are not supported.
.       STOP:   Number of stop bits 0, 1, or 2.
-META
.   The META keyword begins or ends a stanza in which meta parameters can be 
.   defined without a type prefix (see below).  Meta parameters are free entry
.   parameters that are not directly understood by LCONFIG functions, but that
.   the application program might use, or that the user may want recorded in
.   a data file.  Once a meta stanza is begun, any parameter name that is not 
.   recognized as a built-in parameter is used to create a new meta parameter.
.   The type of the stanza determines which data type the new parameter will
.   hold.  The meta parameter accepts one of nine string values; str, string, 
.   int, integer, flt, float, end, none, and stop.
.
.   meta int  OR  meta integer
.       starts a stanza in which any unrecognized parameter names are used to
.       create integer meta parameters.
.   meta flt  OR  meta float
.       starts a stanza in which any unrecognized parameter names are used to
.       create floating point meta parameters.
.   meta str  OR  meta string
.       starts a stanza in which any unrecognized parameter names are used to
.       create string meta parameters.
.   meta end  OR  meta stop  OR  meta none
.       ends the meta stanza so that unrecognized parameter names will cause an
.       error.  
.
.   Adding a "meta end" pair to the end of a meta stanza is not strictly 
.   necessary, but it is STRONGLY recommended.  Defining non-meta parameters in 
.   a meta stanza is allowed, but STRONGLY discouraged.  Configuration file 
.   authors should ALWAYS avoid a situation where a typo in an important 
.   parameter creates a new parameter instead of throwing an error.
.
.   For example...
.       meta integer
.           cats 9
.           dogs 8
.           teeth 606
.       meta float
.           weightkg 185.6
.       meta end
.
-INT:META
-FLT:META
-STR:META
.   Meta configuration parameters are custom parameters that are not natively
.   understood by LCONF, but are recorded in the configuration file anyway.
.   The parent application may access them and use them to control the process,
.   or they may simply be a way to log notes in a data file.
.   Because LCONF has no knowledge of these parameters, it doesn't know how
.   to parse them; as integers, floats, or strings.  Worse still, a minor key-
.   stroke error while typing a normal parameter might be mistaken as a meta
.   parameter.
.   Both of these problems are addressed by preceeding the parameter with a 
.   three-letter prefix and a colon.  For exmaple, an integer parameter named
.   "year" would be declared by
.       int:year 2016
.   Once established, it can be accessed using the get_meta_int or put_meta_int
.   functions.
        int year;
.       get_meta_int(dconf,"year",&year);
*/
int lc_load_config(lc_devconf_t* dconf,         // array of device configuration structs
                const unsigned int devmax, // maximum number of devices to load
                const char* filename);  // name of the file to read


/* WRITE_CONFIG
Write the configuration parameters set in DCONF to a configuration
file identified by filename.  The append flag indicates whether the file should
be overwritten or appended with the parameters.  In order to save multiple
device configurations to the same file, the append flag should be set.
*/
void lc_write_config(lc_devconf_t* dconf, FILE* ff);



/* OPEN_CONFIG
When dconf is an array of device configurations loaded by load_config().  DCONF
is a pointer to the device configuration struct corresponding to the device 
connection to open.
*/
int lc_open(lc_devconf_t* dconf);



/* CLOSE_CONFIG
Closes the connection to device DCONF.
*/
int lc_close(lc_devconf_t* dconf);


/*UPLOAD_CONFIG
Presuming that the connection has already been opened by open_config(), this
function uploads the appropriate parameters to the respective registers of 
device devnum in the dconf array.
*/
int lc_upload_config(lc_devconf_t* dconf);


/*SHOW_CONFIG
Prints a display of the parameters configured in DCONF.
*/
void lc_show_config(lc_devconf_t* dconf);


/*GET_META_XXX
From the "devnum" device configuration, retrieve a meta parameter of the specified type.
If the type is called incorrectly, the function will still return a value, but it will
be the giberish data that you get when you pull from a union.
If the parameter is not found, get_meta_XXX will return an error.  Parameters are not called
with their type prefixes.  For example, a parameter declared in a config file as 
    int:year 2016
can be referenced by 
    int year;
    get_meta_int(dconf,devnum,"year",&year);
*/
int lc_get_meta_int(lc_devconf_t* dconf, const char* param, int* value);
int lc_get_meta_flt(lc_devconf_t* dconf, const char* param, double* value);
int lc_get_meta_str(lc_devconf_t* dconf, const char* param, char* value);

/*PUT_META_XXX
Write to the meta array.  If the parameter already exists, the old data will be overwritten.
If the parameter does not exist, a new meta entry will be created.  If the meta array is
full, put_meta_XXX will return an error.
*/
int lc_put_meta_int(lc_devconf_t* dconf, const char* param, int value);
int lc_put_meta_flt(lc_devconf_t* dconf, const char* param, double value);
int lc_put_meta_str(lc_devconf_t* dconf, const char* param, char* value);


/*UPDATE_EF
Refresh the extended features registers.  All lc_devconf_t EF parameters will be 
rewritten to their respective registers and measurements will be downloaded into
the appropriate EF channel member variables.

The clock frequency will NOT be updated.  To change the EFfrequency parameter,
upload_config() should be re-called.  This will halt acquisition and re-start 
it.
*/
int lc_update_ef(lc_devconf_t* dconf);

/*COMMUNICATE
Executes a read/write operation on a digital communication channel.  The 
COMCHANNEL is the integer index of the configured COMCHANNEL.  The TXBUFFER is
an array to transmit over the channel of length TXLENGTH.  The RXBUFFER is an
array of data to listen for with length RXLENGTH.

If positive, the TIMEOUT_MS is the time in milliseconds to wait for a reply 
before rasing an error.  If TIMEOUT_MS is negative, then COMMUNICATE will 
immediately respond with whatever data were waiting.  If TIMEOUT_MS is precisely
zero, then COMMUNICATE will wait indefinitely until RXLENGTH bytes of data are
available.  Otherwise, TIMEOUT_MS is the number of milliseconds to wait for 
RXLENGTH bytes to arrive.  If RXLENGTH is zero, then the read operation will be
skipped altogether.

In UART mode, the applicaiton should only read and write to odd-indexed values
of the buffers.  The LJM interface encodes UART data in 16-bit registers padded
with zeros, so the even-indices are neither transmitted nor received.

Returns -1 if there is an error.

For non-blocking listening to a COM channel, see the LC_COM_LISTEN and 
LC_COM_READ functions.
*/
int lc_communicate(lc_devconf_t* dconf, const unsigned int comchannel,
        const char *txbuffer, const unsigned int txlength, 
        char *rxbuffer, const unsigned int rxlength,
        const int timeout_ms);
        

/*COM_START
COM_READ
COM_WRITE
COM_STOP

The START, READ, WRITE, and STOP operations control a COM interface.

The LC_COM_START function uploads the configuration for the COM channel and 
enables the feature.  The LJM receive buffer is configured to have the size
RXLENGTH in bytes.  For UART, this should be twice the number of bytes the 
application plans to transmit because the LJM UART interface uses 16-bit-wide
buffers.  The other COM interfaces use byte buffers, so the same deliberate 
oversizing is not needed.  Returns -1 on failure and 0 on success.

The LC_COM_STOP funciton disables the COM interface without reading or writing
to it.  Returns a -1 on failure and 0 on success.

The LC_COM_WRITE function transmits the contents of the TXBUFFER over an active
COM interface.  TXLENGTH is the number of bytes in the TXBUFFER array.  For UART
interfaces, only odd indices of the TXBUFFER should be used since the LJM UART
interface uses 16-bit wide buffers, and the most significant 8-bits are ignored.
Returns -1 on failure and 0 on success.

The LC_COM_READ function reads from the COM receive buffer into the RXBUFFER 
array up to a maximum of RXLENGTH bytes.  Reading can be done in one of three 
modes depending on the value passed to TIMEOUT_MS.  When TIMEOUT_MS is positive,
LC_COM_READ will wait until exactly RXLENGTH bytes of data are available or 
until TIMEOUT_MS milliseconds have passed.  When TIMEOUT_MS is negative, 
LC_COM_READ will read any data available and return immediately.  When 
TIMEOUT_MS is zero, LC_COM_READ will wait indefinitely for the data to become
available.  Unlike the WRITE, START, and STOP funcitons, the LC_COM_READ 
funciton returns the number of bytes read on success, and -1 on failure. 
*/
int lc_com_start(lc_devconf_t* dconf, const unsigned int comchannel,
        const unsigned int rxlength);
        
int lc_com_stop(lc_devconf_t* dconf, const unsigned int comchannel);
        
int lc_com_write(lc_devconf_t* dconf, const unsigned int comchannel,
        const char *txbuffer, const unsigned int txlength);
        
int lc_com_read(lc_devconf_t* dconf, const unsigned int comchannel,
        char *rxbuffer, const unsigned int rxlength, int timeout_ms);

/*LC_STREAM_STATUS
Report on a data stream's status
CHANNELS - number of analog input channels streaming
SAMPLES_PER_READ - number of samples per channel in each read/write block
SAMPLES_STREAMED - a running total of the samples per channel streamed into the
                    buffer from the T7
SAMPLES_READ - a running total of the samples per channel read from the buffer.
SAMPLES_WAITING - the number of samples waiting in the buffer

To determine the total number of samples in a read/write block, calculate
    CHANNELS * SAMPLES_PER_READ
*/
void lc_stream_status(lc_devconf_t* dconf, 
        unsigned int *samples_streamed, unsigned int *samples_read,
        unsigned int *samples_waiting);

/* LC_STREAM_ISCOMPLETE
Returns 1 to indicate that at least dconf[devnum].nsample samples per channel
have been streamed from the T7.  Returns a 0 otherwise.
*/
int lc_stream_iscomplete(lc_devconf_t* dconf);

/* LC_STREAM_ISEMPTY
Returns 1 to indicate that all data in the buffer has been read.  Returns a 0
otherwise.
*/
int lc_stream_isempty(lc_devconf_t* dconf);

/* LC_STREAM_START
Start a stream operation based on the device configuration for device devnum.
samples_per_read can be used to override the NSAMPLE parameter in the device
configuration.  If samples_per_read is <= 0, then the LCONF_SAMPLES_PER_READ
value will be used instead.
*/
int lc_stream_start(lc_devconf_t* dconf,   // Device array and number
            int samples_per_read);    // how many samples per call to read_data_stream


/*LC_STREAM_SERVICE
Read data from an active stream on the device devnum.  The SERVICE_DATA_STREAM
function also services the software trigger state.

Data are read directly from the T7 into a ring buffer in the lc_devconf_t struct.  
Until data become available, READ_DATA_STREAM will return NULL data arrays,
but once valid data are ready, READ_DATA_STREAM returns pointers into this
ring buffer.
*/
int lc_stream_service(lc_devconf_t* dconf);

/*LC_STREAM_READ
If data are available on the data stream on device number DEVNUM, then 
READ_DATA_STREAM will return a DATA pointer into a buffer that contains data
ready for use.  The amount of data available is indicated by the CHANNELS
and SAMPLES_PER_READ integers.  The total length of the data array will be 
CHANNELS * SAMPLES_PER_READ.

The measurements of each scan are arranged in data[] by channel number.  When 
three channels are included in each measurement, the data array would appear 
something like this
data[0]     sample0, channel0
data[1]     sample0, channel1
data[2]     sample0, channel2
data[3]     sample1, channel0
data[4]     sample1, channel1
data[5]     sample1, channel2
... 

Once READ_DATA_STREAM has been called, the lc_devconf_t struct updates its internal
pointers, and the next call's DATA register will either point to the next 
available block of data, or it will be NULL if the buffer is empty.

It is essential that the application not attempt to read more than CHANNELS *
SAMPLES_PER_READ data elements.  The following data might be valid, but they 
also might wrap around the end of the ring buffer.  Instead, READ_DATA_STREAM
should be called again to return a new valid data pointer.

For example, the following might appear in a loop 

int err;
unsigned int channels, samples_per_read, index;
double my_buffer[2048];
double *pointer;
... setup code ... 
err = service_data_stream(dconf, 0);
err = read_data_stream(dconf, 0, &data, &channels, &samples_per_read);
if(data){
    for(index=0; index<samples_per_read*channels; index++)
        my_buffer[index] = data[index];
}
*/
int lc_stream_read(lc_devconf_t* dconf, double **data, 
        unsigned int *channels, unsigned int *samples_per_read);

/*STOP_STREAM
Halt an active stream on device devnum.
*/
int lc_stream_stop(lc_devconf_t* dconf);

/*
.
.   File Streaming - shifting data directly to a data file
.
*/

/*LC_DATAFILE_INIT
Writes the configuration header and timestamp to a data file.  This should
be called prior to write_data_file()
*/
int lc_datafile_init(lc_devconf_t* dconf, FILE *FF);


/*LC_DATAFILE_WRITE
Streams data from the dconf buffer to an open data file.  WRITE_DATA_FILE calls
READ_DATA_STREAM to pull data from the device ring buffer.  The buffer will
be empty unless there are also prior calls to SERVICE_DATA_STREAM.
*/
int lc_datafile_write(lc_devconf_t* dconf, FILE *FF);




#endif
