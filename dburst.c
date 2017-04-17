#include "ldisplay.h"
#include "lconfig.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define DEF_CONFIGFILE  "dburst.conf"
#define DEF_SAMPLES     "-1"
#define DEF_DURATION    "-1"
#define MAXSTR          128

/*...................
.   Global Options
...................*/
char    config_file[MAXSTR] = DEF_CONFIGFILE,
        samples[MAXSTR] = DEF_SAMPLES,
        duration[MAXSTR] = DEF_DURATION;
int samples_i, duration_i;

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
"dburst [-h] [-c CONFIGFILE] [-n SAMPLES] [-t DURATION]\n"\
"  Runs a single high-speed burst data colleciton operation.\n"\
"\n"\
"(c)2017 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    double *buffer;
    unsigned int buffer_size;
    int ndev;
    DEVCONF dconf[NDEV];

    // Parse the command-line options
    if(parse_options(argc, argv))
        return -1;

    // Load the configuration
    printf("Loading configuration file...");
    if(load_config(dconf, NDEV, config_file)){
        printf("FAILED\n");
        fprintf(stderr, "DBURST failed while loading the configuration file \"%s\"\n", config_file);
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    ndev = ndev_config(dconf, NDEV);

    if(ndev<=0){
        fprintf(stderr,"DRUN did not detect any valid devices for data acquisition.\n");
        return -1;
    }

    // Calculate the buffer size required
    // If neither the sample nor duration option is configured
    // Default to a single read operation
    if(sample_i < 0 && duration_i < 0)


    // Loop over the devices discovered
    for(devnum=0; devnum<MAXDEV && dconf[devnum].connection>=0; devnum++){
        printf("Starting measurement %d...", devnum);
        if(open_config(dconf, 0)){
            printf("FAILED\n");
            fprintf(stderr, "DBURST failed to open the device.\n");
            return -1;
        }
        if(upload_config(dconf, 0)){
            printf("FAILED\n");
            fprintf(stderr, "DBURST failed while configuring the device.\n");
            close_config(dconf,0);
            return -1;
        }
        if(start_file_stream(dconf, 0, -1, pre_file)){
            printf("FAILED\n");
            fprintf(stderr, "DBURST failed to start data collection.\n");
            close_config(dconf,0);
            return -1;
        }
        if(read_file_stream(dconf, 0)){
            printf("FAILED\n");
            fprintf(stderr, "DBURST failed while trying to read the preliminary data!\n");
            stop_file_stream(dconf,0);
            close_config(dconf,0);
            return -1;
        }
        if(stop_file_stream(dconf, 0)){
            printf("FAILED\n");
            fprintf(stderr, "DBURST failed to halt preliminary data collection!\n");
            close_config(dconf,0);
            return -1;
        }
        close_config(dconf,0);
        printf("DONE\n");

    }



    // Perform the streaming data collection process
    if(open_config(dconf, stream_dev)){
        printf("FAILED\n");
        fprintf(stderr, "DRUN failed to open the device for streaming.\n");
        return -1;
    }
    if(upload_config(dconf, stream_dev)){
        printf("FAILED\n");
        fprintf(stderr, "DRUN failed while configuring the data stream.\n");
        close_config(dconf,stream_dev);
        return -1;
    }
    if(start_file_stream(dconf, stream_dev, -1, data_file)){
        printf("FAILED\n");
        fprintf(stderr, "DRUN failed to start the data stream.\n");
        close_config(dconf,stream_dev);
        return -1;
    }
    show_config(dconf, stream_dev);

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
        }else if(strncmp(argv[index],"-m",MAXSTR)==0)
            target = max_buffer;
        else if(strncmp(argv[index],"-c",MAXSTR)==0)
            target = config_file;
        else if(target){
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
            fprintf(stderr, "Did not recognize the sample count \"%s\"\n", multiplier);
            return -1;
        }
    }else if(err!=1){
        fprintf(stderr, "Failed to parse buffer size \"%s\"\n", samples);
        return -1;
    }
    // Post processing - convert the duration string to an integer
    // Scan for an integer followed by m,s,M, or H
    err = sscanf(samples,"%d%c",&duration_i, &multiplier);
    if(err==2){
        if(multiplier=='m')
            // NOOP Do nothing for milliseconds
        else if(multiplier=='s')
            duration_i *= 1000;
        else if(multiplier=='M')
            duration_i *= 60000;
        else if(multiplier=='H')
            duration_i *= 3600000;
        else{
            fprintf(stderr, "Did not recognize the sample count \"%s\"\n", multiplier);
            return -1;
        }
    }else if(err==1){
        samples_i *= 1000;
    }else{
        fprintf(stderr, "Failed to parse buffer size \"%s\"\n", samples);
        return -1;
    }

    return 0;
}
