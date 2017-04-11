#include "ldisplay.h"
#include "lconfig.h"
#include <unistd.h>
#include <stdio.h>

#define CONFIG_FILE "drun.conf"
#define PRE_FILE   "drun_pre.dat"
#define POST_FILE   "drun_post.dat"
#define DATA_FILE   "drun.dat"
#define NBUFFER     65535
#define NDEV        3
#define MAXLOOP     2000

int main(){
    int ndev;   // actual number of devices
                // NDEV is the maximum 
    int stream_dev = 0; // which device will be used for streaming?
    char go;    // Flag for whether to continue the stream loop
    unsigned int count; // count the number of loops for safe exit
    DEVCONF dconf[NDEV];

    // Load the configuration
    printf("Loading configuration file...");
    if(load_config(dconf, NDEV, CONFIG_FILE)){
        printf("FAILED\n");
        fprintf(stderr, "DRUN failed while loading the configuration file \"" CONFIG_FILE "\"\n");
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    ndev = ndev_config(dconf, NDEV);

    if(ndev<=0){
        fprintf(stderr,"DRUN did not detect any valid devices for data acquisition.\n");
        return -1;
    }else{
        // Unless additional devices are found, use the first for streaming
        stream_dev = 0;
        printf("Found %d device configurations\n",ndev);
    }

    // Perform the preliminary data collection process
    if(ndev>1){
        // set the streaming device to the second in the array
        stream_dev = 1;
        printf("Performing preliminary static measurements...");
        if(open_config(dconf, 0)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed to open the preliminary data collection device.\n");
            return -1;
        }
        if(upload_config(dconf, 0)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed while configuring preliminary data collection.\n");
            close_config(dconf,0);
            return -1;
        }
        if(start_file_stream(dconf, 0, -1, PRE_FILE)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed to start preliminary data collection.\n");
            close_config(dconf,0);
            return -1;
        }
        if(read_file_stream(dconf, 0)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed while trying to read the preliminary data!\n");
            stop_file_stream(dconf,0);
            close_config(dconf,0);
            return -1;
        }
        if(stop_file_stream(dconf, 0)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed to halt preliminary data collection!\n");
            close_config(dconf,0);
            return -1;
        }
        close_config(dconf,0);
        printf("DONE\n");
    }


    // Perform the streaming data collection process
    printf("Press \"Q\" to quit the process\nStreaming measurements...");
    if(open_config(dconf, stream_dev)){
        printf("FAILED\n");
        fprintf(stderr, "DRUN failed to open the device for streaming.\n");
        return -1;
    }
    if(upload_config(dconf, stream_dev)){
        printf("FAILED\n");
        fprintf(stderr, "DRUN failed while configuring the data stream.\n");
        close_config(dconf,stream_dev);
        return -1;
    }
    if(start_file_stream(dconf, stream_dev, -1, DATA_FILE)){
        printf("FAILED\n");
        fprintf(stderr, "DRUN failed to start the data stream.\n");
        close_config(dconf,stream_dev);
        return -1;
    }

    // Setup the keypress for exit conditions
    setup_keypress();
    go = 1;
    for(count=0; go && count<MAXLOOP; count++){
        if(read_file_stream(dconf, stream_dev)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed while trying to read the data stream!\n");
            stop_file_stream(dconf,stream_dev);
            close_config(dconf,stream_dev);
            finish_keypress();
            return -1;
        }

        // Test for exit conditions
        if(keypress() && getchar() == 'Q')
            go = 0;
    }
    finish_keypress();

    if(stop_file_stream(dconf, stream_dev)){
        printf("FAILED\n");
        fprintf(stderr, "DRUN failed to halt the data stream!\n");
        close_config(dconf,stream_dev);
        return -1;
    }
    close_config(dconf,0);
    printf("DONE\n");

    // Perform the post data collection process
    if(ndev>2){
        printf("Performing post static measurements...");
        if(open_config(dconf, 2)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed to open the post data collection device.\n");
            return -1;
        }
        if(upload_config(dconf, 2)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed while configuring post data collection.\n");
            close_config(dconf,2);
            return -1;
        }
        if(start_file_stream(dconf, 2, -1, POST_FILE)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed to start post data collection.\n");
            close_config(dconf,2);
            return -1;
        }
        if(read_file_stream(dconf, 2)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed while trying to read the post data!\n");
            stop_file_stream(dconf,2);
            close_config(dconf,2);
            return -1;
        }
        if(stop_file_stream(dconf, 2)){
            printf("FAILED\n");
            fprintf(stderr, "DRUN failed to halt post data collection!\n");
            close_config(dconf,2);
            return -1;
        }
        close_config(dconf,2);
        printf("DONE\n");
    }

    printf("Exited successfully.\n");
    return 0;
}
