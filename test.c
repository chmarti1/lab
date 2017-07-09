#include "lconfig.h"
#include <stdio.h>
#include <stdlib.h>

int main(){
    DEVCONF dconf[3];
    FILE* ff;

    load_config(dconf,3,"test.conf");

    ff = fopen("testout.conf","w");
    write_config(dconf,0,ff);
    fclose(ff);
    return 0;
}

