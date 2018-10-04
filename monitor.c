#include "ldisplay.h"
#include "lconfig.h"
#include <string.h>     // duh
#include <stdio.h>      // duh
#include <sys/time.h>   // for refresh timing

#define DEF_CONFIGFILE  "monitor.conf"
#define DEF_UPDATE      0.5
#define EXIT_CHAR       'Q'
#define VERSION         "0.1"
#define MAXSTR          128
#define FMT_GOTO        "\x1B[%d;%dH"
#define FMT_HEADER      "\x1B[1m%20s (%3s): %16s   %16s\x1B[0m\n"
#define FMT_LINE        "%20s (%3d): %16f V %16f %s\n"
#define FMT_CL          "\x1B[K"

/*...................
.   Global Options
...................*/


/*....................
. Prototypes
.....................*/
// Parse the command-line options strings
// Modifies the globals appropriately
int parse_options(int argc, char*argv[]);

/*....................
. Help text
.....................*/
const char help_text[] = \
"monitor [-h | -v] [-c CONFIGFILE] [-t UPDATE]\n"\
"  Prints a time-averaged summary of measurements from all configured\n"\
"  channels.  Each data collection phase is performed in a burst of\n"\
"  NSAMPLE samples per channel, which are averaged to produce the\n"\
"  reported values.  Monitor uses the AILABEL parameter to name each\n"\
"  channel, and honors the calibration configuration to produce the\n"\
"  calibrated value column.\n"\
"\n"\
"-h\n"\
"  Print this help and exit.\n"\
"-v\n"\
"  Print the version number and exit.\n"\
"\n"\
"-c CONFIGFILE\n"\
"  Specifies the LCONFIG configuration file to be used to configure the\n"\
"  LabJack T7.  By default, MONITOR will look for monitor.conf\n"\
"\n"\
"-t UPDATE\n"\
"  Specifies the approximate time between screen refreshes in seconds.\n"\
"  By default, the screen is refreshed about every 0.5 seconds.  If the\n"\
"  data acquisition parameters, SAMPLEHZ and NSAMPLE cause the data \n"\
"  collection to last longer than UPDATE, then it will be ignored.\n"\
"\n"\
"GPLv3\n"\
"(c)2018 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    // DCONF parameters
    int     nsample,        // number of samples to collected
            nich;           // number of channels in the operation
    // Temporary variables
    int     count,          // a counter for loops
            err,            // LCONF return value
            ii, jj;         // For loop indices
    double  raw_meas[LCONF_MAX_AICH],   // uncalibrated measurements
            cal_meas[LCONF_MAX_AICH],   // calibrated measurements
            Tamb,           // Ambient temperature
            *data;          // Pointer into the LCONF buffer
    char    stemp[MAXSTR],  // Temporary string
            param[MAXSTR];  // Parameter
    char    optchar;
    // Configuration results
    char    config_file[MAXSTR] = DEF_CONFIGFILE;
    double  update_sec = DEF_UPDATE;
    // Status registers
    char            go;                 // Flag for exit conditions
    unsigned int    channels,
                    samples_per_read;
    unsigned int    iusec, isec;        // Timing interval in us and s
    struct timeval  tupdate, tt;        // Used for timing the refresh
    DEVCONF dconf[1];       // Device configuration structur(s)

    // Parse the command-line options
    // use an outer foor loop as a catch-all safety
    for(count=0; count<argc; count++){
        
        switch(optchar=getopt(argc, argv, "vhc:t:")){
        // Help text
        case 'h':
            printf(help_text);
            return 0;
        // Version number
        case 'v':
            printf("%s\n",VERSION);
            return 0;
        // Config file
        case 'c':
            strcpy(config_file, optarg);
            break;
        // Time intervals
        case 't':
            if(!sscanf(optarg, "%lf", &update_sec)){
                fprintf(stderr, "MONITOR: Expected an update period in seconds. Got: %s\n", optarg);
                return -1;
            }
            break;
        case -1:    // Deliberately combine -1 and default
            count = argc;
            break;
        default:
            fprintf(stderr, "MONITOR: Unexpected switch\n");
            return -1;
        }
    }
    // Convert the update interval in seconds into seconds and usec
    if(update_sec<=0){
        iusec = 0;
        isec = 0;
    }else{
        isec = (unsigned int)(update_sec);
        iusec = (unsigned int)((update_sec - isec) * 1e6);
    }

    // Load the configuration
    printf("Loading configuration file...");
    fflush(stdout);
    if(load_config(dconf, 1, config_file)){
        fprintf(stderr, "MONITOR failed while loading the configuration file \"%s\"\n", config_file);
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    if(ndev_config(dconf, 1)<=0){
        fprintf(stderr,"MONITOR did not detect any valid devices for data acquisition.\n");
        return -1;
    }
    
    // Detect the number of input channels
    channels = nistream_config(dconf, 0);
    // Print some information
    printf("  Stream channels : %d\n", channels);
    printf("      Sample rate : %.1fHz\n", dconf[0].samplehz);
    printf(" Samples per chan : %d\n", dconf[0].nsample);

    printf("Setting up measurement...");
    fflush(stdout);
    if(open_config(dconf, 0)){
        fprintf(stderr, "MONITOR failed to open the device.\n");
        return -1;
    }
    if(upload_config(dconf, 0)){
        fprintf(stderr, "MONITOR failed while configuring the device.\n");
        close_config(dconf,0);
        return -1;
    }
    printf("DONE\n");

    // Initialize the display
    clear_terminal();
    setup_keypress();
    fprintf(stdout, FMT_GOTO, 1, 1);
    fprintf(stdout, "Press '%c' to exit.", EXIT_CHAR);
    fprintf(stdout, FMT_GOTO, 2, 1);
    fprintf(stdout, FMT_HEADER, "Label", "Chn", "Voltage", "Calibrated");

    // Initialize the data parameters to safe values in case there is
    // an error while executing the stream functions.
    channels = 0;
    samples_per_read = 0;

    go = 1;
    for(count=0; go; count++){
        // Calculate the next update time
        gettimeofday(&tupdate,NULL);
        tupdate.tv_sec += isec;
        tupdate.tv_usec += iusec;
        
        // Execute the measurement
        nsample = 0;
        err = start_data_stream(dconf,0, -1);
        while( !iscomplete_data_stream(dconf,0) ){
            err = service_data_stream(dconf,0);
            err = err || read_data_stream(dconf, 0, &data, &channels, &samples_per_read);
            // Stop everything if there is an error or the user exits
            if(err){
                fprintf(stderr, "MONITOR: There was an error while streaming data!\n");
                go = 0;
                break;
            }else if((keypress() && getchar()==EXIT_CHAR)){
                go = 0;
                break;
            }
            if(data){
                // Average the data
                for(ii=0;ii<channels;ii++){
                    for(jj=ii;jj<channels*samples_per_read;jj+=channels){
                        raw_meas[ii] += data[jj];
                    }
                }
            }
        }
        stop_data_stream(dconf,0);
        // Detect the number of samples read
        status_data_stream(dconf, 0, NULL, &nsample, NULL);
        clean_data_stream(dconf,0);

        LJM_eReadName(dconf[0].handle, "TEMPERATURE_AIR_K", &Tamb);
        // Complete the averaging
        for(ii=0;ii<channels;ii++){
            raw_meas[ii]/=nsample;
            cal_meas[ii] = apply_cal(dconf,0,ii,raw_meas[ii],Tamb);
        }
        
        // Update the output
        for(ii=0; ii<dconf[0].naich; ii++){
            fprintf(stdout, FMT_GOTO, 3+ii,1);
            fprintf(stdout, FMT_CL);
            fprintf(stdout, FMT_LINE, 
                    dconf[0].aich[ii].label,
                    ii,
                    raw_meas[ii],
                    cal_meas[ii],
                    dconf[0].aich[ii].units);
        }
        
        // Now, wait for the refresh period
        while(1){
            gettimeofday(&tt, NULL);
            if(tt.tv_sec > tupdate.tv_sec || 
                    (tt.tv_sec==tupdate.tv_sec && tt.tv_usec >= tupdate.tv_usec))
                break;
            else if(err || (keypress() && getchar()==EXIT_CHAR)){
                go = 0;
                break;
            }
        }
    }
    finish_keypress();

    stop_data_stream(dconf,0);
    close_config(dconf,0);

    return 0;
}
