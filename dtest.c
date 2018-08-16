#include "ldisplay.h"
#include "lconfig.h"
#include <string.h>     // duh
#include <unistd.h>     // for system calls
#include <stdlib.h>     // for malloc and free
#include <stdio.h>      // duh
#include <time.h>       // for file stream time stamps

#define DEF_CONFIGFILE  "dtest.conf"
#define MAX_DEV            8
#define MAXSTR          128

/*...................
.   Global Options
...................*/
char    config_file[MAXSTR] = DEF_CONFIGFILE;

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
"dtest [-h] [-c CONFIGFILE]\n"\
"  Opens a connection to the device specified in dtest.conf\n"\
"(or CONFIGFILE if specified), prints device information and exits\n"\
"GPLv3\n"\
"(c)2017 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    int ndev, devnum;
    DEVCONF dconf[MAX_DEV];

    // Parse the command-line options
    if(parse_options(argc, argv))
        return -1;

    // Load the configuration
    printf("Loading configuration file...");
    fflush(stdout);
    if(load_config(dconf, MAX_DEV, config_file)){
        fprintf(stderr, "DTEST failed while loading the configuration file \"%s\"\n", config_file);
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    ndev = ndev_config(dconf,MAX_DEV);
    if(ndev<=0){
        fprintf(stderr,"DTEST did not detect any valid device connections.\n");
        return -1;
    }else{
        fprintf(stdout,"  Found %d configured device connections (maximum %d)\n", ndev, MAX_DEV);
    }

    for(devnum = 0; devnum<ndev; devnum++){
        printf("================\nDEVICE %d\n",devnum);
        switch(dconf[devnum].connection){
        case LJM_ctANY:
            printf("  Connection type : ANY\n" );
            break;
        case LJM_ctUSB:
            printf("  Connection type : USB\n" );
            break;
        case LJM_ctETHERNET:
            printf("  Connection type : ETH\n" );
            printf("               IP : %s\n", dconf[devnum].ip);
            break;
        default:
            printf("  Connection type : UNSUPPORTED (%d)\n", dconf[devnum].connection);
        }
        

        printf("Connecting...");
        fflush(stdout);
        if(open_config(dconf, 0)){
            fprintf(stderr, "DTEST failed to open the device.\n");
            return -1;
        }
        if(upload_config(dconf, 0)){
            fprintf(stderr, "DTEST failed while configuring the device.\n");
            close_config(dconf,0);
            return -1;
        }
        printf("DONE\n");

        show_config(dconf, devnum);
        
        close_config(dconf, devnum);
        
        printf("\n");
    }
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
        }else if(strncmp(argv[index],"-c",MAXSTR)==0){
            target = config_file;
        }else if(target){
            strncpy(target,argv[index],MAXSTR);
            target = NULL;
        }else{
            target = NULL;
            fprintf(stderr,"Unexpected parameter \"%s\"\n", argv[index]);
            return -1;
        }
    }
    return 0;
}
