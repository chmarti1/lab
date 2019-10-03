#include "lconfig.h"
#include <string.h>
#include <unistd.h>

#define MAXBUFFER 128


void xmit(lc_devconf_t *dconf, char * tx, char * rx, unsigned int rxlength){
    static unsigned char txbuffer[MAXBUFFER], rxbuffer[MAXBUFFER];
    int ii, jj, txlength;
    txlength = strlen(tx);
    for(ii = 0; ii<txlength; ii++){
        jj = ii<<1;
        txbuffer[jj] = 0;
        txbuffer[jj+1] = tx[ii];
        printf("%c", tx[ii]);
    }
    lc_communicate(dconf, 0, txbuffer, txlength<<1, rxbuffer, rxlength<<1, 4000);
    for(ii = 0; ii<rxlength; ii++){
        rx[ii] = rxbuffer[(ii<<1)+1];
    }
}

void main(){
    lc_devconf_t dconf;
    unsigned char rxbuffer[4], txbuffer[4];
    int ii;
    
    lc_load_config(&dconf, 1, "test.conf");
    lc_open(&dconf);
    lc_show_config(&dconf);
    xmit(&dconf, "PRINT(\"S\")\n", rxbuffer, 1);
    printf("%c\n", rxbuffer[0]);
    /*
    xmit(&dconf, "LOAD\n", rxbuffer, 0);
    txbuffer[0] = 255;
    txbuffer[0] = '\n';
    txbuffer[0] = '\0';
    xmit(&dconf, txbuffer, rxbuffer, 0);
    */
    xmit(&dconf, "UCI\nUDI\nZS\n", rxbuffer, 0);
    xmit(&dconf, "MP\nA=100\nV=100000\nP=-50000\nG\nTWAIT\n", rxbuffer, 0);
    lc_close(&dconf);
}
