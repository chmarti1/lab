#include "lconfig.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(){
    DEVCONF dconf[3];
    int ii;
    double *data;
    unsigned int channels, samples_per_read, samples_read, samples_streamed, samples_waiting;

    load_config(dconf,3,"test.conf");
    show_config(dconf,0);
    return 0;
}

