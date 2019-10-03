[back](documentation.md)

Version 4.00<br>
September 2019<br>
Chris Martin<br>

##  <a name='top'></a> The LConfig API
If you want to use the LConfig system, but you don't just want to use the canned drun or dburst codes, you'll need to know the LConfig API.  The lconfig.h header exposes functions for 
- [Interacting with configuration files](#config)
- [Interacting with devices](#dev)
- [Performing diagnostics](#diag)
- [Data collection](#data)
- [Meta configuration](#meta)

It is important to remember that LConfig is just a layer built on top of LabJack's interface to automate the device configuration, to handle software triggering, and to handle data buffering.

The typical steps for writing an applicaiton that uses LConfig are: <br>
** (1) Load a configuration file ** with `lc_load_config()`.  The configuratino file will completely define the data acquisition operation by specifying which channels should be configured, how much data should be streamed at what rate, and how software triggering should be done.
** (2) Open the device ** with `lc_open()`.  After loading the configuration file, the device configuration struct will contain all the information needed to find the correct device.
** (3) Upload the configuration ** with `lc_upload_config()`.  Most (but not all) of the directives in the configuration file are enforced in this step.  AI resolution, analog output streaming, and trigger settings are not enforced until a stream is started.
** (4) Start a data stream ** with `lc_stream_start()`.  This enforces the last of the configuration parameters and initiates the stream of data.
** (5) Service the data stream ** with `lc_stream_service()`.  The data service function moved the newly available data into a ring buffer and (if so configured) scans for a trigger event.  This funciton is responsible for maintaining the trigger state registers in the device configuration struct.
** (6) Read data ** with `lc_stream_read()`.  When data are available, this funciton returns a pointer into the ring buffer, where the next block of data may be read.  If no data are available, either because a trigger has not occurred or because the service function has not yet completed, the pointer is NULL.  Separating service and read operations permits applications to quckly stream lots of data and read it later for speed.
** (7) Stop the stream ** with `lc_stream_stop()`.  This halts the data collection process, but does NOT free the data buffer.  Some applications (like `lcburst`) may want to quickly stream data to memory and read it later when the acquisition process is done.  This allows steps 7 and 6 to be reversed without consequence.
** (8) Close the device connection ** with `lc_close()`.  Closing a connection automatically calls `clear_buffer()`, which frees the ring buffer memory and dumps any data not read.

### <a name='config'></a> Interacting with configuration files

```C
int lc_load_config(       lc_devconf_t* dconf, 
                const unsigned int devmax, 
                       const char* filename)
```
`lc_load_config` loads a configuration file into an array of `lc_devconf_t` structures.  Each instance of the `connection` parameter signals the configuration a new device.  `dconf` is an array of `lc_devconf_t` structs with `devmax` elements that will contain the configuration parameters found in the data file, `filename`.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
void lc_write_config(     lc_devconf_t* dconf, 
                             FILE* ff)
```
`write_config` writes a configuration stanza to an open file, `ff`, for the parameters of device configuration struct pointed to by `dconf`.  A `lc_devconf_t` structure written by `lc_write_config` should result in an identical structure after being read by `lc_load_config`.

Rather than accept file names, `lc_write_config` accepts an open file pointer so it is convenient for creating headers for data files (which don't need to be closed after the configuration is written).  It also means that the write operation doesn't need to return an error status.

[top](#top)

### <a name='dev'></a> Interacting with devices

```C
int lc_open( lc_devconf_t* dconf )
```
`lc_open_config` opens a connection to the device pointed to by `dconf`.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int lc_close( lc_devconf_t* dconf )
```
`lc_close_config` closes a connection to the device pointed to by `dconf`.  If a ringbuffer is still allocated to the device, it is freed, so it is important NOT to call `lc_close_config` before data is read from the buffer.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int lc_upload_config( lc_devconf_t* dconf )
```
`lc_upload_config` writes to the relevant modbus registers to assert the parameters in the `dconf` configuration struct.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

[top](#top)

### <a name='diag'></a> Performing diagnostics

```C
int lc_ndev(          lc_devconf_t* dconf, 
                const unsigned int devmax)
                
int lc_nistream( lc_devconf_t* dconf )
                
int lc_nostream( lc_devconf_t* dconf )
```
The `lc_ndev` returns the number of devices configured in the `dconf` array, returning a number no greater than `devmax`.  `lc_nistream` and `lc_nostream` return the number of input and ouptut stream channels configured.  The input stream count is especially helpful since it includes non-analog input streams (like digital input streams).

```C
void lc_show_config( lc_devconf_t* dconf )
```
The `lc_show_config` function prints a detailed list of parameters specified by the `dconf` configuraiton struct.  The format of the printout is varied based on what is configured so that unconfigured features are not summarized.

[top](#top)

### <a name='data'></a> Data collection

```C
int lc_stream_start(    lc_devconf_t* dconf, 
                const unsigned int devnum,
                               int samples_per_read);

int lc_stream_service(  lc_devconf_t* dconf, 
                const unsigned int devnum);

int lc_stream_read(     lc_devconf_t* dconf, 
                const unsigned int devnum, 
                            double **data, 
                      unsigned int *channels, 
                      unsigned int *samples_per_read);

int lc_stream_stop(     lc_devconf_t* dconf, 
                const unsigned int devnum);
                
```
There are four steps to a data acquisition process; start, service, read, and stop.  Version 3 of LCONFIG uses an automatically configured ring buffer, so the application never needs to perform memory management.  The addition of a service function lets the back end deal with moving data into the ring buffer, testing for trigger events, and it allows the application to distinguish between moving data on to the local machine and retrieving it for use in the application.

`lc_stream_start` configures the LCONFIG buffer and starts the device's data collection process on the device `devnum` in the `dconf` array.  For `lc_stream_start` to work correctly, the device should already be open and configured using the `lc_open_config` and `upload_config` functions.  The device will be configured to transfer packets with `samples_per_read` samples per channel.  If `samples_per_read` is negative, then the LCONF_SAMPLES_PER_READ constant will be used instead.  This information is recorded in the `RB` ringbuffer struct.  The buffer can be substantial since RAM checks are performed prior to allocating memory.

All data transfers are done in *blocks*.  A *block* of data is `samples_per_read` x `channels` of individual measurements.

The `lc_stream_service` function retrieves a single new block of data into the ring buffer.  If pre-triggering is configured, trigger events will be ignored until enough samples have built up in the buffer to meet the pre-trigger requirements.  Once a trigger event occurs (or if no trigger is configured), blocks of data will become available for reading.  

The `lc_stream_read` function returns a double precision array, `data` representing a 2D array with  a single block of data `samples_per_read` x `channels` long.  The `s` sample of `c` channel can be retrieved by `data[s*channels + c]`.  If there are no data available, then `data` will be `NULL`.

The data acquisition process is halted by the `lc_stream_stop` function, but the buffer is left intact.  As a result, data can be slowly read from the buffer long after the data collection is complete.  Before a new stream process can be started, the `clean_data_stream` function should be called to free the buffer.  In applications where only one stream operation will be executed, it may be easier to allow `lc_close_config` to clean up the buffer instead.

```C
void lc_stream_status( lc_devconf_t* dconf, 
               const unsigned int devnum,
                     unsigned int *samples_streamed, 
                     unsigned int *samples_read,
                     unsigned int *samples_waiting);
                     
int lc_stream_iscomplete(lc_devconf_t* dconf, 
                 const unsigned int devnum);
                 
int lc_stream_isempty( lc_devconf_t* dconf, 
               const unsigned int devnum);
```
These functions are handy tools for monitoring the progress of a data collection process.  The `lc_stream_status` function returns the per-channel stream counts streamed into, read out of, and waiting in the ring buffer.  Authors should keep in mind that the `lc_stream_service` function adjusts the `samples_streamed` value to exclude data that was thrown away in the triggering process.

`lc_stream_iscomplete` returns a 1 or 0 to indicate whether the total number of `samples_streamed` per channel has exceeded the `nsample` parameter found in the configuration file.

`lc_stream_isempty` returns a 1 or 0 to indicate whether the ring buffer has been exhausted by read operations.

```C
int lc_datafile_init(    lc_devconf_t* dconf, 
                const unsigned int devnum,
                             FILE* FF);
                        
int lc_datafile_write(     lc_devconf_t* dconf, 
                const unsigned int devnum,
                             FILE* FF);

```
These are tools for automatically writing data files from the stream data.  They accept a file pointer from an open `iostream` file.  In versions 2.03 and older, LCONFIG was responsible for managing the file opening and closing process, but as of version 3.00, it is up the application to provide an open file.

`lc_datafile_init` writes a configuration file header to the data file.  It also adds a timestamp indicating the date and time that `lc_datafile_init` was executed.  It should be emphasized that (especially where triggers are involved) substantial time can pass between this timestamp and the availability of data.  Care should be taken if precise absolute time values are needed.

`lc_datafile_write` calls `lc_stream_read` and prints an ascii formatted data array into the open file provided.  

Note that the applicaiton still needs to call `lc_stream_start` to begin the data acquisition process, and `lc_stream_service`.  Only `lc_stream_read` is replaced from the typical streaming process with `lc_datafile_write`.


```C
int lc_update_ef( lc_devconf_t* dconf );
```
For now, the extended feature channels represent the only features in LCONFIG that require users to interact manually with the lc_devconf_t struct member variables.  `lc_update_ef` is called to command the LabJack to react to changes in the EF settings or to obtain new EF measurements.  For example, the following code might be used to adjust flexible I/O channel 0's duty cycle to 25%.
```
dconf[devnum].fioch[0].duty = .25;
lc_update_ef(dconf,devnum);
```
The `lc_update_ef()` function also downloads the latest measurements into the relevant member variables.  The following code might be used to measure a pulse frequency from a flow sensor.
```C
double frequency;
int lc_update_ef(&dconf);
frequency = 1e6 / dconf[devnum].fioch[0].time;
```

[top](#top)

### <a name='meta'></a> Meta configuration

Experiments are about more than just data rates and analog input ranges.  What were the parameters that mattered most to you?  What did you change in today's experiment?  Was there anything you wanted to note in the data file?  Are there standard searchable fields you want associated with your data?  Meta parameters are custom configuraiton parameters that are embedded in the data file just like device parameters, but they are ignored when configuring the T4/T7.  They are only there to help you keep track of your data.

These have been absolutely essential for me.

```C
int lc_get_meta_int(          lc_devconf_t* dconf, 
                 const unsigned int devnum,
                        const char* param, 
                               int* value);
                               
int lc_get_meta_flt(          lc_devconf_t* dconf, 
                 const unsigned int devnum,
                        const char* param, 
                            double* value);

int lc_get_meta_str(          lc_devconf_t* dconf, 
                 const unsigned int devnum,
                        const char* param, 
                              char* value);
```
The `lc_get_meta_XXX` functions retrieve meta parameters from the `devnum` element of the `dconf` array by their name, `param`.  If the parameter does not exist or if it is of the incorrect type, 
The values are written to target of the `value` pointer, and the function returns the `LCONF_ERROR` or `LCONF_NOERR` error status based on whether the parameter was found.

```C
int lc_put_meta_int(          lc_devconf_t* dconf, 
                 const unsigned int devnum, 
                        const char* param, 
                                int value);
                                
int lc_put_meta_flt(          lc_devconf_t* dconf, 
                 const unsigned int devnum, 
                        const char* param, 
                             double value);

int lc_put_meta_str(          lc_devconf_t* dconf, 
                 const unsigned int devnum, 
                        const char* param, 
                              char* value);
```
The `lc_put_meta_XXX` functions retrieve meta parameters from the `devnum` element of the `dconf` array by their name, `param`.  The values are written to target of the `value` pointer, and the function returns the `LCONF_ERROR` or `LCONF_NOERR` error status based on whether the parameter was found.

[top](#top)
