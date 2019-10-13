/* MOVEY
 *  The MOVEY utility sends UART commands through the LabJack to command 
 *  Animatics SMART MOTORS to move an X-Y translaiton stage.  It loads the 
 *  LabJack configuration in move.conf using the LConfig system
 */


#include "lconfig/lconfig.h"
#include "smiface.h"
#include <stdio.h>
#include <getopt.h>

#define CONFIG_FILE "move.conf"

const char help_text[] = \
"movex  [-hi] [-c CONFIGFILE] X\n"\
"movey  [-hi] [-c CONFIGFILE] Y\n"\
"movexy [-hi] [-c CONFIGFILE] X Y\n"\
"\n"\
"  Communicates over a LabJack's UART with two Animatics Smart Motors to command\n"\
"  x and y\n motion.  Motion is NOT coordinated; only the final position is\n"\
"  specified.  Before calling MOVEY, the user should first call INITXY to\n"\
"  initialize the motors.\n"\
"\n"\
"  By default, X and Y should be floating point numbers representing absolute X\n"\
"  and Y coordinates in mm.  If the -i flag is specified, then X and Y will be\n"\
"  interpreted as inches.  Since the '-' character signals an option flag, \n"\
"  negative values are, instead, signaled with a leading '_', so \"_12\" is\n"\
"  interpreted as negative 12.\n"\
"\n"\
"-c CONFIGFILE\n"\
"  Specifies the LCONFIG configuration file to be used to configure the\n"\
"  LabJack.  By default, MOVEY will look for \"move.conf\"\n"\
"\n"\
"-h\n"\
"  Prints this help text and exits\n"
"\n"\
"-i\n"\
"  Use inches instead of millimeters\n"\
"\n"\
"GPLv3\n"\
"(c)2019 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    char config_file[LCONF_MAX_STR];
    enum {UNIT_MM, UNIT_IN} units;
    char this;
    double y;
    
    // Initialize the configuraiton file name
    strcpy(config_file, "/etc/lconfig/move.conf");
    // Initialize the unit system
    units = UNIT_MM;

    // Parse the command-line options
    // use an outer foor loop as a catch-all safety
    while(-1 != (this=getopt(argc, argv, "hic:"))){
        switch(this){
        // Help text
        case 'h':
            printf(help_text);
            return 0;
        // Config file
        case 'c':
            strcpy(config_file, optarg);
            break;
        // Duration
        case 'i':
            units = UNIT_IN;
            break;
        case '?':
            fprintf(stderr, "MOVEY received unrecognized flag: %c\n", optopt);
            return -1;
        // Catch errors
        default:
            fprintf(stderr, "MOVEY encountered an unexpeced exception while parsing command line options.\n");
            return -1;
        }
    }
    if(argc - optind != 1){
        fprintf(stderr, "MOVEY expects one mandatory arguments - Y location\n");
        return -1;
    }
    // Check for negative numbers
    if(argv[optind][0] == '_') argv[optind][0] = '-';
    
    if(sscanf(argv[optind], "%lf", &y)!=1){
        fprintf(stderr, "MOVEY expected a numerical Y location, but received %s\n", argv[optind]);
        return -1;
    }else if(units == UNIT_IN){
        y *= 25.4;
    }
    
    if(smi_conf(config_file))
        return -1;
    if(smi_move_y(y))
        return -1;
    return 0;;
}