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
    open_config(dconf,0);
    upload_config(dconf,0);
    start_data_stream(dconf,0,-1);
    printf("%d\n%d\n%d\n", dconf[0].RB.samples_per_read, 
        dconf[0].RB.blocksize_samples, dconf[0].RB.size_samples);
    samples_waiting = 0;
    for(ii=0; ii<20 && samples_streamed<dconf[0].nsample; ii++){
        service_data_stream(dconf,0);
        read_data_stream(dconf,0,&data);
        status_data_stream(dconf,0,&channels, 
            &samples_per_read, &samples_read, &samples_streamed, 
            &samples_waiting);
        printf("%lx, CH:%d SPR:%d SR:%d SS:%d SW:%d\n",(unsigned long int) data,
            channels, samples_per_read, samples_read, samples_streamed, 
            samples_waiting);
    }
    stop_data_stream(dconf,0);
    close_config(dconf,0);
    return 0;
}

