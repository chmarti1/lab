#include "lconfig.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(){
    DEVCONF dconf[3];
    int ii;

    load_config(dconf,3,"test.conf");
    open_config(dconf,0);
    upload_config(dconf,0);
    for(ii=0; ii<100; ii++){
        sleep(1);
        update_fio(dconf,0);
        show_config(dconf,0);
    }

    close_config(dconf,0);
    return 0;
}

