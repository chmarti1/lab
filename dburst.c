#include "ldisplay.h"
#include "lconfig.h"
#include <string.h>     // duh
#include <unistd.h>     // for system calls
#include <stdlib.h>     // for malloc and free
#include <stdio.h>      // duh
#include <time.h>       // for file stream time stamps

#define DEF_CONFIGFILE  "dburst.conf"
#define DEF_DATAFILE    "dburst.dat"
#define DEF_SAMPLES     "-1"
#define DEF_DURATION    "-1"
#define VERSION         "0.2"
#define MAXSTR          128

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
"dburst [-h | -v] [-c CONFIGFILE] [-n SAMPLES] [-t DURATION]\n"\
"     [-d DATAFILE] [-f|i|s param=value]\n"\
"  Runs a single high-speed burst data colleciton operation. Data are\n"\
"  streamed directly into ram and then saved to a file after collection\n"\
"  is complete.  This allows higher data rates than streaming to the hard\n"\
"  drive.\n"\
"\n"\
"-v\n"\
"  Print the version number and exit.\n"\
"\n"\
"-c CONFIGFILE\n"\
"  Specifies the LCONFIG configuration file to be used to configure the\n"\
"  LabJack T7.  By default, DBURST will look for dburst.conf\n"\
"\n"\
"-f param=value\n"\
"-i param=value\n"\
"-s param=value\n"\
"  These flags signal the creation of a meta parameter at the command\n"\
"  line.  f,i, and s signal the creation of a float, integer, or string\n"\
"  meta parameter that will be written to the data file header.\n"\
"     $ DBURST -f height=5.25 -i temperature=22 -s day=Monday\n"\
"\n"\
"-n SAMPLES\n"\
"  Specifies the integer number of samples per channel desired.  This is\n"\
"  treated as a minimum, since DBURST will collect samples in packets\n"\
"  of LCONF_SAMPLES_PER_READ (64) per channel.  LCONFIG will collect the\n"\
"  number of packets required to collect at least this many samples.\n"\
"\n"\
"  For example, the following is true\n"\
"    $ dburst -n 32   # collects 64 samples per channel\n"\
"    $ dburst -n 64   # collects 64 samples per channel\n"\
"    $ dburst -n 65   # collects 128 samples per channel\n"\
"    $ dburst -n 190  # collects 192 samples per channel\n"\
"\n"\
"  Suffixes M (for mega or million) and K or k (for kilo or thousand)\n"\
"  are recognized.\n"\
"    $ dburst -n 12k   # requests 12000 samples per channel\n"\
"\n"\
"  If both the test duration and the number of samples are specified,\n"\
"  which ever results in the longest test will be used.  If neither is\n"\
"  specified, then DBURST will collect one packet worth of data.\n"\
"\n"\
"-t DURATION\n"\
"  Specifies the test duration with an integer.  By default, DURATION\n"\
"  should be in seconds.\n"\
"    $ dburst -t 10   # configures a 10 second test\n"\
"\n"\
"  Short or long test durations can be specified by a unit suffix: m for\n"\
"  milliseconds, M for minutes, and H for hours.  s for seconds is also\n"\
"  recognized.\n"\
"    $ dburst -t 500m  # configures a 0.5 second test\n"\
"    $ dburst -t 1M    # configures a 60 second test\n"\
"    $ dburst -t 1H    # configures a 3600 second test\n"\
"\n"\
"  If both the test duration and the number of samples are specified,\n"\
"  which ever results in the longest test will be used.  If neither is\n"\
"  specified, then DBURST will collect one packet worth of data.\n"\
"\n"\
"GPLv3\n"\
"(c)2017 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    // DCONF parameters
    int     nsample,        // number of samples to collect
            nich;           // number of channels in the operation
    // Temporary variables
    int     count,          // a counter for loops
            col;            // column counter for data loop
    double  ftemp;          // Temporary float
    int     itemp;          // Temporary integer
    char    stemp[MAXSTR],  // Temporary string
            param[MAXSTR];  // Parameter
    char    optchar;
    // Configuration results
    char    config_file[MAXSTR] = DEF_CONFIGFILE,
            data_file[MAXSTR] = DEF_DATAFILE;
    int     samples = 0, 
            duration = 0;
    // Status registers
    unsigned int    samples_streamed, 
                    samples_read, 
                    samples_waiting;
    // Finally, the essentials; a data file and the device configuration
    FILE *dfile;
    DEVCONF dconf[1];

    // Parse the command-line options
    // use an outer foor loop as a catch-all safety
    for(count=0; count<argc; count++){
        switch(getopt(argc, argv, "vhc:n:t:d:f:i:s:")){
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
        // Duration
        case 't':
            optchar = 0;
            if(sscanf(optarg, "%d%c", &duration, &optchar) < 1){
                fprintf(stderr,
                        "The duration was not a number: %s\n", optarg);
                return -1;
            }
            switch(optchar){
            case 'H':
                duration *= 60;
            case 'M':
                duration *= 60;
            case 's':
            case 0:
                duration *= 1000;
            case 'm':
                break;
            default:
                fprintf(stderr,
                        "Unexpected sample duration unit %c\n", optchar);
                return -1;
            }
            break;
        // Sample count
        case 'n':
            optchar = 0;
            if(sscanf(optarg, "%d%c", &samples, &optchar) < 1){
                fprintf(stderr,
                        "The sample count was not a number: %s\n", optarg);
                return -1;
            }
            switch(optchar){
            case 'M':
                duration *= 1000;
            case 'k':
            case 'K':
                samples *= 1000;
            case 0:
                break;
            default:
                fprintf(stderr,
                        "Unexpected sample count unit: %c\n", optchar);
                return -1;
            }
            break;
        // Data file
        case 'd':
            strcpy(data_file, optarg);
            break;
        // Process meta parameters later
        case 'f':
        case 'i':
        case 's':
            break;
        case '?':
            fprintf(stderr, "Unexpected option %s\n", argv[optind]);
            return -1;
        case -1:    // Deliberately combine -1 and default
        default:
            count = argc;
            break;
        }
    }

    // Load the configuration
    printf("Loading configuration file...");
    fflush(stdout);
    if(load_config(dconf, 1, config_file)){
        fprintf(stderr, "DBURST failed while loading the configuration file \"%s\"\n", config_file);
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    if(ndev_config(dconf, 1)<=0){
        fprintf(stderr,"DBURST did not detect any valid devices for data acquisition.\n");
        return -1;
    }
    // Detect the number of input columns
    nich = nistream_config(dconf, 0);

    // Process the staged command-line meta parameters
    // use an outer for loop as a catch-all safety
    optind = 1;
    for(count=0; count<argc; count++){
        switch(getopt(argc, argv, "hc:n:t:d:f:i:s:")){
        // Process meta parameters later
        case 'f':
            if(sscanf(optarg,"%[^=]=%lf",(char*) param, &ftemp) != 2){
                fprintf(stderr, "DBURST expected param=float format, but found %s\n", optarg);
                return -1;
            }
            printf("flt:%s = %lf\n",param,ftemp);
            if (put_meta_flt(dconf, 0, param, ftemp))
                fprintf(stderr, "DBURST failed to set parameter %s to %lf\n", param, ftemp);            
            break;
        case 'i':
            if(sscanf(optarg,"%[^=]=%d",(char*) param, &itemp) != 2){
                fprintf(stderr, "DBURST expected param=integer format, but found %s\n", optarg);
                return -1;
            }
            printf("int:%s = %d\n",param,itemp);
            if (put_meta_int(dconf, 0, param, itemp))
                fprintf(stderr, "DBURST failed to set parameter %s to %d\n", param, itemp);
            break;
        case 's':
            if(sscanf(optarg,"%[^=]=%s",(char*) param, (char*) stemp) != 2){
                fprintf(stderr, "DBURST expected param=string format, but found %s\n", optarg);
                return -1;
            }
            printf("str:%s = %s\n",param,stemp);
            if (put_meta_str(dconf, 0, param, stemp))
                fprintf(stderr, "DBURST failed to set parameter %s to %s\n", param, stemp);
            break;
        // Escape condition
        case -1:
            count = argc;
            break;
        // We've already done error handling
        default:
            break;
        }
    }

    // Calculate the number of samples to collect
    // If neither the sample nor duration option is configured, leave 
    // configuration alone
    if(samples > 0 || duration > 0){
        // Calculate the number of samples to collect
        // Use which ever is larger: samples or duration
        nsample = (duration * dconf[0].samplehz) / 1000;  // duration is in ms
        nsample = nsample > samples ? nsample : samples;
        dconf[0].nsample = nsample;
    }

    // Print some information
    printf("  Stream channels : %d\n", nich);
    printf("      Sample rate : %.1fHz\n", dconf[0].samplehz);
    printf(" Samples per chan : %d (%d requested)\n", dconf[0].nsample, samples);
    ftemp = dconf[0].nsample/dconf[0].samplehz;
    if(ftemp>60){
        ftemp /= 60;
        if(ftemp>60){
            ftemp /= 60;
            printf("    Test duration : %fhr (%d requested)\n", (float)(ftemp), duration/3600000);
        }else{
            printf("    Test duration : %fmin (%d requested)\n", (float)(ftemp), duration/60000);
        }
    }else if(ftemp<1)
        printf("    Test duration : %fms (%d requested)\n", (float)(ftemp*1000), duration);
    else
        printf("    Test duration : %fs (%d requested)\n", (float)(ftemp), duration/1000);


    printf("Setting up measurement...");
    fflush(stdout);
    if(open_config(dconf, 0)){
        fprintf(stderr, "DBURST failed to open the device.\n");
        return -1;
    }
    if(upload_config(dconf, 0)){
        fprintf(stderr, "DBURST failed while configuring the device.\n");
        close_config(dconf,0);
        return -1;
    }
    printf("DONE\n");

    // Start the data stream
    if(start_data_stream(dconf, 0, -1)){
        fprintf(stderr, "\nDBURST failed to start data collection.\n");
        close_config(dconf,0);
        return -1;
    }

    // Stream data
    printf("Streaming data");
    fflush(stdout);
    if(dconf[0].trigstate == TRIG_PRE)
        printf("\nWaiting for trigger\n");

    while(!iscomplete_data_stream(dconf,0)){
        if(service_data_stream(dconf, 0)){
            fprintf(stderr, "\nDBURST failed while servicing the T7 connection!\n");
            stop_data_stream(dconf,0);
            close_config(dconf,0);
            return -1;            
        }

        if(dconf[0].trigstate == TRIG_IDLE || dconf[0].trigstate == TRIG_ACTIVE){
            printf(".");
            fflush(stdout);
        }
    }
    // Halt data collection
    if(stop_data_stream(dconf, 0)){
        fprintf(stderr, "\nDBURST failed to halt preliminary data collection!\n");
        close_config(dconf,0);
        return -1;
    }
    printf("DONE\n");

    // Open the output file
    printf("Writing the data file");
    fflush(stdout);
    dfile = fopen(data_file,"w");
    if(dfile == NULL){
        printf("FAILED\n");
        fprintf(stderr, "DBURST failed to open the data file \"%s\"\n", data_file);
        return -1;
    }

    // Write the configuration header
    init_data_file(dconf,0,dfile);
    // Write the samples
    while(!isempty_data_stream(dconf,0)){
        write_data_file(dconf,0,dfile);
        //printf(".");
        //fflush(stdout);
        status_data_stream(dconf,0,&samples_streamed, &samples_read, &samples_waiting);
        printf(".");
        fflush(stdout);
    }
    fclose(dfile);
    close_config(dconf,0);
    printf("DONE\n");

    printf("Exited successfully.\n");
    return 0;
}
