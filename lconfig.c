
#include <stdio.h>      // 
#include <stdlib.h>     // for rand, malloc, and free
#include <string.h>     // for strncmp and strncpy
#include <math.h>       // for sin and cos
#include <limits.h>     // for MIN/MAX values of integers
#include <time.h>       // for file stream time stamps
#include <LabJackM.h>   // duh
#include <stdint.h>     // being careful about bit widths

#include "lconfig.h"


/*....................
.   Globals
....................*/
char err_str[LJM_MAX_NAME_SIZE];

/*.............................
.
.   Algorithm definition
.       "Private" functions
.
.............................*/


void strlower(char* target){
    char *a;
    unsigned int N = 0;
    a = target;
    while( (*a)!='\0' && N<LCONF_MAX_STR){
        if( (*a)>='A' && (*a)<='Z' )
            *a += 'a'-'A';
        a++;
        N++;
    }
}


int array_address(const char* root_address, unsigned int channel){
    int reg, type;
    LJM_NameToAddress(root_address,&reg,&type);
    if(type==0) // 16-bit integer
        reg+=channel;
    else if(type==1 || type==2 || type==3) // 32 bit vars
        reg+=2*channel;
    return reg;
}

/* Depreciated - moved away from explicit addressing
void airegisters(const unsigned int channel, int *reg_negative, int *reg_range, int *reg_resolution){
    int reg_temp, type_temp;
    LJM_NameToAddress("AIN0_NEGATIVE_CH",&reg_temp,&type_temp);
    *reg_negative = reg_temp + channel;
    LJM_NameToAddress("AIN0_RANGE",&reg_temp,&type_temp);
    *reg_range = reg_temp + 2*channel;
    LJM_NameToAddress("AIN0_RESOLUTION_INDEX",&reg_temp,&type_temp);
    *reg_resolution = reg_temp + channel;
}

void aoregisters(const unsigned int buffer,     // The output stream number (not the DAC number)
                int *reg_target,                // STREAM_OUT#_TARGET (This should point to the DAC register) 
                int *reg_buffersize,            // STREAM_OUT#_BUFFER_SIZE (The memory to allocate to the buffer)
                int *reg_enable,                // STREAM_OUT#_ENABLE (Is the output stream enabled?)
                int *reg_buffer,               // STREAM_OUT#_BUFFER_F32 (This is the destination for floating point data)
                int *reg_loopsize,              // STREAM_OUT#_LOOP_SIZE (The number of samples in the loop)
                int *reg_setloop){              // STREAM_OUT#_SET_LOOP (how to interact with the loop; should be 1)
    
    int dummy;
    LJM_NameToAddress("STREAM_OUT0_TARGET",reg_target,&dummy);
    (*reg_target)+=2*buffer;
    LJM_NameToAddress("STREAM_OUT0_BUFFER_SIZE",reg_buffersize,&dummy);
    (*reg_buffersize)+=2*buffer;
    LJM_NameToAddress("STREAM_OUT0_ENABLE",reg_enable,&dummy);
    (*reg_enable)+=2*buffer;
    LJM_NameToAddress("STREAM_OUT0_BUFFER_F32",reg_buffer,&dummy);
    (*reg_buffer)+=2*buffer;
    LJM_NameToAddress("STREAM_OUT0_LOOP_SIZE",reg_loopsize,&dummy);
    (*reg_loopsize)+=2*buffer;
    LJM_NameToAddress("STREAM_OUT0_SET_LOOP",reg_setloop,&dummy);
    (*reg_setloop)+=2*buffer;
}

*/


void print_color(const char* head, char* value1, char* value2){
    if(strncmp(value1,value2,LCONF_MAX_STR)==0)
        printf("%25s: %-6s (" LCONF_COLOR_GREEN "%s" LCONF_COLOR_NULL ")\n", head, value1, value2);
    else
        printf("%25s: %-6s (" LCONF_COLOR_RED "%s" LCONF_COLOR_NULL ")\n", head, value1, value2);
}



void read_param(FILE *source, char *param){
    int tempc;
    // clear the parameter
    // if the fscanf operation fails, this will return gracefully
    param[0] = '\0';
    // Get the next word
    fscanf(source, LCONF_MAX_READ, param);
    // If the next word is the beginning of a comment
    // read to the end of the line and try again
    // If the next word begins with a double-hash, terminate.
    while(param[0]=='#' && param[1]!='#'){
        // Read the rest of the line
        do { tempc = fgetc(source); } while(tempc!='\n' && tempc>=0);
        fscanf(source, LCONF_MAX_READ, param);
    }
}


void init_config(DEVCONF* dconf){
    int ainum, aonum, metanum, fionum;
    // Global configuration
    dconf->connection = -1;
    dconf->serial[0] = '\0';
    dconf->ip[0] = '\0';
    dconf->gateway[0] = '\0';
    dconf->subnet[0] = '\0';
    // Global sample settings
    dconf->samplehz = -1.;
    dconf->settleus = 1.;
    dconf->nsample = LCONF_DEF_NSAMPLE;
    // Trigger settings
    dconf->trigchannel = -1;
    dconf->triglevel = 0.;
    dconf->trigpre = 0;
    dconf->trigpre_collected = 0;
    dconf->trigstate = TRIG_IDLE;
    dconf->trigedge = TRIG_RISING;
    // Filestream
    dconf->FS.buffer = NULL;
    dconf->FS.file = NULL;
    dconf->FS.samples_per_read = 0;
    // Metas
    for(metanum=0; metanum<LCONF_MAX_META; metanum++)
        dconf->meta[metanum].param[0] = '\0';
    // Analog Inputs
    dconf->naich = 0;
    for(ainum=0; ainum<LCONF_MAX_NAICH; ainum++){
        dconf->aich[ainum].channel = -1;
        dconf->aich[ainum].nchannel = LCONF_DEF_AI_NCH;
        dconf->aich[ainum].range = LCONF_DEF_AI_RANGE;
        dconf->aich[ainum].resolution = LCONF_DEF_AI_RES;
    }
    // Analog Outputs
    dconf->naoch = 0;
    for(aonum=0; aonum<LCONF_MAX_NAOCH; aonum++){
        dconf->aoch[aonum].channel = -1;
        dconf->aoch[aonum].frequency = -1;
        dconf->aoch[aonum].signal = AO_CONSTANT;
        dconf->aoch[aonum].amplitude = LCONF_DEF_AO_AMP;
        dconf->aoch[aonum].offset = LCONF_DEF_AO_OFF;
        dconf->aoch[aonum].duty = LCONF_DEF_AO_DUTY;
    }
    // Flexible IO
    dconf->nfioch = 0;
    dconf->fiofrequency = 0.;
    for(fionum=0; fionum<LCONF_MAX_NFIOCH; fionum++){
        dconf->fioch[fionum].channel = -1;
        dconf->fioch[fionum].signal = FIO_NONE;
        dconf->fioch[fionum].edge = FIO_RISING;
        dconf->fioch[fionum].debounce = FIO_DEBOUNCE_NONE;
        dconf->fioch[fionum].direction = FIO_INPUT;
        dconf->fioch[fionum].time = 0.;
        dconf->fioch[fionum].duty = .5;
        dconf->fioch[fionum].phase = 0.;
        dconf->fioch[fionum].counts = 0;
    }
}


void clean_file_stream(FILESTREAM* FS){
    if(FS->file) fclose(FS->file); 
    FS->file=NULL;
    if(FS->buffer) free(FS->buffer);
    FS->buffer = NULL;
    FS->samples_per_read = 0;
}


/*....................................
.
.   Diagnostic
.       "Public" functions
.
......................................*/

int nistream_config(DEVCONF* dconf, const unsigned int devnum){
    int out, fionum;
    out = dconf[devnum].naich;
    for(fionum=0; fionum<dconf[devnum].nfioch; fionum++){
        if(dconf[devnum].fioch[fionum].direction == FIO_INPUT)
            out++;
    }
    return out;
}

int nostream_config(DEVCONF* dconf, const unsigned int devnum){
    int out, fionum;
    out = dconf[devnum].naoch;
    return out;
}

int ndev_config(DEVCONF* dconf, const unsigned int devmax){
    int ii;
    for(ii=0; ii<devmax; ii++)
        if(dconf[ii].connection<0)
            return ii;
    return devmax;
}


/*....................................
.
.   Algorithm Definition
.       "Public" functions
.
......................................*/


int load_config(DEVCONF* dconf, const unsigned int devmax, const char* filename){
    int devnum=-1, ainum=-1, aonum=-1, fionum=-1;
    int itemp;
    float ftemp;
    char param[LCONF_MAX_STR], value[LCONF_MAX_STR], meta[LCONF_MAX_STR];
    char metatype;
    FILE* ff;

    if(devmax>LCONF_MAX_NDEV){
        fprintf(stderr,"LCONFIG: LOAD_CONFIG called with %d devices, but only %d are supported.\n",devmax,LCONF_MAX_NDEV);
        return LCONF_ERROR;
    }

    // First, initialize the configuration array to have safe values.
    for(devnum++;devnum<devmax;devnum++)
        init_config( &dconf[devnum] );

    // Keep track of a meta type flag as a state machine.  If the flag is set
    // to a valid value ('f', 'i', or 's') all unrecognized parameters are 
    // treated as a meta parameter of that type.
    metatype = 'n';

    // start at the beginning.
    devnum = -1;

    // open the file
    ff = fopen(filename, "r");
    if(ff==NULL){
        fprintf(stderr,"LOAD: Failed to open the configuration file \"%s\".\n",filename);
        return LCONF_ERROR;
    }
    // Read the entire file
    while(!feof(ff)){
        // Read in the parameter name
        read_param(ff,param);
        strlower(param);
        // if the "##" termination sequence is discovered
        if(param[0]=='#' && param[1]=='#'){
            fclose(ff);
            return LCONF_NOERR;
        }
        // Read in the parameter's value
        read_param(ff,value);
        strlower(value);
        // if the "##" termination sequence is discovered
        if(param[0]=='#' && param[1]=='#'){
            fprintf(stderr,"LOAD: Unexpected termination \"##\" while parsing parameter \"%s\".\n",param);
            fclose(ff);
            return LCONF_ERROR;
        }

        // Case out the parameters
        //
        // The CONNECTION parameter
        //
        if(streq(param,"connection")){
            // increment the device index
            devnum++;
            if(devnum>=devmax){
                fprintf(stderr,"LOAD: Too many devices specified. Maximum allowed is %d.\n", devmax);
                goto lconf_loadfail;
            }

            // look for usb, eth, or any
            if(strncmp(value,"usb",LCONF_MAX_STR)==0){
                dconf[devnum].connection=LJM_ctUSB;
            }else if(strncmp(value,"eth",LCONF_MAX_STR)==0){
                dconf[devnum].connection=LJM_ctETHERNET;
            }else if(strncmp(value,"any",LCONF_MAX_STR)==0){
                dconf[devnum].connection=LJM_ctANY;
            }else{
                fprintf(stderr,"LOAD: Unrecognized connection type: %s\n",value);
                fprintf(stderr,"%s\n","Expected \"usb\", \"eth\", or \"any\".");
                goto lconf_loadfail;
            }
        //
        // What if CONNECTION isn't first?
        //
        }else if(devnum<0){
            fprintf(stderr,"LOAD: The first configuration parameter must be \"connection\".\n\
Found \"%s\"\n", param);
            return LCONF_ERROR;
        //
        // The IP parameter
        //
        }else if(streq(param,"ip")){
            strncpy(dconf[devnum].ip,value,LCONF_MAX_STR);
        //
        // The GATEWAY parameter
        //
        }else if(streq(param,"gateway")){
            strncpy(dconf[devnum].gateway,value,LCONF_MAX_STR);
        //
        // The SUBNET parameter
        //
        }else if(streq(param,"subnet")){
            strncpy(dconf[devnum].subnet,value,LCONF_MAX_STR);
        //
        // The SERIAL parameter
        //
        }else if(streq(param,"serial")){
            strncpy(dconf[devnum].serial,value,LCONF_MAX_STR);
        //
        // The SAMPLEHZ parameter
        //
        }else if(streq(param,"samplehz")){
            if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: Illegal SAMPLEHZ value \"%s\". Expected float.\n",value);
                goto lconf_loadfail;
            }
            dconf[devnum].samplehz = ftemp;
        //
        // The SETTLEUS parameter
        //
        }else if(streq(param,"settleus")){
            if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: Illegal SETTLEUS value \"%s\". Expected float.\n",value);
                goto lconf_loadfail;
            }
            dconf[devnum].settleus = ftemp;
        //
        // The NSAMPLE parameter
        //
        }else if(streq(param,"nsample")){
            if(sscanf(value,"%d",&itemp)!=1){
                fprintf(stderr,"LOAD: Illegal NSAMPLE value \"%s\".  Expected integer.\n",value);
                goto lconf_loadfail;
            }else if(itemp <= 0)
                fprintf(stderr,"LOAD: **WARNING** NSAMPLE value was less than or equal to zero.\n"
                       "      Fix this before initiating a stream!\n");
            dconf[devnum].nsample = itemp;
        //
        // The AICHANNEL parameter
        //
        }else if(streq(param,"aichannel")){
            ainum = dconf[devnum].naich;
            // Check for an array overrun
            if(ainum>=LCONF_MAX_NAICH){
                fprintf(stderr,"LOAD: Too many AIchannel definitions.  Only %d are allowed.\n",LCONF_MAX_NAICH);
                goto lconf_loadfail;
            // Make sure the channel number is a valid integer
            }else if(sscanf(value,"%d",&itemp)!=1){
                fprintf(stderr,"LOAD: Illegal AIchannel number \"%s\". Expected integer.\n",value);
                goto lconf_loadfail;
            }
            // Make sure the channel number is in range for the T7.
            if(itemp<0 || itemp>LCONF_MAX_AICH){
                fprintf(stderr,"LOAD: AIchannel number %d is out of range [0-%d].\n", itemp, LCONF_MAX_AICH);
                goto lconf_loadfail;
            }
            // increment the number of active channels
            dconf[devnum].naich++;
            // Set the channel and all the default parameters
            dconf[devnum].aich[ainum].channel = itemp;
            dconf[devnum].aich[ainum].range = LCONF_DEF_AI_RANGE;
            dconf[devnum].aich[ainum].resolution = LCONF_DEF_AI_RES;
            dconf[devnum].aich[ainum].nchannel = LCONF_DEF_AI_NCH;
        //
        // The AIRANGE parameter
        //
        }else if(streq(param,"airange")){
            ainum = dconf[devnum].naich-1;
            // Test that there is a configured channel already
            if(ainum<0){
                fprintf(stderr,"LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                goto lconf_loadfail;
            }
            if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: Illegal AIrange number \"%s\". Expected a float.\n",value);
                goto lconf_loadfail;
            }
            dconf[devnum].aich[ainum].range = ftemp;
        //
        // The AINEGATIVE parameter
        //
        }else if(streq(param,"ainegative")){
            ainum = dconf[devnum].naich-1;
            // Test that there is a configured channel already
            if(ainum<0){
                fprintf(stderr,"LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                goto lconf_loadfail;
            }
            // if value is a string specifying ground
            if(strncmp(value,"ground",LCONF_MAX_STR)==0){
                itemp = LJM_GND;
            // If the value is a string specifying a differential connection
            }else if(strncmp(value,"differential",LCONF_MAX_STR)==0){
                // set the channel to be one greater than the primary aichannel.
                itemp = dconf[devnum].aich[ainum].channel+1;
            }else if(sscanf(value,"%d",&itemp)!=1){
                fprintf(stderr,"LOAD: Illegal AIneg index \"%s\". Expected an integer, \"differential\", or \"ground\".\n",value);
                goto lconf_loadfail;
            }else if(itemp == LJM_GND){
                // bypass further error checking if the index referrs to ground.
            }else if(itemp < 0 || itemp > LCONF_MAX_AICH){
                fprintf(stderr,"LOAD: AIN negative channel number %d is out of range [0-%d].\n", itemp, LCONF_MAX_AICH);
                goto lconf_loadfail;
            }else if(itemp%2-1){
                fprintf(stderr,"LOAD: Even channels cannot serve as negative channels\n\
and negative channels cannot opperate in differential mode.\n");
                goto lconf_loadfail;
            }else if(itemp-dconf[devnum].aich[ainum].channel!=1){
                fprintf(stderr,"LOAD: Illegal choice of negative channel %d for positive channel %d.\n\
Negative channels must be the odd channel neighboring the\n\
even channels they serve.  (e.g. AI0/AI1)\n", itemp, dconf[devnum].aich[ainum].channel);
                goto lconf_loadfail;
            }
            dconf[devnum].aich[ainum].nchannel = itemp;
        //
        // The AIRESOLUTION parameter
        //
        }else if(streq(param,"airesolution")){
            ainum = dconf[devnum].naich-1;
            if(ainum<0){
                fprintf(stderr,"LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                goto lconf_loadfail;
            }

            if(sscanf(value,"%d",&itemp)!=1){
                fprintf(stderr,"LOAD: Illegal AIres index \"%s\". Expected an integer.\n",value);
                goto lconf_loadfail;
            }else if(itemp < 0 || itemp > LCONF_MAX_AIRES){
                fprintf(stderr,"LOAD: AIres index %d is out of range [0-%d].\n", itemp, LCONF_MAX_AIRES);
                goto lconf_loadfail;
            }
            dconf[devnum].aich[ainum].resolution = itemp;
        //
        // The AOCHANNEL parameter
        //
        }else if(streq(param,"aochannel")){
            aonum = dconf[devnum].naoch;
            // Check for an array overrun
            if(aonum>=LCONF_MAX_NAOCH){
                fprintf(stderr,"LOAD: Too many AOchannel definitions.  Only %d are allowed.\n",LCONF_MAX_NAOCH);
                goto lconf_loadfail;
            // Make sure the channel number is a valid integer
            }else if(sscanf(value,"%d",&itemp)!=1){
                fprintf(stderr,"LOAD: Illegal AOchannel number \"%s\". Expected integer.\n",value);
                goto lconf_loadfail;
            }
            // Make sure the channel number is in range for the T7.
            if(itemp<0 || itemp>LCONF_MAX_AOCH){
                fprintf(stderr,"LOAD: AOchannel number %d is out of range [0-%d].\n", itemp, LCONF_MAX_AOCH);
                goto lconf_loadfail;
            }
            // increment the number of active channels
            dconf[devnum].naoch++;
            // Set the channel and all the default parameters
            dconf[devnum].aoch[aonum].channel = itemp;
            dconf[devnum].aoch[aonum].signal = AO_CONSTANT;
            dconf[devnum].aoch[aonum].amplitude = LCONF_DEF_AO_AMP;
            dconf[devnum].aoch[aonum].offset = LCONF_DEF_AO_OFF;
            dconf[devnum].aoch[aonum].duty = LCONF_DEF_AO_DUTY;
        //
        // The AOSIGNAL parameter
        //
        }else if(streq(param,"aosignal")){
            aonum = dconf[devnum].naoch-1;
            if(aonum<0){
                fprintf(stderr,"LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                goto lconf_loadfail;
            }
            // Case out valid AOsignal values
            if(strncmp(value,"constant",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = AO_CONSTANT;
            else if(strncmp(value,"sine",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = AO_SINE;
            else if(strncmp(value,"square",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = AO_SQUARE;
            else if(strncmp(value,"triangle",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = AO_TRIANGLE;
            else if(strncmp(value,"noise",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = AO_NOISE;
            else{
                fprintf(stderr,"LOAD: Illegal AOsignal type: %s\n",value);
                goto lconf_loadfail;
            }
        //
        // AOFREQUENCY
        //
        }else if(streq(param,"aofrequency")){
            aonum = dconf[devnum].naoch-1;
            if(aonum<0){
                fprintf(stderr,"LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                goto lconf_loadfail;
            // Convert the string into a float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: AOfrequency expected float, but found: %s\n", value);
                goto lconf_loadfail;
            }else if(ftemp<=0){
                fprintf(stderr,"LOAD: AOfrequency must be positive.  Found: %f\n", ftemp);
                goto lconf_loadfail;
            }
            dconf[devnum].aoch[aonum].frequency = ftemp;
        //
        // AOAMPLITUDE
        //
        }else if(streq(param,"aoamplitude")){
            aonum = dconf[devnum].naoch-1;
            if(aonum<0){
                fprintf(stderr,"LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                goto lconf_loadfail;
            // Convert the string into a float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: AOamplitude expected float, but found: %s\n", value);
                goto lconf_loadfail;
            }
            dconf[devnum].aoch[aonum].amplitude = ftemp;
        //
        // AOOFFSET
        //
        }else if(streq(param,"aooffset")){
            aonum = dconf[devnum].naoch-1;
            if(aonum<0){
                fprintf(stderr,"LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                goto lconf_loadfail;
            // Convert the string into a float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: AOoffset expected float, but found: %s\n", value);
                goto lconf_loadfail;
            }else if(ftemp<0. || ftemp>5.){
                fprintf(stderr,"LOAD: AOoffset must be between 0 and 5 volts.  Found %f.", ftemp);
                goto lconf_loadfail;
            }
            dconf[devnum].aoch[aonum].offset = ftemp;
        //
        // AODUTY parameter
        //
        }else if(streq(param,"aoduty")){
            aonum = dconf[devnum].naoch-1;
            if(aonum<0){
                fprintf(stderr,"LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                goto lconf_loadfail;
            // Convert the string into a float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: AOduty expected float but found: %s\n", value);
                goto lconf_loadfail;
            }else if(ftemp<0. || ftemp>1.){
                fprintf(stderr,"LOAD: AOduty must be between 0. and 1.  Found %f.\n", ftemp);
                goto lconf_loadfail;
            }
            dconf[devnum].aoch[aonum].duty = ftemp;
        //
        // TRIGCHANNEL parameter
        //
        }else if(streq(param,"trigchannel")){
            if(sscanf(value,"%d",&itemp)!=1){
                fprintf(stderr,"LOAD: TRIGchannel expected an integer channel number but found: %s\n", 
                    value);
                goto lconf_loadfail;
            }else if(itemp<0){
                fprintf(stderr,"LOAD: TRIGchannel must be non-negative.\n");
                goto lconf_loadfail;
            }
            dconf[devnum].trigchannel = itemp;
        //
        // TRIGLEVEL parameter
        //
        }else if(streq(param,"triglevel")){
            if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: TRIGlevel expected a floating point voltage but found: %s\n", 
                    value);
                goto lconf_loadfail;
            }
            dconf[devnum].triglevel = ftemp;
        //
        // TRIGEDGE parameter
        //
        }else if(streq(param,"trigedge")){
            if(streq(value,"rising"))
                dconf[devnum].trigedge = TRIG_RISING;
            else if(streq(value,"falling"))
                dconf[devnum].trigedge = TRIG_FALLING;
            else if(streq(value,"all"))
                dconf[devnum].trigedge = TRIG_ALL;
            else{
                fprintf(stderr,"LOAD: Unrecognized TRIGedge parameter: %s\n", value);
                goto lconf_loadfail;
            }
        //
        // FIOFREQUENCY parameter
        //
        }else if(streq(param,"fiofrequency")){
            if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: Got illegal FIOfrequency: %s\n",value);
                goto lconf_loadfail; 
            }else if(ftemp <= 0.){
                fprintf(stderr,"LOAD: FIOfrequency must be positive!\n");
                goto lconf_loadfail;
            }
            dconf[devnum].fiofrequency = ftemp;
        //
        // FIOCHANNEL parameter
        //
        }else if(streq(param,"fiochannel")){
            fionum = dconf[devnum].nfioch;
            // Check for an array overrun
            if(fionum>=LCONF_MAX_FIOCH){
                fprintf(stderr,"LOAD: Too many FIOchannel definitions.  Only %d are allowed.\n",LCONF_MAX_FIOCH);
                goto lconf_loadfail;
            // Make sure the channel number is a valid integer
            }else if(sscanf(value,"%d",&itemp)!=1){
                fprintf(stderr,"LOAD: Illegal FIOchannel number \"%s\". Expected integer.\n",value);
                goto lconf_loadfail;
            }
            // Make sure the channel number is in range for the T7.
            if(itemp<0 || itemp>LCONF_MAX_FIOCH){
                fprintf(stderr,"LOAD: FIOchannel number %d is out of range [0-%d].\n", itemp, LCONF_MAX_FIOCH);
                goto lconf_loadfail;
            }
            // increment the number of active channels
            dconf[devnum].nfioch++;
            // set the channel number
            dconf[devnum].fioch[fionum].channel = itemp;
            // initialize the other parameters
            dconf[devnum].fioch[fionum].signal = FIO_NONE;
            dconf[devnum].fioch[fionum].edge = FIO_RISING;
            dconf[devnum].fioch[fionum].debounce = FIO_DEBOUNCE_NONE;
            dconf[devnum].fioch[fionum].direction = FIO_INPUT;
            dconf[devnum].fioch[fionum].time = LCONF_DEF_FIO_TIMEOUT;
            dconf[devnum].fioch[fionum].phase = 0.;
            dconf[devnum].fioch[fionum].duty = 0.5;
        //
        // FIOSIGNAL parameter
        //
        }else if(streq(param,"fiosignal")){
            if(streq(value,"pwm")){
                dconf[devnum].fioch[fionum].signal = FIO_PWM;
            }else if(streq(value,"count")){
                dconf[devnum].fioch[fionum].signal = FIO_COUNT;
            }else if(streq(value,"frequency")){
                dconf[devnum].fioch[fionum].signal = FIO_FREQUENCY;
            }else if(streq(value,"phase")){
                dconf[devnum].fioch[fionum].signal = FIO_PHASE;
            }else if(streq(value,"quadrature")){
                dconf[devnum].fioch[fionum].signal = FIO_QUADRATURE;
            }else{
                fprintf(stderr,"LOAD: Illegal FIO signal: %s\n",value);
                goto lconf_loadfail; 
            }
        //
        // FIOEDGE
        //
        }else if(streq(param,"fioedge")){
            if(streq(value,"rising")){
                dconf[devnum].fioch[fionum].edge = FIO_RISING;
            }else if(streq(value,"falling")){
                dconf[devnum].fioch[fionum].edge = FIO_FALLING;
            }else if(streq(value,"all")){
                dconf[devnum].fioch[fionum].edge = FIO_ALL;
            }else{
                fprintf(stderr,"LOAD: Illegal FIO edge: %s\n",value);
                goto lconf_loadfail; 
            }
        //
        // FIODEBOUNCE
        //
        }else if(streq(param,"fiodebounce")){
            if(streq(value,"none")){
                dconf[devnum].fioch[fionum].debounce = FIO_DEBOUNCE_NONE;
            }else if(streq(value,"fixed")){
                dconf[devnum].fioch[fionum].debounce = FIO_DEBOUNCE_FIXED;
            }else if(streq(value,"restart")){
                dconf[devnum].fioch[fionum].debounce = FIO_DEBOUNCE_RESTART;
            }else if(streq(value,"minimum")){
                dconf[devnum].fioch[fionum].debounce = FIO_DEBOUNCE_MINIMUM;
            }else{
                fprintf(stderr,"LOAD: Illegal FIO debounce mode: %s\n",value);
                goto lconf_loadfail;
            }
        //
        // FIODIRECTION
        //
        }else if(streq(param,"fiodirection")){
            if(streq(value,"input")){
                dconf[devnum].fioch[fionum].direction = FIO_INPUT;
            }else if(streq(value,"output")){
                dconf[devnum].fioch[fionum].direction = FIO_OUTPUT;
            }else{
                fprintf(stderr,"LOAD: Illegal FIO direction: %s\n",value);
                goto lconf_loadfail;
            }
        //
        // FIOusec
        //
        }else if(streq(param,"fiousec")){
            if(sscanf(value,"%f",&ftemp)==1){
                dconf[devnum].fioch[fionum].time = ftemp;
            }else{
                fprintf(stderr,"LOAD: Illegal FIO time parameter. Found: %s\n",value);
                goto lconf_loadfail;
            }
        //
        // FIOdegrees
        //
        }else if(streq(param,"fiodegrees")){
            if(sscanf(value,"%f",&ftemp)==1){
                dconf[devnum].fioch[fionum].phase = ftemp;
            }else{
                fprintf(stderr,"LOAD: Illegal FIO phase parameter. Found: %s\n",value);
                goto lconf_loadfail;
            }
        //
        // FIODUTY
        //
        }else if(streq(param,"fioduty")){
            if(sscanf(value,"%f",&ftemp)!=1){
                fprintf(stderr,"LOAD: Illegal FIO duty cycle. Found: %s\n",value);
                goto lconf_loadfail;
            }else if(ftemp<0. || ftemp>1.){
                fprintf(stderr,"LOAD: Illegal FIO duty cycles must be between 0 and 1. Found %f\n",ftemp);
                goto lconf_loadfail;
            }
            dconf[devnum].fioch[fionum].duty = ftemp;
        //
        // META parameter: start/stop a meta stanza
        //
        }else if(streq(param,"meta")){
            if(streq(value,"str") || streq(value,"string"))
                metatype = 's';
            else if(streq(value,"int") || streq(value,"integer"))
                metatype = 'i';
            else if(streq(value,"flt") || streq(value,"float"))
                metatype = 'f';
            else if(streq(value,"end") || \
                    streq(value,"none") || \
                    streq(value,"stop"))
                metatype = 'n';
            else{
                fprintf(stderr,"LOAD: Illegal meta type: %s.\n",value);
                goto lconf_loadfail;
            }
        // META integer configuration
        }else if(strncmp(param,"int:",4)==0){
            sscanf(value,"%d",&itemp);
            put_meta_int(dconf, devnum, &param[4], itemp);
        // META float configuration
        }else if(strncmp(param,"flt:",4)==0){
            sscanf(value,"%f",&ftemp);
            put_meta_flt(dconf, devnum, &param[4], ftemp);
        // META string configuration
        }else if(strncmp(param,"str:",4)==0){
            put_meta_str(dconf, devnum, &param[4], value);
        //
        // META values defined in a stanza
        //
        }else if(metatype == 'i'){
            sscanf(value,"%d",&itemp);
            put_meta_int(dconf, devnum, param, itemp);
        }else if(metatype == 'f'){
            sscanf(value,"%f",&ftemp);
            put_meta_flt(dconf, devnum, param, ftemp);
        }else if(metatype == 's'){
            put_meta_str(dconf, devnum, param, value);
        // Deal gracefully with empty cases; usually EOF.
        }else if(   strncmp(param,"",LCONF_MAX_STR)==0 &&
                    strncmp(value,"",LCONF_MAX_STR)==0){
            //ignore empty strings
        // Deal with unhandled parameters
        }else{
            fprintf(stderr,"LOAD: Unrecognized parameter name: \"%s\"\n", param);
            goto lconf_loadfail;
        }
    }

    fclose(ff);
    return LCONF_NOERR;

    lconf_loadfail:
    fprintf(stderr,"LCONFIG: Error while parsing device %d in file \"%s\"",\
                    devnum,filename);
    fclose(ff);
    return LCONF_ERROR;
}




void write_config(DEVCONF* dconf, const unsigned int devnum, FILE* ff){
    DEVCONF* device;
    AICONF* aich;
    int ainum,aonum, fionum, metanum,temp;
    char mflt, mint, mstr;

    fprintf(ff,"# Configuration automatically generated by WRITE_CONFIG()\n");

    if(dconf[devnum].connection==LJM_ctUSB)
        fprintf(ff,"connection usb\n");
    else
        fprintf(ff,"connection eth\n");

    write_str(serial,serial)
    write_str(ip,ip)
    write_str(gateway,gateway)
    write_str(subnet,subnet)
    write_flt(samplehz,samplehz)
    write_flt(settleus,settleus)
    write_int(nsample,nsample)
    
    // Analog inputs
    if(dconf[devnum].naich)
        fprintf(ff,"\n# Analog Inputs\n");
    for(ainum=0; ainum<dconf[devnum].naich; ainum++){

        write_aiint(aichannel,channel)
        write_aiint(ainegative,nchannel)
        write_aiflt(airange,range)
        write_aiint(airesolution,resolution)
        fprintf(ff,"\n");
    }

    // Analog outputs
    if(dconf[devnum].naoch)
        fprintf(ff,"# Analog Outputs\n");
    for(aonum=0; aonum<dconf[devnum].naoch; aonum++){
        write_aoint(aochannel,channel)
        if(dconf[devnum].aoch[aonum].signal==AO_CONSTANT)  fprintf(ff,"aosignal constant\n");
        else if(dconf[devnum].aoch[aonum].signal==AO_SINE)  fprintf(ff,"aosignal sine\n");
        else if(dconf[devnum].aoch[aonum].signal==AO_SQUARE)  fprintf(ff,"aosignal square\n");
        else if(dconf[devnum].aoch[aonum].signal==AO_TRIANGLE)  fprintf(ff,"aosignal triangle\n");
        else if(dconf[devnum].aoch[aonum].signal==AO_NOISE)  fprintf(ff,"aosignal noise\n");
        else fprintf(ff,"signal ***\n");
        write_aoflt(aofrequency,frequency)
        write_aoflt(aoamplitude,amplitude)
        write_aoflt(aooffset,offset)
        write_aoflt(aoduty,duty)
        fprintf(ff,"\n");
    }

    // FIO CHANNELS
    if(dconf[devnum].nfioch){
        fprintf(ff,"# Flexible Input/Output\n");
        // Frequency
        write_flt(fiofrequency,fiofrequency)
    }
    for(fionum=0; fionum<dconf[devnum].nfioch; fionum++){
        // Channel Number
        write_fioint(fiochannel,channel)

        // Input/output
        if(dconf[devnum].fioch[fionum].direction==FIO_INPUT)
            fprintf(ff, "fiodirection input\n");
        else
            fprintf(ff, "fiodirection output\n");

        // Signal type
        if(dconf[devnum].fioch[fionum].signal==FIO_NONE){
            fprintf(ff, "fiosignal none\n");
        }else if(dconf[devnum].fioch[fionum].signal==FIO_PWM){
            fprintf(ff, "fiosignal pwm\n");
            // Duty
            write_fioflt(fioduty,duty)
            // Phase
            write_fioflt(fiodegrees,phase)
        }else if(dconf[devnum].fioch[fionum].signal==FIO_COUNT){
            fprintf(ff, "fiosignal count\n");
        }else if(dconf[devnum].fioch[fionum].signal==FIO_FREQUENCY){
            fprintf(ff, "fiosignal frequency\n");
        }else if(dconf[devnum].fioch[fionum].signal==FIO_PHASE){
            fprintf(ff, "fiosignal phase\n");
        }else if(dconf[devnum].fioch[fionum].signal==FIO_QUADRATURE){
            fprintf(ff, "fiosignal quadrature\n");
        }
        // Debounce (None is the default - do nothing)
        if(dconf[devnum].fioch[fionum].debounce==FIO_DEBOUNCE_FIXED)
            fprintf(ff, "fiodebounce fixed\n");
        else if(dconf[devnum].fioch[fionum].debounce==FIO_DEBOUNCE_RESTART)
            fprintf(ff, "fiodebounce restart\n");
        else if(dconf[devnum].fioch[fionum].debounce==FIO_DEBOUNCE_MINIMUM)
            fprintf(ff, "fiodebounce minimum\n");
        // Edge type (rising is the default - do nothing)
        if(dconf[devnum].fioch[fionum].edge==FIO_FALLING)  
            fprintf(ff, "fioedge falling\n");
        else if(dconf[devnum].fioch[fionum].edge==FIO_ALL)  
            fprintf(ff, "fioedge all\n");
        // Timeout
        if(dconf[devnum].fioch[fionum].time){
            write_fioflt(fiousec,time)
        }
    }

    // Write the meta parameters in stanzas
    // First, detect whether and which meta parameters there are
    mflt = 0; mint = 0; mstr = 0;
    for(metanum=0; metanum<LCONF_MAX_META && \
            dconf[devnum].meta[metanum].param[0]!='\0'; metanum++){
        if(dconf[devnum].meta[metanum].type=='i')
            mint = 1;
        else if(dconf[devnum].meta[metanum].type=='f')
            mflt = 1;
        else if(dconf[devnum].meta[metanum].type=='s')
            mstr = 1;
    }

    if(mflt || mint || mstr)
        fprintf(ff,"# Meta Parameters");

    // Write the integers
    if(mint){
        fprintf(ff,"\nmeta integer\n");
        for(metanum=0; metanum<LCONF_MAX_META && \
                dconf[devnum].meta[metanum].param[0]!='\0'; metanum++)
            if(dconf[devnum].meta[metanum].type=='i')
                fprintf(ff,"%s %d\n",
                    dconf[devnum].meta[metanum].param,
                    dconf[devnum].meta[metanum].value.ivalue);
    }

    // Write the floats
    if(mflt){
        fprintf(ff,"\nmeta float\n");
        for(metanum=0; metanum<LCONF_MAX_META && \
                dconf[devnum].meta[metanum].param[0]!='\0'; metanum++)
            if(dconf[devnum].meta[metanum].type=='f')
                fprintf(ff,"%s %f\n",
                    dconf[devnum].meta[metanum].param,
                    dconf[devnum].meta[metanum].value.fvalue);
    }

    // Write the strings
    if(mstr){
        fprintf(ff,"\nmeta string\n");
        for(metanum=0; metanum<LCONF_MAX_META && \
                dconf[devnum].meta[metanum].param[0]!='\0'; metanum++)
            if(dconf[devnum].meta[metanum].type=='s')
                fprintf(ff,"%s %s\n",
                    dconf[devnum].meta[metanum].param,
                    dconf[devnum].meta[metanum].value.svalue);
    }

    // End the stanza
    if(mflt || mint || mstr)
        fprintf(ff,"meta end\n");
    fprintf(ff,"\n## End Configuration ##\n");
}



int open_config(DEVCONF* dconf, const unsigned int devnum){
    int err,done=LCONF_NOERR;
    char *id;
    const static char any[4] = "ANY";


    // If the connection is USB, use the serial number and ignore the IP
    if(dconf[devnum].connection==LJM_ctUSB){
        if(dconf[devnum].serial[0]!='\0')
            id=dconf[devnum].serial;
        else
            id= (char*) any;
    }else if(dconf[devnum].connection==LJM_ctETHERNET){
        if(dconf[devnum].ip[0]!='\0')
            id=dconf[devnum].ip;
        else if(dconf[devnum].serial[0]!='\0')
            id=dconf[devnum].serial;
        else
            id= (char*) any;
    }
    
    if(id==any)
        fprintf(stderr,"OPEN: Device %d was requested without a \
device identifier.\nResults may be inconsistent.\n",devnum);

    err = LJM_Open(LJM_dtT7, dconf[devnum].connection, id, &dconf[devnum].handle);

    if(err){
        fprintf(stderr,"LCONFIG: Error opening device %d with identifier \"%s\"\n",devnum,id);
        LJM_ErrorToString(err, err_str);
        fprintf(stderr,"%s\n",err_str);
        done=LCONF_ERROR;
    }
    return done;
}




int close_config(DEVCONF* dconf, const unsigned int devnum){
    int err;
    err = LJM_Close(dconf[devnum].handle);
    if(err){
        fprintf(stderr,"LCONFIG: Error closing device %d.\n",devnum);
        LJM_ErrorToString(err, err_str);
        fprintf(stderr,"%s\n",err_str);
    }
    return err;
}





int upload_config(DEVCONF* dconf, const unsigned int devnum){
    int err,handle;             // integers for interacting with the device
    int reg_temp, type_temp;    // register and type holding variables
    // Regsisters for analog input
    int ainum, naich;
    // Registers for analog output
    int aonum,naoch;
    // Registers for fio configuration
    int fionum,nfioch;
    unsigned int fio_clk_roll, fio_clk_div;
    unsigned int samples;
    double ei,er,gi,gr;     // exponent and generator (real and imaginary)
    int dummy;  // a dummy variable
    char flag;
    double ftemp, ftemp1, ftemp2;   // temporary floating points
    unsigned int itemp, itemp1, itemp2;
    char stemp[LCONF_MAX_STR];   // temporary string
    unsigned int index;   // temporary integers
    unsigned int channel, target;

    naich = dconf[devnum].naich;
    naoch = dconf[devnum].naoch;
    handle = dconf[devnum].handle;

    // Global parameters...

    // Test the connection and retrieve connection type
    err = LJM_GetHandleInfo(handle, 
                            &dummy,
                            &reg_temp,
                            &dummy,
                            &dummy,
                            &dummy,
                            &dummy);
    if(err){
        fprintf(stderr,"UPLOAD: No answer from device handle %d.\n", handle);
        goto lconf_uploadfail;
    }

    // If the conneciton is USB, assert the IP parameters
    if(reg_temp == LJM_ctUSB){
        flag=0;
        if(!LJM_IPToNumber(dconf[devnum].ip, &itemp)){
            err = LJM_eWriteName(handle, "ETHERNET_IP_DEFAULT", (double) itemp);
            flag=1;
        }
        if(!err && !LJM_IPToNumber(dconf[devnum].gateway, &itemp)){
            err = LJM_eWriteName(handle, "ETHERNET_GATEWAY_DEFAULT", (double) itemp);
            flag=1;
        }
        if(!err && !LJM_IPToNumber(dconf[devnum].subnet, &itemp)){
            err = LJM_eWriteName(handle, "ETHERNET_SUBNET_DEFAULT", (double) itemp);
            flag=1;
        }
        // if any of the IP parameters were asserted
        // force a static IP configuration, and cycle the ethernet power
        if(!err && flag){
            LJM_eWriteName(handle, "ETHERNET_DHCP_ENABLE_DEFAULT", 0.);
            LJM_eWriteName(handle, "POWER_ETHERNET", 0.);
            err = LJM_eWriteName(handle, "POWER_ETHERNET", 1.);
        }
        if(err){
            fprintf(stderr,"UPLOAD: Failed to assert ethernet parameters on device with handle %d.\n",handle);
            goto lconf_uploadfail;
        }
    }

    // Loop through the analog input channels.
    for(ainum=0;ainum<naich;ainum++){
        channel = dconf[devnum].aich[ainum].channel;
        // Perform a sanity check on the channel number
        if(channel>=LCONF_MAX_AICH){
            fprintf(stderr,"UPLOAD: Analog input %d points to channel %d, which is out of range [0-%d]\n",
                    ainum, channel, LCONF_MAX_AICH);
            goto lconf_uploadfail;
        }
        /* Determine AI register addresses - depreciated - the new code relies on the LJM naming system
        airegisters(channel,&reg_negative,&reg_range,&reg_resolution);
        */

        // Write the negative channel
        stemp[0] = '\0';
        sprintf(stemp, "AIN%d_NEGATIVE_CH", channel);
        err = LJM_eWriteName( handle, stemp, 
                dconf[devnum].aich[ainum].nchannel);
        /* Depreciated - used the address instead of the register name
        err = LJM_eWriteAddress(  handle,
                            reg_negative,
                            LJM_UINT16,
                            dconf[devnum].aich[ainum].nchannel);
        */
        if(err){
            fprintf(stderr,"UPLOAD: Failed to update analog input %d, AI%d negative input to %d\n", 
                    ainum, channel, dconf[devnum].aich[ainum].nchannel);
            goto lconf_uploadfail;
        }
        // Write the range
        stemp[0] = '\0';
        sprintf(stemp, "AIN%d_RANGE", channel);
        err = LJM_eWriteName( handle, stemp, 
                dconf[devnum].aich[ainum].range);
        /* Depreciated - used address instead of register name
        err = LJM_eWriteAddress(  handle,
                            reg_range,
                            LJM_FLOAT32,
                            dconf[devnum].aich[ainum].range);
        */
        if(err){
            fprintf(stderr,"UPLOAD: Failed to update analog input %d, AI%d range input to %f\n", 
                    ainum, channel, dconf[devnum].aich[ainum].range);
            goto lconf_uploadfail;
        }
        // Write the resolution
        stemp[0] = '\0';
        sprintf(stemp, "AIN%d_RESOLUTION_INDEX", channel);
        err = LJM_eWriteName( handle, stemp, 
                dconf[devnum].aich[ainum].resolution);
        /* Depreciated - used the address instead of the register name
        err = LJM_eWriteAddress(  handle,
                            reg_resolution,
                            LJM_UINT16,
                            dconf[devnum].aich[ainum].resolution);
        */
        if(err){
            fprintf(stderr,"UPLOAD: Failed to update analog input %d, AI%d resolution index to %d\n", 
                    ainum, channel, dconf[devnum].aich[ainum].resolution);
            goto lconf_uploadfail;
        }

    }

    // Set up all analog outputs
    for(aonum=0;aonum<naoch;aonum++){
        /*
        // Find the address of the corresponding channel number
        // Start with DAC0 and increment by 2 for each channel number
        LJM_NameToAddress("DAC0",&reg_temp,&type_temp);
        channel = reg_temp + 2*dconf[devnum].aoch[aonum].channel;
        */
        channel = dconf[devnum].aoch[aonum].channel;
        stemp[0] = '\0';
        sprintf(stemp, "DAC%d", channel);
        LJM_NameToAddress(stemp, &target, &type_temp);
        /* Depreciated - used register address instead of name
        // Now, get the buffer configuration registers
        aoregisters(aonum, &reg_target, &reg_buffersize, &reg_enable, &reg_buffer, &reg_loopsize, &reg_setloop);
        */
        // Disable the output buffer
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_ENABLE", aonum);
        err = LJM_eWriteName( handle, stemp, 0);
        //err = LJM_eWriteAddress(handle, reg_enable, LJM_UINT32, 0);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to disable analog output buffer STREAM_OUT%d_ENABLE\n", 
                    aonum);
            goto lconf_uploadfail;
        }
        // Configure the stream target to point to the DAC channel
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_TARGET", aonum);
        err = LJM_eWriteName( handle, stemp, target);
        //err = LJM_eWriteAddress(handle, reg_target, LJM_UINT32, target);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write target AO%d (%d), to STREAM_OUT%d_TARGET\n", 
                    channel, target, aonum);
            goto lconf_uploadfail;
        }
        // Configure the buffer size
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_BUFFER_SIZE", aonum);
        err = LJM_eWriteName( handle, stemp, LCONF_MAX_AOBUFFER);
        //err = LJM_eWriteAddress(handle, reg_buffersize, LJM_UINT32, LCONF_MAX_AOBUFFER);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write AO%d buffer size, %d, to STREAM_OUT%d_BUFFER_SIZE\n", 
                    channel, LCONF_MAX_AOBUFFER, aonum);
            goto lconf_uploadfail;
        }

        // Enable the output buffer
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_ENABLE", aonum);
        err = LJM_eWriteName( handle, stemp, 1);
        //err = LJM_eWriteAddress(handle, reg_enable, LJM_UINT32, 1);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to enable analog output buffer STREAM_OUT%d_ENABLE\n", 
                    aonum);
            goto lconf_uploadfail;
        }

        // Construct the buffer data
        // Restrict the signal period to an integer multiple of sample periods
        if(dconf[devnum].aoch[aonum].frequency<=0.){
            fprintf(stderr,"UPLOAD: Analog output %d has zero or negative frequency.\n",aonum);
            goto lconf_uploadfail;
        }
        samples = (unsigned int)(dconf[devnum].samplehz / dconf[devnum].aoch[aonum].frequency);
        // samples * 2 will be the buffer size
        if(samples*2 > LCONF_MAX_AOBUFFER){
            fprintf(stderr,"UPLOAD: Analog output %d signal will require too many samples at this frequency.\n",aonum);
            goto lconf_uploadfail;
        }
        // Get the buffer address
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_BUFFER", aonum);
        LJM_NameToAddress(stemp, &reg_temp, &type_temp);

        // Case out the different signal types
        // AO_CONSTANT
        if(dconf[devnum].aoch[aonum].signal==AO_CONSTANT){
            for(index=0; index<samples; index++)
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp,\
                    dconf[devnum].aoch[aonum].offset);
        // AO_SINE
        }else if(dconf[devnum].aoch[aonum].signal==AO_SINE){
            // initialize generators
            gr = 1.; gi = 0;
            ftemp = TWOPI / samples;
            er = cos( ftemp );
            ei = sin( ftemp );
            for(index=0; index<samples; index++){
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp,\
                    dconf[devnum].aoch[aonum].amplitude*gi + \
                    dconf[devnum].aoch[aonum].offset);
                // Update the complex arithmetic sine generator
                // g *= e (only complex arithmetic)
                // where e = exp(j*T*2pi*f) and g is initially 1+0i
                // will cause g to orbit the unit circle
                ftemp = er*gr - gi*ei;
                gi = gi*er + gr*ei;
                gr = ftemp;
            }
        // AO_SQUARE
        }else if(dconf[devnum].aoch[aonum].signal==AO_SQUARE){
            itemp = samples*dconf[devnum].aoch[aonum].duty;
            if(itemp>samples) itemp = samples;
            // Calculate the high level
            ftemp = dconf[devnum].aoch[aonum].offset + \
                        dconf[devnum].aoch[aonum].amplitude;
            for(index=0; index<itemp; index++)
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, ftemp);
            // Calculate the low level
            ftemp = dconf[devnum].aoch[aonum].offset - \
                        dconf[devnum].aoch[aonum].amplitude;
            for(;index<samples;index++)
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, ftemp);
        // AO_TRIANGLE
        }else if(dconf[devnum].aoch[aonum].signal==AO_TRIANGLE){
            itemp = samples*dconf[devnum].aoch[aonum].duty;
            if(itemp>=samples) itemp = samples-1;
            else if(itemp<=0) itemp = 1;
            // Calculate the slope (use the generator constants)
            er = 2.*dconf[devnum].aoch[aonum].amplitude/itemp;
            // Initialize the generator
            gr = dconf[devnum].aoch[aonum].offset - \
                    dconf[devnum].aoch[aonum].amplitude;
            for(index=0; index<itemp; index++){
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, gr);
                gr += er;
            }
            // Calculate the new slope
            er = 2.*dconf[devnum].aoch[aonum].amplitude/(samples-itemp);
            for(; index<samples; index++){
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, gr);
                gr -= er;
            }
        // AO_NOISE
        }else if(dconf[devnum].aoch[aonum].signal==AO_NOISE){
            for(index=0; index<samples; index++){
                // generate a random number between -1 and 1
                ftemp = (2.*((double)rand())/RAND_MAX - 1.);
                ftemp *= dconf[devnum].aoch[aonum].amplitude;
                ftemp += dconf[devnum].aoch[aonum].offset;
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, ftemp);
            }
        }

        // Check for a buffer write error
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write data to the OStream STREAM_OUT%d_BUFFER_F32\n", 
                    aonum);
            goto lconf_uploadfail;
        }

        // Set the loop size
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_LOOP_SIZE", aonum);
        err = LJM_eWriteName( handle, stemp, samples);
        //err = LJM_eWriteAddress(handle, reg_loopsize, LJM_UINT32, samples);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write loop size %d to STREAM_OUT%d_LOOP_SIZE\n", 
                    samples, aonum);
            goto lconf_uploadfail;
        }
        // Update loop settings
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_SET_LOOP", aonum);
        err = LJM_eWriteName( handle, stemp, 1);
        //err = LJM_eWriteAddress(handle, reg_setloop, LJM_UINT32, 1);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write 1 to STREAM_OUT%d_SET_LOOP\n", 
                    aonum);
            goto lconf_uploadfail;
        }
    }

    //
    // Upload the FIO parameters
    // First, deactivate all of the extended features
    // For some reason, upper DIO#_EF_ENABLE raises an error
    for(fionum=0; fionum<8; fionum++){
        sprintf(stemp, "DIO%d_EF_ENABLE", fionum);
        err = err ? err : LJM_eWriteName(handle, stemp, 0);
    }
    if(err){
        fprintf(stderr,"UPLOAD: Failed to disable DIO extended features\n");
        goto lconf_uploadfail;
    }
    // Determine the clock 0 rollover and clock divisor from the FIOFREQUENCY parameter
    if(dconf[devnum].fiofrequency>0)
        ftemp = 1e6 * LCONF_CLOCK_MHZ / dconf[devnum].fiofrequency;
    else
        ftemp = 0xFFFFFFFF; // If FIOFREQUENCY is not configured, use a default
    fio_clk_div = 1;
    // The roll value is a 32-bit integer, and the maximum divisor is 256
    while(ftemp > 0xFFFFFFFF && fio_clk_div < 256){
        ftemp /= 2;
        fio_clk_div *= 2;
    }
    // Do some sanity checking
    if(ftemp > 0xFFFFFFFF){
        fprintf(stderr,"UPLOAD: FIO frequency was too low (%f Hz). Minimum is %f uHz.\n", 
            dconf[devnum].fiofrequency, 1e12 * LCONF_CLOCK_MHZ / 0xFFFFFFFF / 256);
        goto lconf_uploadfail;
    }else if(ftemp < 0x8){
        fprintf(stderr,"UPLOAD: FIO frequency was too high (%f Hz).  Maximum is %f MHz.\n",
            dconf[devnum].fiofrequency, LCONF_CLOCK_MHZ / 0x8);
        goto lconf_uploadfail;
    }
    // Convert the rollover value to unsigned int
    fio_clk_roll = (unsigned int) ftemp;
    // Write the clock configuration
    // disable the clock
    err = LJM_eWriteName( handle, "DIO_EF_CLOCK0_ENABLE", 0);
    // Only configure the clock if FIO channels have been configured.
    if(dconf[devnum].nfioch>0){
        err = err ? err : LJM_eWriteName( handle, "DIO_EF_CLOCK0_ROLL_VALUE", fio_clk_roll);
        // set the clock divisor
        err = err ? err : LJM_eWriteName(handle, "DIO_EF_CLOCK0_DIVISOR", fio_clk_div);
        // re-enable the clock
        err = err ? err : LJM_eWriteName(handle, "DIO_EF_CLOCK0_ENABLE", 1);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to configure FIO Clock 0\n");
            goto lconf_uploadfail;
        }
        // If the clock setup was successful, then back-calculate the actual frequency
        // and store it in the fiofrequency parameter.
        dconf[devnum].fiofrequency = 1e6 * LCONF_CLOCK_MHZ / fio_clk_roll / fio_clk_div;
    }
    
    // Configure the FIO channels
    for(fionum=0; fionum<dconf[devnum].nfioch; fionum++){
        channel = dconf[devnum].fioch[fionum].channel;
        switch(dconf[devnum].fioch[fionum].signal){
            // Pulse-width modulation
            case FIO_PWM:
                // PWM input
                if(dconf[devnum].fioch[fionum].direction==FIO_INPUT){
                    // Permitted channels are 0 and 1
                    if(channel > 1){
                        fprintf(stderr,"UPLOAD: Pulse width input only permitted on channels 0 and 1. Found ch %d.\n",
                            dconf[devnum].fioch[fionum].channel);
                        goto lconf_uploadfail;
                    }
                    // Measurement index 5
                    sprintf(stemp, "DIO%d_EF_INDEX", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 5);
                    // Options - use clock 0
                    sprintf(stemp, "DIO%d_EF_OPTIONS", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 0x00000000);
                    // Use continuous acquisition
                    sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 1);
                    // Enable the EF channel
                    sprintf(stemp, "DIO%d_EF_ENABLE", channel);
                    err = err ? err : LJM_eWriteName(handle, stemp, 1);
                // PWM output
                }else{
                    if(channel > 5 || channel == 1){
                        fprintf(stderr,"UPLOAD: Pulse width output only permitted on channels 0, 2, 3, 4, and 5. Found ch %d.\n",
                            dconf[devnum].fioch[fionum].channel);
                        goto lconf_uploadfail;
                    }
                    // Calculate the start index
                    // Unwrap the phase first (lazy method)
                    while(dconf[devnum].fioch[fionum].phase<0.)
                        dconf[devnum].fioch[fionum].phase += 360.;
                    itemp1 = fio_clk_roll * (dconf[devnum].fioch[fionum].phase / 360.);
                    itemp1 %= fio_clk_roll;
                    // Calculate the stop index
                    itemp2 = fio_clk_roll * dconf[devnum].fioch[fionum].duty;
                    itemp2 = (((long int) itemp1) + ((long int) itemp2))%fio_clk_roll;
                    // Measurement index 1
                    sprintf(stemp, "DIO%d_EF_INDEX", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 1);
                    // Options - use clock 0
                    sprintf(stemp, "DIO%d_EF_OPTIONS", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 0x00000000);
                    if(dconf[devnum].fioch[fionum].edge==FIO_FALLING){
                        // Write the start index
                        sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                        err = err ? err : LJM_eWriteName( handle, stemp, itemp2);
                        // Write the stop index
                        sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                        err = err ? err : LJM_eWriteName( handle, stemp, itemp1);
                    }else{
                        // Write the start index
                        sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                        err = err ? err : LJM_eWriteName( handle, stemp, itemp1);
                        // Write the stop index
                        sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                        err = err ? err : LJM_eWriteName( handle, stemp, itemp2);
                    }
                    // Enable the EF channel
                    sprintf(stemp, "DIO%d_EF_ENABLE", channel);
                    err = err ? err : LJM_eWriteName(handle, stemp, 1);
                }
            break;
            case FIO_COUNT:
                // counter input
                if(dconf[devnum].fioch[fionum].direction == FIO_INPUT){
                    // check for valid channels
                    if(channel > 7 || channel == 4 || channel ==5){
                        fprintf(stderr, "UPLOAD: FIO Counter input on channel %d is not supported.\n Use FIO channel 0,1,2,3,6,or 7.",
                            channel);
                        goto lconf_uploadfail;
                    }
                    switch(dconf[devnum].fioch[fionum].debounce){
                        case FIO_DEBOUNCE_NONE:
                            // Use a counter with no debounce algorithm
                            sprintf(stemp, "DIO%d_EF_INDEX", channel);
                            err = err ? err : LJM_eWriteName(handle, stemp, 8);                            
                        break;
                        case FIO_DEBOUNCE_FIXED:
                            sprintf(stemp, "DIO%d_EF_INDEX", channel);
                            err = err ? err : LJM_eWriteName(handle, stemp, 9);
                            sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                            if(dconf[devnum].fioch[fionum].edge == FIO_FALLING)
                                err = err ? err : LJM_eWriteName(handle, stemp, 3);
                            else if(dconf[devnum].fioch[fionum].edge == FIO_RISING)
                                err = err ? err : LJM_eWriteName(handle, stemp, 4);
                            else{
                                fprintf(stderr,"UPLOAD: Fixed interval debounce is not supported on  'all' edges\n");
                                goto lconf_uploadfail;
                            }
                            // Write the timeout interval
                            sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                            err = err ? err : LJM_eWriteName(handle, stemp, 
                                dconf[devnum].fioch[fionum].time);
                        break;
                        case FIO_DEBOUNCE_RESTART:
                            sprintf(stemp, "DIO%d_EF_INDEX", channel);
                            err = err ? err : LJM_eWriteName(handle, stemp, 9);
                            sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                            if(dconf[devnum].fioch[fionum].edge == FIO_FALLING)
                                err = err ? err : LJM_eWriteName(handle, stemp, 0);
                            else if(dconf[devnum].fioch[fionum].edge == FIO_RISING)
                                err = err ? err : LJM_eWriteName(handle, stemp, 1);
                            else
                                err = err ? err : LJM_eWriteName(handle, stemp, 2);
                            // Write the timeout interval
                            sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                            err = err ? err : LJM_eWriteName(handle, stemp, 
                                dconf[devnum].fioch[fionum].time);
                        break;
                        case FIO_DEBOUNCE_MINIMUM:
                            sprintf(stemp, "DIO%d_EF_INDEX", channel);
                            err = err ? err : LJM_eWriteName(handle, stemp, 9);
                            sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                            if(dconf[devnum].fioch[fionum].edge == FIO_FALLING)
                                err = err ? err : LJM_eWriteName(handle, stemp, 5);
                            else if(dconf[devnum].fioch[fionum].edge == FIO_RISING)
                                err = err ? err : LJM_eWriteName(handle, stemp, 6);
                            else{
                                fprintf(stderr,"UPLOAD: Minimum duration debounce is not supported on  'all' edges\n");
                                goto lconf_uploadfail;
                            }
                            // Write the timeout interval
                            sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                            err = err ? err : LJM_eWriteName(handle, stemp, 
                                dconf[devnum].fioch[fionum].time);
                        break;
                    }
                    // Enable the EF channel
                    sprintf(stemp, "DIO%d_EF_ENABLE", channel);
                    err = err ? err : LJM_eWriteName(handle, stemp, 1);
                }else{
                    fprintf(stderr, "UPLOAD: Count output is not currently supported.\n");
                    goto lconf_uploadfail;
                }
            break;
            case FIO_FREQUENCY:
                // Check for valid channels
                if(channel > 1){
                    fprintf(stderr, "UPLOAD: FIO Frequency on channel %d is not supported. 0 and 1 are allowed.\n",
                        channel);
                    goto lconf_uploadfail;
                }else if(dconf[devnum].fioch[fionum].direction == FIO_OUTPUT){
                    fprintf(stderr, "UPLOAD: FIO Frequency output is not supported. Use PWM.\n");
                    goto lconf_uploadfail;
                }
                if(dconf[devnum].fioch[fionum].edge == FIO_FALLING){
                    sprintf(stemp, "DIO%d_EF_INDEX", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 4);
                }else{
                    sprintf(stemp, "DIO%d_EF_INDEX", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 3);
                }
                // Operate continuously
                sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                err = err ? err : LJM_eWriteName( handle, stemp, 1);
                // Enable the EF channel
                sprintf(stemp, "DIO%d_EF_ENABLE", channel);
                err = err ? err : LJM_eWriteName(handle, stemp, 1);
            break;
            case FIO_PHASE:
                channel = (channel / 2) * 2;
                if(channel != 0){
                    fprintf(stderr, "UPLOAD: Digital phase input is only supported on channels 0 and 1.\n");
                    goto lconf_uploadfail;
                }
                if(dconf[devnum].fioch[fionum].direction == FIO_OUTPUT){
                    fprintf(stderr, "UPLOAD: Phase output is not supported\n");
                    goto lconf_uploadfail;
                }
                // Measurement index 6
                sprintf(stemp, "DIO%d_EF_INDEX", channel);
                err = err ? err : LJM_eWriteName( handle, stemp, 6);
                sprintf(stemp, "DIO%d_EF_INDEX", channel+1);
                err = err ? err : LJM_eWriteName( handle, stemp, 6);
                if(dconf[devnum].fioch[fionum].edge == FIO_RISING){
                    sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 1);
                    sprintf(stemp, "DIO%d_EF_CONFIG_A", channel+1);
                    err = err ? err : LJM_eWriteName( handle, stemp, 1);
                }else if(dconf[devnum].fioch[fionum].edge == FIO_FALLING){
                    sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 0);
                    sprintf(stemp, "DIO%d_EF_CONFIG_A", channel+1);
                    err = err ? err : LJM_eWriteName( handle, stemp, 0);
                }else{
                    fprintf(stderr, "UPLOAD: Phase measurements on ALL edges are not supported.\n");
                    goto lconf_uploadfail;
                }
                // Enable the EF channels
                sprintf(stemp, "DIO%d_EF_ENABLE", channel);
                err = err ? err : LJM_eWriteName(handle, stemp, 1);
                sprintf(stemp, "DIO%d_EF_ENABLE", channel+1);
                err = err ? err : LJM_eWriteName(handle, stemp, 1);
            break;
            case FIO_QUADRATURE:
                channel = (channel/2) * 2;  // force channel to be even
                // Check for valid channels
                if(channel > 6 || channel == 4){
                    fprintf(stderr, "UPLOAD: FIO Quadrature channels %d/%d not supported. 0/1, 2/3, 6/7 are allowed.\n",
                        channel, channel+1);
                    goto lconf_uploadfail;
                }else if(dconf[devnum].fioch[fionum].direction == FIO_OUTPUT){
                    fprintf(stderr, "UPLOAD: FIO Quadrature output is not allowed.\n");
                    goto lconf_uploadfail;
                }
                // Measurement index 10
                sprintf(stemp, "DIO%d_EF_INDEX", channel);
                err = err ? err : LJM_eWriteName( handle, stemp, 10);
                sprintf(stemp, "DIO%d_EF_INDEX", channel+1);
                err = err ? err : LJM_eWriteName( handle, stemp, 10);
                // Turn off Z-phase input
                sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                err = err ? err : LJM_eWriteName( handle, stemp, 0);
                sprintf(stemp, "DIO%d_EF_CONFIG_A", channel+1);
                err = err ? err : LJM_eWriteName( handle, stemp, 0);
                // Enable the EF channels
                sprintf(stemp, "DIO%d_EF_ENABLE", channel);
                err = err ? err : LJM_eWriteName(handle, stemp, 1);
                sprintf(stemp, "DIO%d_EF_ENABLE", channel+1);
                err = err ? err : LJM_eWriteName(handle, stemp, 1);
            break;
        }
        if(err){
            fprintf(stderr, "UPLOAD: Failed to configure FIO channel %d\n.", channel);
            goto lconf_uploadfail;
        }
    }



    return LCONF_NOERR;

    lconf_uploadfail:
    fprintf(stderr,"LCONFIG: Error uploading to device %d\n", devnum);
    LJM_ErrorToString(err, err_str);
    fprintf(stderr,"%s\n",err_str);
    return LCONF_ERROR;
}




int download_config(DEVCONF *dconf, const unsigned int devnum, DEVCONF *out){
    int err, aonum, ainum, fionum, reg_temp, type_temp;
    double ftemp, ftemp1, ftemp2;
    int itemp;
    int handle, naich, naoch, nfioch, fioindex;
    int DeviceType,ConnectionType,SerialNumber,IPAddress,Port,MaxBytesPerMB;
    char stemp[LCONF_MAX_STR];
    unsigned int channel, fio_clk_roll, fio_clk_div;

    // First, initialize the output
    init_config(out);

    // Grab global information from the original configuration that cannot be
    // obtained from the device itself.
    handle = dconf[devnum].handle;
    out->handle = handle;
    naich = dconf[devnum].naich;
    out->naich = naich;
    naoch = dconf[devnum].naoch;
    out->naoch = naoch;
    nfioch = dconf[devnum].nfioch;
    out->nfioch = nfioch;

    // Test the connection and retrieve addressing parameters
    err = LJM_GetHandleInfo(handle, 
                            &DeviceType,
                            &ConnectionType,
                            &SerialNumber,
                            &IPAddress,
                            &Port,
                            &MaxBytesPerMB);
    if(err){
        fprintf(stderr,"DOWNLOAD: No answer from device handle %d.\n", handle);
        goto lconf_downloadfail;
    }
    out->connection = ConnectionType;
    sprintf(out->serial, "%d", SerialNumber);

    // Get the remaining ethernet parameters
    err = LJM_eReadName(handle, "ETHERNET_IP", &ftemp);
    if(err){
        fprintf(stderr,"DOWNLOAD: Failed to read the IP address for handle %d.\n", handle);
        goto lconf_downloadfail;
    }
    LJM_NumberToIP((unsigned int) ftemp, out->ip);
    err = LJM_eReadName(handle, "ETHERNET_GATEWAY", &ftemp);
    if(err){
        fprintf(stderr,"DOWNLOAD: Failed to read the ethernet gateway for handle %d.\n", handle);
        goto lconf_downloadfail;
    }
    LJM_NumberToIP((unsigned int) ftemp, out->gateway);
    err = LJM_eReadName(handle, "ETHERNET_SUBNET", &ftemp);
    if(err){
        fprintf(stderr,"DOWNLOAD: Failed to read the ethernet subnet for handle %d.\n", handle);
        goto lconf_downloadfail;
    }
    LJM_NumberToIP((unsigned int) ftemp, out->subnet);

    // Loop through the analog input channels.
    for(ainum=0;ainum<naich;ainum++){
        // Read the channel directly from the original configuration
        channel = dconf[devnum].aich[ainum].channel;
        out->aich[ainum].channel = channel;

        /* Depreciated - uses register addresses instead of names
        // Get the addresses for that channel's configuration data
        airegisters(out->aich[ainum].channel,
                    &reg_negative, &reg_range, &reg_resolution);
        */
        // Read the negative channel
        stemp[0] = '\0';
        sprintf(stemp, "AI%d_NEGATIVE_CH", channel);
        err = LJM_eReadName( handle, stemp, &ftemp);
        /* Depreciated - used the register address instead of name
        err = LJM_eReadAddress(  handle,
                            reg_negative,
                            LJM_UINT16,
                            &ftemp);
        */
        if(err){
            fprintf(stderr,"DOWNLOAD: Failed to read the negative AI channel number at address\n");
            goto lconf_downloadfail;
        }
        out->aich[ainum].nchannel = (int) ftemp;

        // Read the range
        stemp[0] = '\0';
        sprintf(stemp, "AI%d_RANGE", channel);
        err = LJM_eReadName( handle, stemp, &ftemp);
        /* Depreciated -used the register address instead of the name
        err = LJM_eReadAddress( handle,
                            reg_range,
                            LJM_FLOAT32,
                            &ftemp);
        */
        if(err){
            fprintf(stderr,"DOWNLOAD: Failed to read the AI channel range at address\n");
            goto lconf_downloadfail;
        }
        out->aich[ainum].range = ftemp;

        // Read the resolution
        stemp[0] = '\0';
        sprintf(stemp, "AI%d_RESOLUTION", channel);
        err = LJM_eReadName( handle, stemp, &ftemp);
        /* Depreciated - used the register address instead of the name
        err = LJM_eReadAddress( handle,
                            reg_resolution,
                            LJM_UINT16,
                            &ftemp);
        */
        if(err){
            fprintf(stderr,"DOWNLOAD: Failed to read the AI channel resolution index at address\n");
            goto lconf_downloadfail;
        }
        out->aich[ainum].resolution = (int) ftemp;
    }

    // Loop through the analog output channels
    for(aonum=0; aonum<naoch; aonum++){
        // Determine the channel from the STREAM_OUT#_TARGET
        sprintf(stemp, "STREAM_OUT%d_TARGET", aonum);
        err = err ? err : LJM_eReadName(handle, stemp, &ftemp);
        LJM_NameToAddress("DAC0",&reg_temp,&type_temp);
        // calculate a channel number based on the target's distance from DAC0
        out->aoch[aonum].channel = ((int)ftemp - reg_temp)/2;
        // calculate a signal frequency based on the loop size
        sprintf(stemp, "STREAM_OUT%d_LOOP_SIZE", aonum);
        err = err ? err : LJM_eReadName(handle, stemp, &ftemp);
        // borrow the samplehz parameter to estimate signal frequency
        if(ftemp>0)
            out->aoch[aonum].frequency = dconf[devnum].samplehz / ftemp;
        else
            out->aoch[aonum].frequency = -1;
    }
    if(err){
        fprintf(stderr,"DOWNLOAD: Failed to read the analog output parameters.\n");
        goto lconf_downloadfail;
    }

    // Retrieve the FIO clock settings if there are FIO channels
    if(nfioch>0){
        err = err ? err : LJM_eReadName(handle, "DIO_EF_CLOCK0_ROLL_VALUE", &ftemp);
        fio_clk_roll = ftemp;
        err = err ? err : LJM_eReadName(handle, "DIO_EF_CLOCK0_DIVISOR", &ftemp);
        fio_clk_div = ftemp;
        out->fiofrequency = ((double) 1e6 * LCONF_CLOCK_MHZ) / fio_clk_roll / fio_clk_div;
    }
    if(err){
        fprintf(stderr,"DOWNLOAD: Failed to read the FIO clock settings.\n");
        goto lconf_downloadfail;
    }

    // Check the FIO extended features
    for(fionum=0; fionum<nfioch; fionum++){
        // read the channel from the existing configuration
        channel = dconf[devnum].fioch[fionum].channel;
        out->fioch[fionum].channel = channel;

        // Retrieve the extended feature index for the channel
        sprintf(stemp, "DIO%d_EF_INDEX", channel);
        err = err ? err : LJM_eReadName(handle, stemp, &ftemp);
        fioindex = (int) ftemp;
        // Check the index
        if(fioindex == 1){
            // PWM Output
            out->fioch[fionum].signal = FIO_PWM;
            out->fioch[fionum].direction = FIO_OUTPUT;
            // The choice of edge is arbitrary, so take it from the original
            out->fioch[fionum].edge = dconf[devnum].fioch[fionum].edge;
            if(dconf[devnum].fioch[fionum].edge==FIO_FALLING){
                // Read the start index
                sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp2);
                // Read the stop index
                sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp1);
            }else{
                // Read the start index
                sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp1);
                // Read the stop index
                sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp2);
            }
            if(err){
                fprintf(stderr,"DOWNLOAD: Failed to download FIO ch%d PWM parameters\n",
                    channel);
                goto lconf_downloadfail;
            }
            // Calculate phase
            out->fioch[fionum].phase = ftemp1 * 360. / fio_clk_roll;
            // Calculate duty
            ftemp2 -= ftemp1;
            // If the waveform has wrapped, unwrap it
            if(ftemp2 < 0)
                ftemp2 += fio_clk_roll;
            out->fioch[fionum].duty = ftemp2 / fio_clk_roll;
        }else if(fioindex == 3){
            // Rising edge frequency measurement
            out->fioch[fionum].signal = FIO_FREQUENCY;
            out->fioch[fionum].direction = FIO_INPUT;
            out->fioch[fionum].edge = FIO_RISING;
        }else if(fioindex == 4){
            // Falling edge frequency measurement
            out->fioch[fionum].signal = FIO_FREQUENCY;
            out->fioch[fionum].direction = FIO_INPUT;
            out->fioch[fionum].edge = FIO_FALLING;
        }else if(fioindex == 5){
            // Pulse width measurement
            out->fioch[fionum].signal = FIO_PWM;
            out->fioch[fionum].direction = FIO_INPUT;
            out->fioch[fionum].edge = dconf[devnum].fioch[fionum].edge;
        }else if(fioindex == 6){
            // Verify that the sister channel has also been configured
            sprintf(stemp, "DIO%d_EF_INDEX", channel+1);
            err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
            if(ftemp != 6){
                fprintf(stderr, "DOWNLOAD: Channel %d is set to phase, but %d is not.\n",
                    channel, channel+1);
            }
            
            // Phase measurement
            out->fioch[fionum].signal = FIO_PHASE;
            out->fioch[fionum].direction = FIO_INPUT;
            // Get the edge indicator
            sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
            err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
            if(err){
                fprintf(stderr, "DOWNLOAD: Failed to read digital phase input parameters.\n");
                goto lconf_downloadfail;
            }
            if(ftemp==0)
                out->fioch[fionum].edge = FIO_FALLING;
            else if(ftemp==1)
                out->fioch[fionum].edge = FIO_RISING;
            else{
                fprintf(stderr,"DOWNLOAD: FIO channel %d phase measurement edge invalid (%d)\n.",
                    channel, (int) ftemp);
                out->fioch[fionum].edge = FIO_RISING;
            }
        }else if(fioindex == 8){
            // Counter
            out->fioch[fionum].signal = FIO_COUNT;
            out->fioch[fionum].direction = FIO_INPUT;
            out->fioch[fionum].debounce = FIO_DEBOUNCE_NONE;
        }else if(fioindex == 9){
            // Counter with debounce
            out->fioch[fionum].signal = FIO_COUNT;
            out->fioch[fionum].direction = FIO_INPUT;
            // Get the timeout interval
            sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
            err = err ? err : LJM_eReadName( handle, stemp, &ftemp1);
            // Get the debounce mode
            sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
            err = err ? err : LJM_eReadName( handle, stemp, &ftemp2);
            if(err){
                fprintf(stderr, "DOWNLOAD: Failed to download counter debounce settings.\n");
                goto lconf_downloadfail;
            }
            out->fioch[fionum].time = ftemp1;
            // Interpret the debounce mode
            if(ftemp2==0){
                out->fioch[fionum].edge=FIO_FALLING;
                out->fioch[fionum].debounce=FIO_DEBOUNCE_RESTART;
            }else if(ftemp2==1){
                out->fioch[fionum].edge=FIO_RISING;
                out->fioch[fionum].debounce=FIO_DEBOUNCE_RESTART;
            }else if(ftemp2==2){
                out->fioch[fionum].edge=FIO_ALL;
                out->fioch[fionum].debounce=FIO_DEBOUNCE_RESTART;
            }else if(ftemp2==3){
                out->fioch[fionum].edge=FIO_FALLING;
                out->fioch[fionum].debounce=FIO_DEBOUNCE_FIXED;
            }else if(ftemp2==4){
                out->fioch[fionum].edge=FIO_RISING;
                out->fioch[fionum].debounce=FIO_DEBOUNCE_FIXED;
            }else if(ftemp2==5){
                out->fioch[fionum].edge=FIO_FALLING;
                out->fioch[fionum].debounce=FIO_DEBOUNCE_MINIMUM;
            }else if(ftemp2==6){
                out->fioch[fionum].edge=FIO_RISING;
                out->fioch[fionum].debounce=FIO_DEBOUNCE_MINIMUM;
            }
        }else if(fioindex == 10){
            out->fioch[fionum].signal = FIO_QUADRATURE;
        }
    }

    return LCONF_NOERR;

    lconf_downloadfail:
    fprintf(stderr,"LCONFIG: Error downloading from device %d\n", devnum);
    LJM_ErrorToString(err, err_str);
    fprintf(stderr,"%s\n",err_str);
    return LCONF_ERROR;
}





void show_config(DEVCONF* dconf, const unsigned int devnum){
    int ainum,aonum,fionum,metanum,metacnt;
    char value1[LCONF_MAX_STR], value2[LCONF_MAX_STR];
    DEVCONF live;

    // download the live parameters for compairson
    download_config(dconf, devnum, &live);

    // Start with the global parameters
    printf("Device [%d]:\n", devnum);
    sprintf(value1, "%d", dconf[devnum].connection);
    sprintf(value2, "%d", live.connection);
    print_color("Connection 1=USB,3=ETH", value1, value2);
    print_color("Serial Number", dconf[devnum].serial, live.serial);
    print_color("IP Address", dconf[devnum].ip, live.ip);
    print_color("Gateway", dconf[devnum].gateway, live.gateway);
    print_color("Subnet", dconf[devnum].subnet, live.subnet);
    sprintf(value1, "%.1f", dconf[devnum].samplehz);
    print_color("Sample Rate(Hz):",value1,"?");
    sprintf(value1, "%.1f", dconf[devnum].settleus);
    print_color("Settling Time(us):",value1,"?");
    sprintf(value1, "%d", dconf[devnum].nsample);
    print_color("Samples per Read:",value1,"?");
    sprintf(value1, "%d", dconf[devnum].naich);
    sprintf(value2, "%d", live.naich);
    print_color("\nAnalog Input Channels", value1, value2);
    for(ainum=0;ainum<dconf[devnum].naich;ainum++){
        printf("Analog Input [%d]:\n",ainum);
        sprintf(value1, "%d", dconf[devnum].aich[ainum].channel);
        sprintf(value2, "%d", live.aich[ainum].channel);
        print_color("Positive Channel",value1,value2);
        sprintf(value1, "%d", dconf[devnum].aich[ainum].nchannel);
        sprintf(value2, "%d", live.aich[ainum].nchannel);
        print_color("Negative Channel",value1,value2);
        sprintf(value1, "%.3f", dconf[devnum].aich[ainum].range);
        sprintf(value2, "%.3f", live.aich[ainum].range);
        print_color("Range (V)",value1,value2);
        sprintf(value1, "%d", dconf[devnum].aich[ainum].resolution);
        sprintf(value2, "%d", live.aich[ainum].resolution);
        print_color("Resolution Index",value1,value2);
    }
    sprintf(value1, "%d", dconf[devnum].naoch);
    sprintf(value2, "%d", live.naoch);
    print_color("\nAnalog Output Channels", value1, value2);
    for(aonum=0;aonum<dconf[devnum].naoch;aonum++){
        printf("Analog Output [%d]:\n",aonum);
        if(dconf[devnum].aoch[aonum].signal==AO_CONSTANT)  print_color("Signal", "constant", "?");
        else if(dconf[devnum].aoch[aonum].signal==AO_SINE)  print_color("Signal", "sine", "?");
        else if(dconf[devnum].aoch[aonum].signal==AO_SQUARE)  print_color("Signal", "square", "?");
        else if(dconf[devnum].aoch[aonum].signal==AO_TRIANGLE)  print_color("Signal", "triangle", "?");
        else if(dconf[devnum].aoch[aonum].signal==AO_NOISE)  print_color("Signal", "noise", "?");
        else print_color("Signal","***","?");
        sprintf(value1, "%d", dconf[devnum].aoch[aonum].channel);
        sprintf(value2, "%d", live.aoch[aonum].channel);
        print_color("Channel",value1,value2);
        sprintf(value1, "%.1f", dconf[devnum].aoch[aonum].frequency);
        sprintf(value2, "%.1f", live.aoch[aonum].frequency);
        print_color("Frequency (Hz)",value1,value2);
        sprintf(value1, "%.3f", dconf[devnum].aoch[aonum].amplitude);
        print_color("Amplitude (V)",value1,"?");
        sprintf(value1, "%.3f", dconf[devnum].aoch[aonum].offset);
        print_color("Offset (V)",value1,"?");
        sprintf(value1, "%.3f", dconf[devnum].aoch[aonum].duty);
        print_color("Duty Cycle",value1,"?");
    }
    // Show FIO parameters
    sprintf(value1, "%d", dconf[devnum].nfioch);
    sprintf(value2, "%d", live.nfioch);
    print_color("\nFIO Channels", value1, value2);
    if(dconf[devnum].nfioch>0){
        sprintf(value1, "%.1f", dconf[devnum].fiofrequency);
        sprintf(value2, "%.1f", live.fiofrequency);
        print_color("FIO Frequency (Hz)", value1, value2);
    }
    for(fionum=0; fionum<dconf[devnum].nfioch; fionum++){
        printf("Flexible Input/Output [%d]:\n", fionum);
        sprintf(value1, "%d", dconf[devnum].fioch[fionum].channel);
        sprintf(value2, "%d", live.fioch[fionum].channel);
        print_color("Channel",value1,value2);
        if(dconf[devnum].fioch[fionum].signal==FIO_PWM)
            sprintf(value1, "PWM");
        else if(dconf[devnum].fioch[fionum].signal==FIO_COUNT)
            sprintf(value1, "COUNT");
        else if(dconf[devnum].fioch[fionum].signal==FIO_FREQUENCY)
            sprintf(value1, "FREQUENCY");
        else if(dconf[devnum].fioch[fionum].signal==FIO_PHASE)
            sprintf(value1, "PHASE");
        else if(dconf[devnum].fioch[fionum].signal==FIO_QUADRATURE)
            sprintf(value1, "QUADRATURE");
        else
            sprintf(value1, "NONE");

        if(live.fioch[fionum].signal==FIO_PWM)
            sprintf(value2, "PWM");
        else if(live.fioch[fionum].signal==FIO_COUNT)
            sprintf(value2, "COUNT");
        else if(live.fioch[fionum].signal==FIO_FREQUENCY)
            sprintf(value2, "FREQUENCY");
        else if(live.fioch[fionum].signal==FIO_PHASE)
            sprintf(value2, "PHASE");
        else if(live.fioch[fionum].signal==FIO_QUADRATURE)
            sprintf(value2, "QUADRATURE");
        else
            sprintf(value2, "NONE");
        print_color("Signal", value1, value2);

        if(dconf[devnum].fioch[fionum].direction==FIO_INPUT)
            sprintf(value1, "INPUT");
        else if(dconf[devnum].fioch[fionum].direction==FIO_OUTPUT)
            sprintf(value1, "OUTPUT");
        else
            sprintf(value1, "?");
        if(live.fioch[fionum].direction==FIO_INPUT)
            sprintf(value2, "INPUT");
        else if(live.fioch[fionum].direction==FIO_OUTPUT)
            sprintf(value2, "OUTPUT");
        else
            sprintf(value2, "?");
        print_color("Direction",value1,value2);

        if(dconf[devnum].fioch[fionum].edge==FIO_RISING)
            sprintf(value1, "RISING");
        else if(dconf[devnum].fioch[fionum].edge==FIO_FALLING)
            sprintf(value1, "FALLING");
        else if(dconf[devnum].fioch[fionum].edge==FIO_ALL)
            sprintf(value1, "ALL");
        else
            sprintf(value1, "?");
        if(live.fioch[fionum].edge==FIO_RISING)
            sprintf(value2, "RISING");
        else if(live.fioch[fionum].edge==FIO_FALLING)
            sprintf(value2, "FALLING");
        else if(live.fioch[fionum].edge==FIO_ALL)
            sprintf(value2, "ALL");
        else
            sprintf(value2, "?");
        print_color("Edge",value1,value2);

        sprintf(value1, "%.1f", dconf[devnum].fioch[fionum].time);
        sprintf(value2, "%.1f", live.fioch[fionum].time);
        print_color("Time (us)",value1,value2);
        sprintf(value1, "%.3f", dconf[devnum].fioch[fionum].duty);
        sprintf(value2, "%.3f", live.fioch[fionum].duty);
        print_color("Duty cycle",value1,value2);
        sprintf(value1, "%.1f", dconf[devnum].fioch[fionum].phase);
        sprintf(value2, "%.1f", live.fioch[fionum].phase);
        print_color("Phase (deg)",value1,value2);
        sprintf(value1, "%d", dconf[devnum].fioch[fionum].counts);
        sprintf(value2, "%d", live.fioch[fionum].counts);
        print_color("Counts",value1,value2);
    }

    // count the meta parameters
    for(metacnt=0;\
            metacnt<LCONF_MAX_META && \
            dconf[devnum].meta[metacnt].param[0]!='\0';\
            metacnt++){}
    printf("\nMeta Parameters: %d\n",metacnt);
    for(metanum=0; metanum<metacnt; metanum++)
        switch(dconf[devnum].meta[metanum].type){
            case 'i':
                printf("%19s (int): %d\n",
                    dconf[devnum].meta[metanum].param,
                    dconf[devnum].meta[metanum].value.ivalue);
            break;
            case 'f':
                printf("%19s (flt): %f\n",
                    dconf[devnum].meta[metanum].param,
                    dconf[devnum].meta[metanum].value.fvalue);
            break;
            case 's':
                printf("%19s (str): %s\n",
                    dconf[devnum].meta[metanum].param,
                    dconf[devnum].meta[metanum].value.svalue);
            break;
            default:
                printf(LCONF_COLOR_RED "%19s ( ? ) **CORRUPT**\n" LCONF_COLOR_NULL,
                    dconf[devnum].meta[metanum].param);
        }
}



int update_fio(DEVCONF* dconf, const unsigned int devnum){
    int handle, channel, fionum, itemp, itemp1, itemp2;
    double ftemp, ftemp1, ftemp2;
    char stemp[LCONF_MAX_STR];
    unsigned int fio_clk_roll, fio_clk_div;
    int err = 0;

    handle = dconf[devnum].handle;

    // Get the roll value
    err = err ? err : LJM_eReadName(handle, "DIO_EF_CLOCK0_ROLL_VALUE", &ftemp);
    fio_clk_roll = (unsigned int) ftemp;
    // Get the divisor
    err = err ? err : LJM_eReadName(handle, "DIO_EF_CLOCK0_DIVISOR", &ftemp);
    fio_clk_div = (unsigned int) ftemp;
    
    // Configure the FIO channels
    for(fionum=0; fionum<dconf[devnum].nfioch; fionum++){
        channel = dconf[devnum].fioch[fionum].channel;
        switch(dconf[devnum].fioch[fionum].signal){
            // Pulse-width modulation
            case FIO_PWM:
                // PWM input
                if(dconf[devnum].fioch[fionum].direction==FIO_INPUT){
                    // Get the time high
                    sprintf(stemp, "DIO%d_EF_READ_A", channel);
                    err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
                    // Get the time low
                    sprintf(stemp, "DIO%d_EF_READ_B", channel);
                    err = err ? err : LJM_eReadName( handle, stemp, &ftemp1);
                    ftemp2 = ftemp + ftemp1;

                    dconf[devnum].fioch[fionum].counts = (unsigned int) ftemp2;
                    dconf[devnum].fioch[fionum].time = ftemp2 * fio_clk_div / LCONF_CLOCK_MHZ;
                    dconf[devnum].fioch[fionum].phase = 0.;
                    if(dconf[devnum].fioch[fionum].edge==FIO_FALLING){
                        dconf[devnum].fioch[fionum].duty = ftemp1 / ftemp2;
                    }else{
                        dconf[devnum].fioch[fionum].duty = ftemp / ftemp2;
                    }

                // PWM output
                }else{
                    // Calculate the start index
                    // Unwrap the phase first (lazy method)
                    while(dconf[devnum].fioch[fionum].phase<0.)
                        dconf[devnum].fioch[fionum].phase += 360.;
                    itemp1 = fio_clk_roll * (dconf[devnum].fioch[fionum].phase / 360.);
                    itemp1 %= fio_clk_roll;
                    // Calculate the stop index
                    itemp2 = fio_clk_roll * dconf[devnum].fioch[fionum].duty;
                    itemp2 = (((long int) itemp1) + ((long int) itemp2))%fio_clk_roll;
                    if(dconf[devnum].fioch[fionum].edge==FIO_FALLING){
                        // Write the start index
                        sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                        err = err ? err : LJM_eWriteName( handle, stemp, itemp2);
                        // Write the stop index
                        sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                        err = err ? err : LJM_eWriteName( handle, stemp, itemp1);
                    }else{
                        // Write the start index
                        sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                        err = err ? err : LJM_eWriteName( handle, stemp, itemp1);
                        // Write the stop index
                        sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                        err = err ? err : LJM_eWriteName( handle, stemp, itemp2);
                    }
                    sprintf(stemp, "DIO%d_EF_ENABLE", channel);
                    err = err ? err : LJM_eWriteName( handle, stemp, 1);
                }
            break;
            case FIO_COUNT:
                sprintf(stemp, "DIO%d_EF_READ_A", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
                dconf[devnum].fioch[fionum].counts = (unsigned int) ftemp;
                dconf[devnum].fioch[fionum].time = 0.;
                dconf[devnum].fioch[fionum].duty = 0.;
                dconf[devnum].fioch[fionum].phase = 0.;
            break;
            case FIO_FREQUENCY:
                sprintf(stemp, "DIO%d_EF_READ_A", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
                dconf[devnum].fioch[fionum].counts = (unsigned int) ftemp;
                dconf[devnum].fioch[fionum].time = ftemp * fio_clk_div / LCONF_CLOCK_MHZ;
                dconf[devnum].fioch[fionum].duty = 0.;
                dconf[devnum].fioch[fionum].phase = 0.;
            break;
            case FIO_PHASE:
                sprintf(stemp, "DIO%d_EF_READ_A", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
                dconf[devnum].fioch[fionum].counts = (unsigned int) ftemp;
                dconf[devnum].fioch[fionum].time = ftemp * fio_clk_div / LCONF_CLOCK_MHZ;
                dconf[devnum].fioch[fionum].duty = 0.;
                dconf[devnum].fioch[fionum].phase = 0.;
            break;
            case FIO_QUADRATURE:
                sprintf(stemp, "DIO%d_EF_READ_A", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
                dconf[devnum].fioch[fionum].counts = (unsigned int) ftemp;
                dconf[devnum].fioch[fionum].time = 0;
                dconf[devnum].fioch[fionum].duty = 0.;
                dconf[devnum].fioch[fionum].phase = 0.;
            break;
        }
        if(err){
            fprintf(stderr, "FIO_UPDATE: Failed while working on FIO channel %d\n.", channel);
            break;
        }
    }

    if(err){
        fprintf(stderr,"LCONFIG: Error updating Flexible IO parameters on device %d\n", devnum);
        LJM_ErrorToString(err, err_str);
        fprintf(stderr,"%s\n",err_str);
        return LCONF_ERROR;
    }
    return LCONF_NOERR;
}


int get_meta_int(DEVCONF* dconf, const unsigned int devnum, const char* param, int* value){
    int metanum;
    // Scan through the meta list
    // stop if end-of-list or empty entry
    for(metanum=0;
        metanum<LCONF_MAX_META && \
        dconf[devnum].meta[metanum].param[0] != '\0';
        metanum++)
        // If the parameter matches, return the value
        if(strncmp(dconf[devnum].meta[metanum].param,param,LCONF_MAX_STR)==0){
            // check that the type is correct
            if(dconf[devnum].meta[metanum].type!='i')
                fprintf(stderr,"GET_META::WARNING:: param %s holds type '%c', but was queried for an integer.\n",\
                    param,dconf[devnum].meta[metanum].type);
            *value = dconf[devnum].meta[metanum].value.ivalue;
            return LCONF_NOERR;
        }
    fprintf(stderr,"GET_META: Failed to find meta parameter, %s\n",param);
    return LCONF_NOERR;
}

int get_meta_flt(DEVCONF* dconf, const unsigned int devnum, const char* param, double* value){
    int metanum;
    // Scan through the meta list
    // stop if end-of-list or empty entry
    for(metanum=0;
        metanum<LCONF_MAX_META && \
        dconf[devnum].meta[metanum].param[0] != '\0';
        metanum++)
        // If the parameter matches, return the value
        if(strncmp(dconf[devnum].meta[metanum].param,param,LCONF_MAX_STR)==0){
            // check that the type is correct
            if(dconf[devnum].meta[metanum].type!='f')
                fprintf(stderr,"GET_META::WARNING:: param %s holds type '%c', but was queried for an float.\n",\
                    param,dconf[devnum].meta[metanum].type);
            *value = dconf[devnum].meta[metanum].value.fvalue;
            return LCONF_NOERR;
        }
    fprintf(stderr,"GET_META: Failed to find meta parameter, %s\n",param);
    return LCONF_NOERR;
}

int get_meta_str(DEVCONF* dconf, const unsigned int devnum, const char* param, char* value){
    int metanum;
    // Scan through the meta list
    // stop if end-of-list or empty entry
    for(metanum=0;
        metanum<LCONF_MAX_META && \
        dconf[devnum].meta[metanum].param[0] != '\0';
        metanum++)
        // If the parameter matches, return the value
        if(strncmp(dconf[devnum].meta[metanum].param,param,LCONF_MAX_STR)==0){
            // check that the type is correct
            if(dconf[devnum].meta[metanum].type!='s')
                fprintf(stderr,"GET_META::WARNING:: param %s holds type '%c', but was queried for an string.\n",\
                    param,dconf[devnum].meta[metanum].type);
            strncpy(value, dconf[devnum].meta[metanum].value.svalue, LCONF_MAX_STR);
            return LCONF_NOERR;
        }
    fprintf(stderr,"GET_META: Failed to find meta parameter, %s\n",param);
    return LCONF_ERROR;
}

int put_meta_int(DEVCONF* dconf, const unsigned int devnum, const char* param, int value){
    int metanum;
    for(metanum=0; metanum<LCONF_MAX_META; metanum++){
        if(dconf[devnum].meta[metanum].param[0] == '\0'){
            strncpy(dconf[devnum].meta[metanum].param, param, LCONF_MAX_STR);
            dconf[devnum].meta[metanum].value.ivalue = value;
            dconf[devnum].meta[metanum].type = 'i';
            return LCONF_NOERR;
        }else if(strncmp(dconf[devnum].meta[metanum].param, param, LCONF_MAX_STR)==0){
            dconf[devnum].meta[metanum].value.ivalue = value;
            dconf[devnum].meta[metanum].type = 'i';
            return LCONF_NOERR;
        }
    }
    fprintf(stderr,"PUT_META: Meta array is full. Failed to place parameter, %s\n",param);
    return LCONF_ERROR;
}

int put_meta_flt(DEVCONF* dconf, const unsigned int devnum, const char* param, double value){
    int metanum;
    for(metanum=0; metanum<LCONF_MAX_META; metanum++){
        if(dconf[devnum].meta[metanum].param[0] == '\0'){
            strncpy(dconf[devnum].meta[metanum].param, param, LCONF_MAX_STR);
            dconf[devnum].meta[metanum].value.fvalue = value;
            dconf[devnum].meta[metanum].type = 'f';
            return LCONF_NOERR;
        }else if(strncmp(dconf[devnum].meta[metanum].param, param, LCONF_MAX_STR)==0){
            dconf[devnum].meta[metanum].value.fvalue = value;
            dconf[devnum].meta[metanum].type = 'f';
            return LCONF_NOERR;
        }
    }
    fprintf(stderr,"PUT_META: Meta array is full. Failed to place parameter, %s\n",param);
    return LCONF_ERROR;
}

int put_meta_str(DEVCONF* dconf, const unsigned int devnum, const char* param, char* value){
    int metanum;
    for(metanum=0; metanum<LCONF_MAX_META; metanum++){
        if(dconf[devnum].meta[metanum].param[0] == '\0'){
            strncpy(dconf[devnum].meta[metanum].param, param, LCONF_MAX_STR);
            strncpy(dconf[devnum].meta[metanum].value.svalue, value, LCONF_MAX_STR);
            dconf[devnum].meta[metanum].type = 's';
            return LCONF_NOERR;
        }else if(strncmp(dconf[devnum].meta[metanum].param, param, LCONF_MAX_STR)==0){
            strncpy(dconf[devnum].meta[metanum].value.svalue, value, LCONF_MAX_STR);
            dconf[devnum].meta[metanum].type = 's';
            return LCONF_NOERR;
        }
    }    
    fprintf(stderr,"PUT_META: Meta array is full. Failed to place parameter, %s\n",param);
    return LCONF_ERROR;
}



int start_data_stream(DEVCONF* dconf, const unsigned int devnum, int samples_per_read){
    int ainum,aonum,index,err;
    int stlist[LCONF_MAX_STCH];
    int resindex, reg_temp, dummy;
    char flag;

    if(samples_per_read <= 0)
        samples_per_read = dconf[devnum].nsample;

    if(dconf[devnum].naich<1 && dconf[devnum].naoch<1){
        fprintf(stderr,"STREAM_START: Cannot start a data stream with no channels on device %d.\n", devnum);
        return LCONF_ERROR;
    }

    // Initialize the resolution index
    resindex = dconf[devnum].aich[0].resolution;
    flag = 1;   // use the flag to remember if we have already issued a warning.
    // Assemble the list of channels and check the resolution settings
    for(ainum=0; ainum<dconf[devnum].naich; ainum++){
        // Populate the channel list
        stlist[ainum] = dconf[devnum].aich[ainum].channel * 2;
        // Do we have a conflict in resindex values?
        if(flag && resindex != dconf[devnum].aich[ainum].resolution){
            fprintf(stderr,"STREAM_START: AI channel resolution indices disagree; defaulting to the lowest.\n");
            flag=0;
        }
        // If this channel's resolution index is less than those previous
        if(resindex > dconf[devnum].aich[ainum].resolution)
            resindex = dconf[devnum].aich[ainum].resolution;
    }

    if(!flag)
        fprintf(stderr,"STREAM_START: Used resolution index %d for all streaming channels.\n",resindex);


    // Write the configuration parameters unique to streaming.
    err=LJM_eWriteName(dconf[devnum].handle, "STREAM_RESOLUTION_INDEX", resindex);

    // Add any analog output streams to the stream scanlist
    // This approach collects analog input BEFORE updating the outputs.
    // This allows one scan cycle for system settling before the new 
    // measurements are taken.  On the other hand, it also imposes a 
    // scan cycle's worth of phase between output and input.
    index = dconf[devnum].naich;
    // Get the STREAM_OUT0 address
    LJM_NameToAddress("STREAM_OUT0",&reg_temp,&dummy);
    for(aonum=0;aonum<dconf[devnum].naoch;aonum++){
        stlist[index] = reg_temp;
        // increment the index and the register
        reg_temp += 1;
        index+=1;
    }
    // Write the settling time
    err=LJM_eWriteName(dconf[devnum].handle, "STREAM_SETTLING_US", dconf[devnum].settleus);
    if(err){
        fprintf(stderr,"STREAM_START: Failed to set STREAM_SETTLING_US register.\n");
        goto lconf_startfail;
    }

    // Start the stream.
    err=LJM_eStreamStart(dconf[devnum].handle, 
            samples_per_read,
            dconf[devnum].naich+dconf[devnum].naoch,
            stlist,
            &dconf[devnum].samplehz);
    if(err){
        fprintf(stderr,"STREAM_START: Failed to start the stream.\n");
        goto lconf_startfail;
    }
    return LCONF_NOERR;

    lconf_startfail:
    fprintf(stderr,"LCONFIG: Encountered an error starting a data stream on device %d.\n",devnum);
    LJM_ErrorToString(err, err_str);
    fprintf(stderr,"%s\n",err_str);
    return LCONF_ERROR;
}


int read_data_stream(DEVCONF* dconf, const unsigned int devnum, double *data){
    int dev_backlog, ljm_backlog, err;

    err = LJM_eStreamRead(dconf[devnum].handle, data, &dev_backlog, &ljm_backlog);

    if(dev_backlog > LCONF_BACKLOG_THRESHOLD)
        fprintf(stderr,"READ_STREAM: Device backlog! %d\n",dev_backlog);
    else if(ljm_backlog > LCONF_BACKLOG_THRESHOLD)
        fprintf(stderr,"READ_STREAM: Software backlog! %d\n",ljm_backlog);
    
    if(err){
        fprintf(stderr,"LCONFIG: Error reading from data stream on device %d.\n",devnum);
        LJM_ErrorToString(err, err_str);
        fprintf(stderr,"%s\n",err_str);
        return LCONF_ERROR;
    }
    return LCONF_NOERR;
}


int stop_data_stream(DEVCONF* dconf, const unsigned int devnum){
    int err;
    err = LJM_eStreamStop(dconf[devnum].handle);

    if(err){
        fprintf(stderr,"LCONFIG: Error stopping a data stream on device %d.\n",devnum);
        LJM_ErrorToString(err, err_str);
        fprintf(stderr,"%s\n",err_str);
        return LCONF_ERROR;
    }
    return LCONF_NOERR;
}



int start_file_stream(DEVCONF* dconf, const unsigned int devnum, 
            int samples_per_read, const char* filename){

    unsigned int bytes;
    time_t now;
    int err;
    FILESTREAM *FS;
    FS = &dconf[devnum].FS;

    if(samples_per_read <= 0)
        samples_per_read = dconf[devnum].nsample;

    if(FS->file){
        fprintf(stderr,"FILE_STREAM: File is already open on device %d.\n",devnum);
        return LCONF_ERROR;
    }
    FS->file = fopen(filename,"w+");
    if(FS->file==NULL){
        fprintf(stderr,"FILE_STREAM: Device %d failed to open file stream to file %s\n",devnum,filename);
        return LCONF_ERROR;
    }
    // Initialize the buffer
    FS->samples_per_read = samples_per_read;
    bytes = dconf[devnum].naich * samples_per_read * sizeof(double);
    FS->buffer = malloc(bytes);
    if(FS->buffer==NULL){
        fprintf(stderr,"FILE_STREAM: Failed to allocate %d bytes to buffer", bytes);
        return LCONF_ERROR;
    }

    // From this point on, free needs to be called upon failure
    // use clean_file_stream() when in doubt

    // Write the configuration header
    write_config(dconf,devnum,FS->file);
    // Start data collection
    err = start_data_stream(dconf,devnum,samples_per_read);
    // Log the time
    time(&now);
    fprintf(FS->file, "#: %s", ctime(&now));

    if(err){
        // The error will already have been printed by start_data_stream()
        fprintf(stderr,"FILE_STREAM: Cleaning file stream.\n");
        clean_file_stream(FS);
        return LCONF_ERROR;
    }
    return LCONF_NOERR;
}




int read_file_stream(DEVCONF* dconf, const unsigned int devnum){
    int err,index,row,ainum;
    FILESTREAM *FS;
    FS = &dconf[devnum].FS;

    err = read_data_stream(dconf,devnum,FS->buffer);
    index = 0;
    // Write data one element at a time.
    for(row=0; row<FS->samples_per_read; row++){
        for(ainum=0; ainum<dconf[devnum].naich-1; ainum++)
            fprintf(FS->file, "%.6e\t", FS->buffer[ index++ ]);
        fprintf(FS->file, "%.6e\n", FS->buffer[ index++ ]);
    }
    return err;
}


int stop_file_stream(DEVCONF* dconf, const unsigned int devnum){
    int err;

    err = stop_data_stream(dconf,devnum);
    clean_file_stream(&dconf[devnum].FS);
    return err;
}



