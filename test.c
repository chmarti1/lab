#include "lconfig.h"
#include <stdio.h>
#include <unistd.h>         // for usleep/sleep

int main(void){
    DEVCONF dconf[8];
    int err;
	FILE *ff;
    
    err = load_config(dconf,8,"test.conf");
    show_config(dconf,0);
	
	ff = fopen("test.dat","w+");
	write_config(dconf,0,ff);
	fclose(ff);

    if(err) printf("ERROR!\n");
    return err;
}
