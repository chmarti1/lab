/*
.
.   Tools for loading laboratory configuration files
.
.   (c) 2016
.   Released under GPLv3.0
.   Chris Martin
.   Assistant Professor of Mechanical Engineering
.   Penn State University, Altoona College
.
.   Tested on the Linux kernal 3.19.0
.   Linux Mint 17.3 Rosa
.   LJ Exoderiver v2.5.3
.   LJM Library 1.13
.   T7 Firmware 1.0188
.   LibUSB 2:0.1.12
.
*/


/*
Use the commands below to compile your code in a *nix environment
You're on your own in Windows.  If you're in Windows, just about
everything should work, but the show_config() function may give
strange results.

$gcc your_code.c -lm -lLabJackM -o your_exec.bin
$chmod a+x your_exec.bin
*/

#ifndef __LCONFIG
#define __LCONFIG

#include <stdio.h>
#include <LabJackM.h>


#define LCONF_VERSION 2.02   // Track modifications in the header
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
Removed calibration information from the AICONF struct
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
Added ndev_config()
*/

#define TWOPI 6.283185307179586

#define LCONF_MAX_STR 32    // longest supported string
#define LCONF_MAX_READ "%32s"
#define LCONF_MAX_META 32   // how many meta parameters should we allow?
#define LCONF_MAX_STCH 15   // maximum number of streaming channels allowed (LCONF_MAX_NAICH + LCONF_MAX_NAOCH)
#define LCONF_MAX_AOCH 1    // maximum analog output channel number allowed
#define LCONF_MAX_NAOCH 2   // maximum analog output channels to allow
#define LCONF_MAX_AICH 13   // highest analog input channel number allowed
#define LCONF_MAX_NAICH 14  // maximum analog input channels to allow
#define LCONF_MAX_AIRES 8   // maximum resolution index
#define LCONF_MAX_NDEV 32   // catch runaway cases if the user passes junk to devmax
#define LCONF_MAX_AOBUFFER  512     // Maximum number of buffered analog outputs
#define LCONF_BACKLOG_THRESHOLD 1024 // raise a warning if the backlog exceeds this number.

// macros for writing lines to configuration files in WRITE_CONFIG()
#define write_str(param,child) if(dconf[devnum].child[0]!='\0'){fprintf(ff, "%s %s\n", #param, dconf[devnum].child);}
#define write_int(param,child) fprintf(ff, "%s %d\n", #param, dconf[devnum].child);
#define write_flt(param,child) fprintf(ff, "%s %f\n", #param, dconf[devnum].child);
#define write_aistr(param,child) if(dconf[devnum].aich[ainum].child[0]!='\0'){fprintf(ff, "%s %s\n", #param, dconf[devnum].aich[ainum].child);}
#define write_aiint(param,child) fprintf(ff, "%s %d\n", #param, dconf[devnum].aich[ainum].child);
#define write_aiflt(param,child) fprintf(ff, "%s %f\n", #param, dconf[devnum].aich[ainum].child);
#define write_aostr(param,child) if(dconf[devnum].aoch[aonum].child[0]!='\0'){fprintf(ff, "%s %s\n", #param, dconf[devnum].aoch[aonum].child);}
#define write_aoint(param,child) fprintf(ff, "%s %d\n", #param, dconf[devnum].aoch[aonum].child);
#define write_aoflt(param,child) fprintf(ff, "%s %f\n", #param, dconf[devnum].aoch[aonum].child);
// macro for testing strings
#define streq(a,b) strncmp(a,b,LCONF_MAX_STR)==0

#define LCONF_COLOR_RED "\x1b[31m"
#define LCONF_COLOR_GREEN "\x1b[32m"
#define LCONF_COLOR_YELLOW "\x1b[33m"
#define LCONF_COLOR_BLUE "\x1b[34m"
#define LCONF_COLOR_MAGENTA "\x1b[35m"
#define LCONF_COLOR_CYAN "\x1b[36m"
#define LCONF_COLOR_NULL "\x1b[0m"

#define LCONF_DEF_NSAMPLE 64
#define LCONF_DEF_AI_NCH 199
#define LCONF_DEF_AI_RANGE 10.
#define LCONF_DEF_AI_RES 0
#define LCONF_DEF_AO_AMP 1.
#define LCONF_DEF_AO_OFF 2.5
#define LCONF_DEF_AO_DUTY 0.5

#define LCONF_NOERR 0
#define LCONF_ERROR 1



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
typedef struct aiconf {
    unsigned int    channel;     // channel number (0-13)
    unsigned int    nchannel;    // negative channel number (0-13 or 199)
    double          range;       // bipolar input range (0.01, 0.1, 1.0, or 10.)
    unsigned int    resolution;  // resolution index (0-8) see T7 docs
} AICONF;
//  The calibration and zero parameters are used by the aical function

// Define the types of analog outputs supported
enum AOSIGNAL {CONSTANT, SINE, SQUARE, TRIANGLE, NOISE};

// Analog output configuration
// This includes everything we need to know to construct a signal for output
typedef struct aoconf {
    unsigned int    channel;      // Channel number (0 or 1)
    enum AOSIGNAL   signal;       // What function type is being generated?
    double          amplitude;    // How big?
    double          frequency;    // Signal frequency in Hz
    double          offset;       // What is the mean value?
    double          duty;         // Duty cycle for a square wave or triangle wave
                                  // duty=1 results in all-high square and an all-rising triangle (sawtooth)
} AOCONF;


// The METACONF is a struct for user-defined flexible parameters.
// These are not used to configure the DAQ, but simply data of record
// relevant to the measurement.  They may be needed by the parent program
// they could hold calibration information, or they may simply be a way
// to make notes about the experiment.
typedef struct metaconf {
    char param[LCONF_MAX_STR];      // parameter name
    union {
        char svalue[LCONF_MAX_STR];
        int ivalue;
        double fvalue;
    } value;                        // union for flexible data types
    char type;                      // reminder of what type we have
} METACONF;


typedef struct filestream {
    FILE* file;                     // The open data file
    double* buffer;                 // A buffer for reading in data
    unsigned int samples_per_read;  // The size of the buffer
} FILESTREAM;


typedef struct devconf {
    int connection;                 // connection type index
    char ip[LCONF_MAX_STR];         // ip address string
    char gateway[LCONF_MAX_STR];    // gateway address string
    char subnet[LCONF_MAX_STR];     // subnet mask string
    char serial[LCONF_MAX_STR];     // serial number string
    AICONF aich[LCONF_MAX_NAICH];    // analog input configuration array
    unsigned int naich;             // number of configured analog input channels
    AOCONF aoch[LCONF_MAX_NAOCH];   // analog output configuration array
    unsigned int naoch;             // number of configured analog output channels
    int handle;                     // device handle
    double samplehz;                // *sample rate in Hz
    double settleus;                // *settling time in us
    unsigned int nsample;           // *number of samples per read
    METACONF meta[LCONF_MAX_META];  // *meta parameters
    FILESTREAM FS;                  // active file stream?
} DEVCONF;





/*
.
.   Prototypes
.
*/


/* NDEV_CONFIG
Return the number of configured device connections in a DEVCONF array.
Counts until it finds a device with a connection index less than 0 (indicating
that it was never configured) or until the devmax limit is reached.  That means
that NDEV_CONFIG will be fooled if DEVCONF elements are not configured 
sequentially.  Fortunately, load_config always works sequentially.
*/
int ndev_config(DEVCONF* dconf, // Array of device configuration structs
                const unsigned int devmax); // maximum number of devices allowed

/* NISTREAM_CONFIG
Returns the number of input stream channels configured. These will be the number 
of columns of data discovered in the data when streaming.
*/
int nistream_config(DEVCONF* dconf, const unsigned int devnum);

/* LOAD_CONFIG
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
Parameters are not case sensitive.  The following parameters are recognized:
-CONNECTION
.   Expects a string "eth", "usb", or "any" to specify the type of connection
.   The connection parameter flags the creation of a new connection to 
.   configure.  Every parameter-value pair that follows will be applied to the
.   preceeding connection.  As a result, the connection parameter must come 
.   before any other parameters.
-SERIAL
.   Identifies the device by its serial number. 
-IP
.   The static IP address to use for an ethernet connection.  If the connection
.   is ETH, then the ip address will be given precedence over the serial
.   number if both are specified.  If the connection is USB, and a valid
.   IP address is still specified, the value will written.  The same is true
.   for the GATEWAY and SUBNET parameters.
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
-AOCHANNEL
.   This parameter indicates the channel number to be configured for cyclic 
.   dynamic output (function generator).  This will be used to generate a 
.   repeating analog output function while a stream operation is active.
.   All AO (analog output) configuration words that are encountered after
.   this parameter will be applied to this channel.  As a result, it the 
.   AOCH paramter must appear before any AO configuration parameters.
-AOSIGNAL
.   The Analog Output Signal is the type of signal to be generated.  Valid
.   options are CONSTANT, SINE, SQUARE, and TRIANGLE.  In order to generate 
.   a sawtooth wave, select TRIANGLE, and adjust the duty cycle to 1 or 0.  
.   See AODUTY.
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
.       get_meta_int(dconf,devnum,"year",&year);
*/
int load_config(DEVCONF* dconf,         // array of device configuration structs
                const unsigned int devmax, // maximum number of devices to load
                const char* filename);  // name of the file to read


/* WRITE_CONFIG
Write the configuration parameters from device devnum of the dconf array to a 
file identified by filename.  The append flag indicates whether the file should
be overwritten or appended with the parameters.  In order to save multiple
device configurations to the same file, the append flag should be set.
*/
void write_config(DEVCONF* dconf, const unsigned int devnum, FILE* ff);



/* OPEN_CONFIG
When dconf is an array of device configurations loaded by load_config(), this
function will open a connection to the device at index "devnum", and the 
handle will be written to that device's "handle" field.
*/
int open_config(DEVCONF* dconf,     // device configuration 
                const unsigned int devnum); // devices index to open



/* CLOSE_CONFIG
Closes the connection to device devnum in the dconf array.
*/
int close_config(DEVCONF* dconf,     // device configuration array
                const unsigned int devnum); // device to close


/*UPLOAD_CONFIG
Presuming that the connection has already been opened by open_config(), this
function uploads the appropriate parameters to the respective registers of 
device devnum in the dconf array.
*/
int upload_config(DEVCONF* dconf, const unsigned int devnum);


/*DOWNLOAD_CONFIG
This function builds a dummy configuration struct corresponding to device
devnum in the dconf array.  The values in that struct are based on the values
found in the corresponding registers of the live device.  Some parameters are
not available for download (like naich and the ai channel numbers).  Instead,
those are used verbatim to identify how many and which channels to investigate
on the live device.
*/
int download_config(DEVCONF* dconf, const unsigned int devnum, DEVCONF* out);


/*SHOW_CONFIG
This function calls download_config() and displays a color-coded comparison
between the configuration in dconf and the live device configuration for the
device identified by devnum.
*/
void show_config(DEVCONF* dconf, const unsigned int devnum);


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
int get_meta_int(DEVCONF* dconf, const unsigned int devnum, const char* param, int* value);
int get_meta_flt(DEVCONF* dconf, const unsigned int devnum, const char* param, double* value);
int get_meta_str(DEVCONF* dconf, const unsigned int devnum, const char* param, char* value);

/*PUT_META_XXX
Write to the meta array.  If the parameter already exists, the old data will be overwritten.
If the parameter does not exist, a new meta entry will be created.  If the meta array is
full, put_meta_XXX will return an error.
*/
int put_meta_int(DEVCONF* dconf, const unsigned int devnum, const char* param, int value);
int put_meta_flt(DEVCONF* dconf, const unsigned int devnum, const char* param, double value);
int put_meta_str(DEVCONF* dconf, const unsigned int devnum, const char* param, char* value);


/*START_DATA_STREAM
Start a stream operation based on the device configuration for device devnum.
samples_per_read can be used to override the NSAMPLE parameter in the device
configuration.  If samples_per_read is <= 0, then the NSAMPLE configuration 
value will be used instead.
*/
int start_data_stream(DEVCONF* dconf, const unsigned int devnum,  // Device array and number
            int samples_per_read);    // how many samples per call to read_data_stream

/*READ_DATA_STREAM
Read in data from an active stream on device devnum.  This is little more than
a wrapper for the LJM_eReadStream function.  Data from device devnum are read
into the data array.

The measurements of each scan are ordered in data[] by channel number.  When 
three channels are included in each measurement, the data array would appear 
something like this
data[0]     scan0, channel0
data[1]     scan0, channel1
data[2]     scan0, channel2
data[3]     scan1, channel0
data[4]     scan1, channel1
data[5]     scan1, channel2
... 

The write_data() function disentangles these samples automatically and writes an
ASCII file that can be loaded into Matlab, Excel, Python, or your analysis 
software of choice.
*/
int read_data_stream(DEVCONF* dconf, const unsigned int devnum, double *data);

/*STOP_STREAM
Halt an active stream on device devnum.
*/
int stop_data_stream(DEVCONF* dconf, const unsigned int devnum);

/*
.
.   File Streaming - shifting data directly to a data file
.
*/

/*CLEAN_FILE_STREAM
This functions "cleans up" after a file streaming operation.  The file is closed
the dynamic buffer memory is returned, and all parameters are forced to zero or
NULL to indicate that the file stream is inactive.

clean_file_stream() cannot fail, and so returns no error.
*/
void clean_file_stream(FILESTREAM* FS);

/*START_FILE_STREAM
This high-level operation calls on several other functions to:
1) Open filename in create/overwrite mode
2) Allocate a buffer array
3) Write the device configuration as a header
4) Start a data stream
5) Write the date and time the stream was started to the data file

The samples_per_read parameter can be used to override the NSAMPLE configuration
parameter.  If samples_per_read is <= 0, then the NSAMPLE parameter will be used
instead.
*/
int start_file_stream(DEVCONF* dconf, const unsigned int devnum,
            int samples_per_read, const char *filename);


/*READ_FILE_STREAM
Similar to read_data_stream(), this function actually calls its namesake to
read data into the FS buffer.  read_file_stream() performs the additional
task of formatting and printing data to the data file.
*/
int read_file_stream(DEVCONF* dconf, const unsigned int devnum);

/*STOP_FILE_STREAM
1) Calls stop_data_stream() to halt the data acquisition process
2) Calls clean_file_stream() to close the file and free the buffer.
*/
int stop_file_stream(DEVCONF* dconf, const unsigned int devnum);






/*HELPER FUNCTIONS
These are functions not designed for use outside the header
*/

// force a string to lower case
void strlower(char* target);

// calculate analog input configuration register addresses
void airegisters(const unsigned int channel, int *reg_negative, int *reg_range, int *reg_resolution);

// calculate analog output configuration register addresses
void aoregisters(const unsigned int buffer,     // The output stream number (not the DAC number)
                int *reg_target,                // STREAM_OUT#_TARGET (This should point to the DAC register) 
                int *reg_buffersize,            // STREAM_OUT#_BUFFER_SIZE (The memory to allocate to the buffer)
                int *reg_enable,                // STREAM_OUT#_ENABLE (Is the output stream enabled?)
                int *reg_buffer,                // STREAM_OUT#_BUFFER_F32 (This is the destination for floating point data)
                int *reg_loopsize,              // STREAM_OUT#_LOOP_SIZE (The number of samples in the loop)
                int *reg_setloop);              // STREAM_OUT#_SET_LOOP (how to interact with the loop; should be 1)

// print a formatted line with color flags for mismatching values
// prints the following format: printf("%s: %s (%s)\n", head, value1, value2)
void print_color(const char* head, char* value1, char* value2);

// Read the next word from a configuration file while ignoring comments
void read_param(FILE* source, char* param);



/* INIT_CONFIG
Initialize a configuration struct to safe default values
*/
void init_config(DEVCONF *dconf);


#endif
