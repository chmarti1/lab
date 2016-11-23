#include "lconfig.h"
#include <unistd.h>         // for usleep/sleep

int main(void){
    DEVCONF dconf[8];
    unsigned int count;
    int err;
    double data[1024];
    
    err = load_config(dconf,8,"test.conf");
    err = err || open_config(dconf,0);
    err = err || upload_config(dconf,0);
    show_config(dconf,0);
    err = err || start_data_stream(dconf,0,64);
    for(count=0; count<64; count+=1)
        err = err || read_data_stream(dconf,0,data);
    err = err || stop_data_stream(dconf,0);
    close_config(dconf,0);
    if(err) printf("ERROR!\n");
    return err;
}
