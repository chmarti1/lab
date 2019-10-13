/* The SMART MOTOR INTERFACE
 *
 */

#ifndef __SMIFACE_H__
#define __SMIFACE_H__

#include "lconfig/lconfig.h"
#include <stdio.h>
#include <string.h>

#define MAXBUFFER       128
#define SMI_XCOM        0
#define SMI_YCOM        1
#define SMI_TIMEOUT     4000
#define SMI_CPR         4000
#define SMI_XTPMM       16./25.4
#define SMI_YTPMM       .25

// Global device configuration
lc_devconf_t dconf;
char rx[MAXBUFFER];

// Translate between raw strings and the LJ com interface
int smi_com(int axis, char * tx, char * rx, int rxlength, int timeout){
    static unsigned char txbuffer[MAXBUFFER<<1], rxbuffer[MAXBUFFER<<1];
    int ii, jj, txlength;
    
    txlength = strlen(tx);
    for(ii = 0; ii<txlength; ii++){
        jj = ii<<1;
        txbuffer[jj] = 0;
        txbuffer[jj+1] = tx[ii];
        //printf("%c", tx[ii]);
    }
    if(lc_communicate(&dconf, axis, txbuffer, txlength<<1, rxbuffer, rxlength<<1, timeout))
        return -1;
    for(ii = 0; ii<rxlength; ii++){
        rx[ii] = rxbuffer[(ii<<1)+1];
        //printf("%c", rx[ii]);
    }
    return 0;
}


int smi_conf(char config_file[]){
    // load the configuration and open a connection to the labjack
    if(     lc_load_config(&dconf, 1, config_file) ||\
            lc_open(&dconf))
        return -1;
    if(dconf.ncomch != 2){
        fprintf(stderr, "SMI_INIT: Expected 2 com channels in config file, %s.  Found %d com channels\n", config_file, dconf.ncomch);
        return -1;
    }
    return 0;
}


int smi_done(){
	lc_close(&dconf);
	return 0;
}

/* Loads a LConfig LabJack configuration file which should contain two COM
 * channels for communicating with the smart motors.  Then, initializes each
 * motor.
 */
int smi_init(){
    static const char initcmd[] = "UCI\n"
        "UDI\n"
        "ZS\n"
        "MP\n"
        "A=100\n"
        "V=100000\n"
        "O=0\n"
        "P=0\n"
        "G\n"
        "PRINT(\"OK\")\n";
        
    // Init X
    memset(rx,0,sizeof(2));
    if(smi_com(SMI_XCOM, initcmd, rx, 2, SMI_TIMEOUT)){
        fprintf(stderr, "SMI_INIT: X initialization failed\n");
        return -1;
    }
    if(strcmp(rx, "OK")!=0){
        fprintf(stderr, "SMI_INIT: X motor failed to respond.\n");
        return -1;
    }
    // Init Y
    memset(rx,0,sizeof(2));
    if(smi_com(SMI_YCOM, initcmd, rx, 2, SMI_TIMEOUT)){
        fprintf(stderr, "SMI_INIT: X initialization failed\n");
        return -1;
    }
    if(strcmp(rx, "OK")!=0){
        fprintf(stderr, "SMI_INIT: Y motor failed to respond.\n");
        return -1;
    }
    
    return 0;
}

/* Issues position commands to x, y, or xy.
*/
int smi_move_x(double x_mm){
    static char tx[MAXBUFFER];
    sprintf(tx, "P=%d\nG\n", (int) (x_mm * SMI_XTPMM * SMI_CPR));
    return smi_com(SMI_XCOM, tx, rx, 0, SMI_TIMEOUT);
}

int smi_move_y(double y_mm){
    static char tx[MAXBUFFER];
    sprintf(tx, "P=%d\nG\n", (int) (y_mm * SMI_YTPMM * SMI_CPR));
    return smi_com(SMI_YCOM, tx, rx, 0, SMI_TIMEOUT);
}

int smi_move_xy(double x_mm, double y_mm){
    int err;
    static char tx[MAXBUFFER];
    sprintf(tx, "P=%d\nG\n", (int) (x_mm * SMI_XTPMM * SMI_CPR));
    err = smi_com(SMI_XCOM, tx, rx, 0, SMI_TIMEOUT);
    sprintf(tx, "P=%d\nG\n", (int) (y_mm * SMI_YTPMM * SMI_CPR));
    if(smi_com(SMI_YCOM, tx, rx, 0, SMI_TIMEOUT) || err)
        return -1;
    return 0;
}


#endif
