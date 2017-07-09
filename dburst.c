#include "ldisplay.h"
#include "lconfig.h"
#include <string.h>     // duh
#include <unistd.h>     // for system calls
#include <stdlib.h>     // for malloc and free
#include <stdio.h>      // duh
#include <time.h>       // for file stream time stamps
#include <sys/sysinfo.h>    // for ram overload checking

#define DEF_CONFIGFILE  "dburst.conf"
#define DEF_DATAFILE    "dburst.dat"
#define DEF_SAMPLES     "-1"
#define DEF_DURATION    "-1"
#define MAXSTR          128

/*...................
.   Global Options
...................*/
char    config_file[MAXSTR] = DEF_CONFIGFILE,
        data_file[MAXSTR] = DEF_DATAFILE,
        samples[MAXSTR] = DEF_SAMPLES,
        duration[MAXSTR] = DEF_DURATION,
        metastage[LCONF_MAX_META][MAXSTR+1];
int samples_i, duration_i, metacount;

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
"dburst [-h] [-c CONFIGFILE] [-n SAMPLES] [-t DURATION] [-d DATAFILE]\n"\
"     [-f|i|s param=value]\n"\
"  Runs a single high-speed burst data colleciton operation. Data are\n"\
"  streamed directly into ram and then saved to a file after collection\n"\
"  is complete.  This allows higher data rates than streaming to the hard\n"\
"  drive.\n"\
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
"  Specifies the integer number of samples per channel desired.  This\n"\
"  treated as a minimum, since DBURST will collect samples in packets\n"\
"  specified by the NSAMPLE configuration parameter.  DBURST calculates\n"\
"  the number of packets required to collect at least this many samples.\n"\
"\n"\
"  For example, when the NSAMPLE parameter is configured to 64 (the \n"\
"  default), the following is true\n"\
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
    double *buffer;
    int reads,  // number of packets to read
        read_size,  // total number of samples for all channels in each packet
        nich, // number of channels in the operation
        count, // a counter for loops
        col;    // column counter for data loop
    long int buffer_bytes;  // requested buffer size in bytes
    double ftemp;
    int itemp;
    char stemp[MAXSTR], param[MAXSTR];
    time_t now;
    struct sysinfo sinf;
    FILE *dfile;
    DEVCONF dconf[1];

    // Parse the command-line options
    if(parse_options(argc, argv))
        return -1;

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
    for(metacount--; metacount>=0; metacount--){
        if(metastage[metacount][0]=='f'){
            if(sscanf(&metastage[metacount][1],"%[^=]=%lf",(char*)&param, &ftemp) != 2){
                fprintf(stderr, "DBURST expected param=float format, but found %s\n", &metastage[metacount][1]);
                return -1;
            }
            printf("flt:%s = %lf\n",param,ftemp);
            if (put_meta_flt(dconf, 0, param, ftemp))
                fprintf(stderr, "DBURST failed to set parameter %s to %lf\n", param, ftemp);
        }else if(metastage[metacount][0]=='i'){
            if(sscanf(&metastage[metacount][1],"%[^=]=%d",(char*)&param, &itemp) != 2){
                fprintf(stderr, "DBURST expected param=integer format, but found %s\n", &metastage[metacount][1]);
                return -1;
            }
            printf("int:%s = %d\n",param,itemp);
            if (put_meta_int(dconf, 0, param, itemp))
                fprintf(stderr, "DBURST failed to set parameter %s to %d\n", param, itemp);
        }else if(metastage[metacount][0]=='s'){
            if(sscanf(&metastage[metacount][1],"%[^=]=%s",(char*)&param, (char*)&stemp) != 2){
                fprintf(stderr, "DBURST expected param=string format, but found %s\n", &metastage[metacount][1]);
                return -1;
            }
            printf("str:%s = %s\n",param,stemp);
            if (put_meta_str(dconf, 0, param, stemp))
                fprintf(stderr, "DBURST failed to set parameter %s to %s\n", param, stemp);
        }else{
            fprintf(stderr, "DBURST unexpected error parsing staged command-line meta parameters!\n");
            return -1;
        }
    }

    // Calculate the buffer size required
    // If neither the sample nor duration option is configured
    // Default to a single read operation
    // First, store in buffer_size the number of samples to collect
    if(samples_i < 0 && duration_i < 0){
        reads = 1;
        read_size = dconf[0].nsample * nich;
    }else{
        // Calculate the number of samples to collect
        // Use which ever is larger: samples_i or duration_i
        reads = duration_i * dconf[0].samplehz / 1000;
        reads = reads > samples_i ? reads : samples_i;
        // Now, adjust the buffer_size to match an integer number of read operations
        read_size = dconf[0].nsample * nich;
        if(reads % dconf[0].nsample)
            reads = (reads / dconf[0].nsample) + 1;
        else
            reads = (reads / dconf[0].nsample);
    }
    buffer_bytes = ((long)reads)*((long)read_size)*sizeof(double);

    // Print some information
    printf("  Stream channels : %d\n", nich);
    printf("      Sample rate : %.1fHz\n", dconf[0].samplehz);
    printf(" Samples per chan : %d (%d requested)\n", reads*dconf[0].nsample, samples_i);
    ftemp = (double) buffer_bytes;
    if(ftemp>1024){
        ftemp/=1024;
        if(ftemp>1024){
            ftemp/=1024;
            printf("      Buffer size : %dMiB\n", (int)ftemp);
        }else
            printf("      Buffer size : %dKiB\n", (int)ftemp);
    }else
        printf("      Buffer size : %dB\n", (int)ftemp);
    printf("Samples per packet: %d\n", read_size);
    printf("          Packets : %d\n", reads);
    ftemp = reads*dconf[0].nsample/dconf[0].samplehz;
    if(ftemp>60){
        ftemp /= 60;
        if(ftemp>60){
            ftemp /= 60;
            printf("    Test duration : %fhr (%s requested)\n", (float)(ftemp), duration);
        }else{
            printf("    Test duration : %fmin (%s requested)\n", (float)(ftemp), duration);
        }
    }else if(ftemp<1)
        printf("    Test duration : %fms (%s requested)\n", (float)(ftemp*1000), duration);
    else
        printf("    Test duration : %fs (%s requested)\n", (float)(ftemp), duration);

    // Check current RAM status
    sysinfo(&sinf);
    // If the requested memory is more than the system's memory, halt
    if(sinf.totalram < buffer_bytes){
        fprintf(stderr, "Total system memory is only %ld bytes\nAborting.\n", sinf.totalram);
        return -1;
    }else if(sinf.freeram*0.9 < buffer_bytes){
        fprintf(stderr, "Only %ld bytes of memory free.\nAborting.\n", sinf.freeram);
        return -1;
    }

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

    // Reserve the buffer and start the data collection
    buffer = malloc(reads * read_size * sizeof(double));
    if(buffer == NULL){
        fprintf(stderr, "DBURST failed to allocate the buffer.\n");
        close_config(dconf,0);
        return -1;
    }
    printf("DONE\n");

    // Log the time
    time(&now);

    // Start the data stream
    if(start_data_stream(dconf, 0, -1)){
        fprintf(stderr, "\nDBURST failed to start data collection.\n");
        close_config(dconf,0);
        free(buffer);
        return -1;
    }

    // Stream data
    printf("Streaming data.");
    fflush(stdout);
    for(count=0; count<reads; count++){
        if(read_data_stream(dconf, 0, &buffer[read_size*count])){
            fprintf(stderr, "\nDBURST failed while trying to read the preliminary data!\n");
            stop_file_stream(dconf,0);
            close_config(dconf,0);
            free(buffer);
            return -1;
        }
        printf("%d.",count);
        fflush(stdout);
    }

    // Halt data collection
    if(stop_file_stream(dconf, 0)){
        fprintf(stderr, "\nDBURST failed to halt preliminary data collection!\n");
        close_config(dconf,0);
        free(buffer);
        return -1;
    }
    close_config(dconf,0);
    printf("DONE\n");

    // Open the output file
    printf("Writing the data file...");
    fflush(stdout);
    dfile = fopen(data_file,"w");
    if(dfile == NULL){
        free(buffer);
        printf("FAILED\n");
        fprintf(stderr, "DBURST failed to open the data file \"%s\"\n", data_file);
        return -1;
    }

    // Write the configuration header
    write_config(dconf,0,dfile);
    // Write the time data colleciton started
    fprintf(dfile, "#: %s", ctime(&now));
    // Write the samples
    count = 0;
    while(count<reads*read_size){
        for(col=0; col<nich-1; col++){
            fprintf(dfile, "%.6e ",buffer[count]);
            count++;
        }
        fprintf(dfile, "%.6e\n",buffer[count]);
        count++;
    }
    fclose(dfile);
    printf("DONE\n");

    free(buffer);
    printf("Exited successfully.\n");
    return 0;
}



int parse_options(int argc, char* argv[]){
    int index, value, err;
    char* target=NULL;
    char multiplier=' ';

    // The target pointer indicates to what global parameter string values
    // will be written.  Option flags refocus the pointer and writing a value
    // will swing the pointer back to NULL.
    for(index=1; index<argc; index++){
        // Help text
        if(strncmp(argv[index],"-h",MAXSTR)==0){
            printf(help_text);
            return -1;
        }else if(strncmp(argv[index],"-t",MAXSTR)==0)
            target = duration;
        else if(strncmp(argv[index],"-c",MAXSTR)==0)
            target = config_file;
        else if(strncmp(argv[index],"-n",MAXSTR)==0)
            target = samples;
        else if(strncmp(argv[index],"-d",MAXSTR)==0)
            target = data_file;
        else if(strncmp(argv[index],"-f",MAXSTR)==0 ||
                strncmp(argv[index],"-i",MAXSTR)==0 ||
                strncmp(argv[index],"-s",MAXSTR)==0){
            if(metacount>LCONF_MAX_META){
                target=NULL;
                fprintf(stderr,"Too many meta parameters. LCONFIG only accepts %d.\n",LCONF_MAX_META);
            }
            // Write the type specifier as the first character
            metastage[metacount][0] = argv[index][1];
            // Copy the remaining text behind
            target = &metastage[metacount][1];
            metacount++;
        }else if(target){
            strncpy(target,argv[index],MAXSTR);
            target = NULL;
        }else{
            target = NULL;
            fprintf(stderr,"Unexpected parameter \"%s\"\n", argv[index]);
            return -1;
        }
    }
    // Post processing - convert the samples string to an integer
    // Scan for an integer followed by M,K, or k
    err = sscanf(samples,"%d%c",&samples_i, &multiplier);
    if(err==2){
        if(multiplier=='k' || multiplier=='K')
            samples_i *= 1000;
        else if(multiplier=='M')
            samples_i *= 1000000;
        else{
            fprintf(stderr, "Did not recognize the sample multiplier \"%c\"\n", multiplier);
            return -1;
        }
    }else if(err!=1){
        fprintf(stderr, "Failed to parse buffer size \"%s\"\n", samples);
        return -1;
    }
    // Post processing - convert the duration string to an integer
    // Scan for an integer followed by m,s,M, or H
    err = sscanf(duration,"%d%c",&duration_i, &multiplier);
    if(err==2){
        if(multiplier=='m'){
            // NOOP Do nothing for milliseconds
        }else if(multiplier=='s')
            duration_i *= 1000;
        else if(multiplier=='M')
            duration_i *= 60000;
        else if(multiplier=='H')
            duration_i *= 3600000;
        else{
            fprintf(stderr, "Did not understand time unit \"%c\"\n", multiplier);
            return -1;
        }
    }else if(err==1){
        duration_i *= 1000;
    }else{
        fprintf(stderr, "Failed to parse buffer size \"%s\"\n", samples);
        return -1;
    }

    return 0;
}
