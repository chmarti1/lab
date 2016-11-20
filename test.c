#include "ldisplay.h"       // For the display helper functions
#include <unistd.h>         // for usleep/sleep

#define MAX_INPUT 128

int main(void){
    char charin = 0,go_f = 1;
    char input[MAX_INPUT];
    unsigned int count = 0;

    clear_terminal();
    setup_keypress();
    print_param(8,30,"Execution count");
    while(go_f){
        count ++;
        print_int(8,30,count);
        LDISP_CGO(10,1);
        fflush(stdout);
        usleep(500000);
        if(prompt_on_keypress(-1,"Enter a new execution count: ",input,MAX_INPUT)){
            clear_terminal();
            print_param(8,30,"Execution count");
            sscanf(input,"%d",&count);
        }
    }
    finish_keypress();
}
