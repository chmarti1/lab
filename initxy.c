/* INITXY
 *  The INITXY utility sends UART commands through the LabJack to command 
 *  Animatics SMART MOTORS to move an X-Y translaiton stage.  It loads the 
 *  LabJack configuration in move.conf using the LConfig system
 */


#include "lconfig.h"
#include "smiface.h"
#include <stdio.h>
#include <getopt.h>

#define CONFIG_FILE "move.conf"

const char help_text[] = \
"INITXY [-h] [-c CONFIGFILE]\n"\
"\n"\
"  Communicates over a LabJack's UART with two Animatics Smart Motors to \n"\
"  initialize them for motion.\n"\
"\n"\
"-c CONFIGFILE\n"\
"  Specifies the LCONFIG configuration file to be used to configure the\n"\
"  LabJack.  By default, INITXY will look for \"move.conf\"\n"\
"\n"\
"-h\n"\
"  Prints this help text and exits\n"
"\n"\
"GPLv3\n"\
"(c)2017-2019 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    char config_file[LCONF_MAX_STR];
    char this;
    
    // Initialize the configuraiton file name
    strcpy(config_file, "move.conf");

    // Parse the command-line options
    // use an outer foor loop as a catch-all safety
    while(-1 != (this=getopt(argc, argv, "hc:"))){
        switch(this){
        // Help text
        case 'h':
            printf(help_text);
            return 0;
        // Config file
        case 'c':
            strcpy(config_file, optarg);
            break;
        case '?':
            fprintf(stderr, "INITXY received unrecognized flag: %c\n", optopt);
        // Catch errors
        default:
            fprintf(stderr, "INITXY encountered an unexpeced exception while parsing command line options.\n");
            return -1;
        }
    }
    
    if(smi_conf(config_file))
        return -1;
    if(smi_init())
        return -1;
    return 0;
}
