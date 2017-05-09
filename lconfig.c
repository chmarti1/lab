
#include <stdio.h>      // 
#include <stdlib.h>     // for rand, malloc, and free
#include <string.h>     // for strncmp and strncpy
#include <math.h>       // for sin and cos
#include <limits.h>     // for MIN/MAX values of integers
#include <time.h>       // for file stream time stamps
#include <LabJackM.h>   // duh

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
    int ainum, aonum, metanum;
    dconf->connection = -1;
    dconf->serial[0] = '\0';
    dconf->ip[0] = '\0';
    dconf->gateway[0] = '\0';
    dconf->subnet[0] = '\0';
    dconf->naich = 0;
    dconf->naoch = 0;
    dconf->samplehz = -1.;
    dconf->settleus = 1.;
    dconf->nsample = LCONF_DEF_NSAMPLE;
    dconf->FS.buffer = NULL;
    dconf->FS.file = NULL;
    dconf->FS.samples_per_read = 0;
    for(metanum=0; metanum<LCONF_MAX_META; metanum++)
        dconf->meta[metanum].param[0] = '\0';
    for(ainum=0; ainum<LCONF_MAX_NAICH; ainum++){
        dconf->aich[ainum].channel = -1;
        dconf->aich[ainum].nchannel = LCONF_DEF_AI_NCH;
        dconf->aich[ainum].range = LCONF_DEF_AI_RANGE;
        dconf->aich[ainum].resolution = LCONF_DEF_AI_RES;
    }
    for(aonum=0; aonum<LCONF_MAX_NAOCH; aonum++){
        dconf->aoch[aonum].channel = -1;
        dconf->aoch[aonum].frequency = -1;
        dconf->aoch[aonum].signal = CONSTANT;
        dconf->aoch[aonum].amplitude = LCONF_DEF_AO_AMP;
        dconf->aoch[aonum].offset = LCONF_DEF_AO_OFF;
        dconf->aoch[aonum].duty = LCONF_DEF_AO_DUTY;
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
    return dconf[devnum].naich;
}

int nostream_config(DEVCONF* dconf, const unsigned int devnum){
    return dconf[devnum].naoch;
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
    int devnum=-1, ainum=-1, aonum=-1;
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
            dconf[devnum].aoch[aonum].signal = CONSTANT;
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
            if(strncmp(value,"constant",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = CONSTANT;
            else if(strncmp(value,"sine",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = SINE;
            else if(strncmp(value,"square",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = SQUARE;
            else if(strncmp(value,"triangle",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = TRIANGLE;
            else if(strncmp(value,"noise",LCONF_MAX_STR)==0) dconf[devnum].aoch[aonum].signal = NOISE;
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
                fprintf(stderr,"LOAD: AOduty expected float, but found: %s\n", value);
                goto lconf_loadfail;
            }else if(ftemp<0. || ftemp>1.){
                fprintf(stderr,"LOAD: AOduty must be between 0. and 1.  Found %f.\n", ftemp);
                goto lconf_loadfail;
            }
            dconf[devnum].aoch[aonum].duty = ftemp;
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
    int ainum,aonum,metanum,temp;
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
    
    fprintf(ff,"\n# Analog Inputs\n");
    for(ainum=0; ainum<dconf[devnum].naich; ainum++){

        write_aiint(aichannel,channel)
        write_aiint(ainegative,nchannel)
        write_aiflt(airange,range)
        write_aiint(airesolution,resolution)
        fprintf(ff,"\n");
    }

    fprintf(ff,"# Analog Outputs\n");
    for(aonum=0; aonum<dconf[devnum].naoch; aonum++){
        write_aoint(aochannel,channel)
        if(dconf[devnum].aoch[aonum].signal==CONSTANT)  fprintf(ff,"aosignal constant\n");
        else if(dconf[devnum].aoch[aonum].signal==SINE)  fprintf(ff,"aosignal sine\n");
        else if(dconf[devnum].aoch[aonum].signal==SQUARE)  fprintf(ff,"aosignal square\n");
        else if(dconf[devnum].aoch[aonum].signal==TRIANGLE)  fprintf(ff,"aosignal triangle\n");
        else if(dconf[devnum].aoch[aonum].signal==NOISE)  fprintf(ff,"aosignal noise\n");
        else fprintf(ff,"signal ***\n");
        write_aoflt(aofrequency,frequency)
        write_aoflt(aoamplitude,amplitude)
        write_aoflt(aooffset,offset)
        write_aoflt(aoduty,duty)
        fprintf(ff,"\n");
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
    int reg_negative,reg_range,reg_resolution;
    // Registers for analog output
    int aonum,naoch;
    int reg_target, reg_buffersize, reg_enable, reg_buffer, reg_loopsize, reg_setloop;
    unsigned int samples;
    double ei,er,gi,gr;     // exponent and generator (real and imaginary)
    int dummy;  // a dummy variable
    char flag;
    double ftemp;   // temporary floating point
    unsigned int index,itemp;   // temporary integers
    unsigned int channel;

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
        // We're going to use it to index a memory address, after all.
        if(channel>=LCONF_MAX_AICH){
            fprintf(stderr,"UPLOAD: Analog input %d points to channel %d, which is out of range [0-%d]\n",
                    ainum, channel, LCONF_MAX_AICH);
            goto lconf_uploadfail;
        }

        airegisters(channel,&reg_negative,&reg_range,&reg_resolution);
        // Write the negative channel
        err = LJM_eWriteAddress(  handle,
                            reg_negative,
                            LJM_UINT16,
                            dconf[devnum].aich[ainum].nchannel);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to update analog input %d, AI%d negative input to %d\n", 
                    ainum, channel, dconf[devnum].aich[ainum].nchannel);
            goto lconf_uploadfail;
        }
        // Write the range
        err = LJM_eWriteAddress(  handle,
                            reg_range,
                            LJM_FLOAT32,
                            dconf[devnum].aich[ainum].range);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to update analog input %d, AI%d range input to %f\n", 
                    ainum, channel, dconf[devnum].aich[ainum].range);
            goto lconf_uploadfail;
        }
        // Write the resolution
        err = LJM_eWriteAddress(  handle,
                            reg_resolution,
                            LJM_UINT16,
                            dconf[devnum].aich[ainum].resolution);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to update analog input %d, AI%d resolution index to %d\n", 
                    ainum, channel, dconf[devnum].aich[ainum].resolution);
            goto lconf_uploadfail;
        }

    }

    // Set up all analog outputs
    for(aonum=0;aonum<naoch;aonum++){
        // Find the address of the corresponding channel number
        // Start with DAC0 and increment by 2 for each channel number
        LJM_NameToAddress("DAC0",&reg_temp,&type_temp);
        channel = reg_temp + 2*dconf[devnum].aoch[aonum].channel;
        // "channel" now contains the address for DAC#
        // Now, get the buffer configuration registers
        aoregisters(aonum, &reg_target, &reg_buffersize, &reg_enable, &reg_buffer, &reg_loopsize, &reg_setloop);

        // Disable the output buffer
        err = LJM_eWriteAddress(handle, reg_enable, LJM_UINT32, 0);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to disable analog output buffer STREAM_OUT%d_ENABLE (%d)\n", 
                    aonum, reg_enable);
            goto lconf_uploadfail;
        }
        // Configure the stream target to point to the DAC channel
        err = LJM_eWriteAddress(handle, reg_target, LJM_UINT32, channel);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write target AO%d (%d), to STREAM_OUT%d_TARGET (%d)\n", 
                    dconf[devnum].aoch[aonum].channel, channel, aonum, reg_target);
            goto lconf_uploadfail;
        }
        // Configure the buffer size
        err = LJM_eWriteAddress(handle, reg_buffersize, LJM_UINT32, LCONF_MAX_AOBUFFER);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write AO%d buffer size, %d, to STREAM_OUT%d_BUFFER_SIZE (%d)\n", 
                    dconf[devnum].aoch[aonum].channel, LCONF_MAX_AOBUFFER, aonum, reg_buffer);
            goto lconf_uploadfail;
        }

        // Enable the output buffer
        err = LJM_eWriteAddress(handle, reg_enable, LJM_UINT32, 1);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to enable analog output buffer STREAM_OUT%d_ENABLE (%d)\n", 
                    aonum, reg_enable);
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
        // Case out the different signal types
        // CONSTANT
        if(dconf[devnum].aoch[aonum].signal==CONSTANT){
            for(index=0; index<samples; index++)
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_buffer, LJM_FLOAT32,\
                    dconf[devnum].aoch[aonum].offset);
        // SINE
        }else if(dconf[devnum].aoch[aonum].signal==SINE){
            // initialize generators
            gr = 1.; gi = 0;
            ftemp = TWOPI / samples;
            er = cos( ftemp );
            ei = sin( ftemp );
            for(index=0; index<samples; index++){
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_buffer, LJM_FLOAT32,\
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
        // SQUARE
        }else if(dconf[devnum].aoch[aonum].signal==SQUARE){
            itemp = samples*dconf[devnum].aoch[aonum].duty;
            if(itemp>samples) itemp = samples;
            // Calculate the high level
            ftemp = dconf[devnum].aoch[aonum].offset + \
                        dconf[devnum].aoch[aonum].amplitude;
            for(index=0; index<itemp; index++)
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_buffer, LJM_FLOAT32, ftemp);
            // Calculate the low level
            ftemp = dconf[devnum].aoch[aonum].offset - \
                        dconf[devnum].aoch[aonum].amplitude;
            for(;index<samples;index++)
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_buffer, LJM_FLOAT32, ftemp);
        // TRIANGLE
        }else if(dconf[devnum].aoch[aonum].signal==TRIANGLE){
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
                err = err ? err : LJM_eWriteAddress(handle, reg_buffer, LJM_FLOAT32, gr);
                gr += er;
            }
            // Calculate the new slope
            er = 2.*dconf[devnum].aoch[aonum].amplitude/(samples-itemp);
            for(; index<samples; index++){
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_buffer, LJM_FLOAT32, gr);
                gr -= er;
            }
        // NOISE
        }else if(dconf[devnum].aoch[aonum].signal==NOISE){
            for(index=0; index<samples; index++){
                // generate a random number between -1 and 1
                ftemp = (2.*((double)rand())/RAND_MAX - 1.);
                ftemp *= dconf[devnum].aoch[aonum].amplitude;
                ftemp += dconf[devnum].aoch[aonum].offset;
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_buffer, LJM_FLOAT32, ftemp);
            }
        }

        // Check for a buffer write error
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write data to the OStream STREAM_OUT%d_BUF_F32 (%d)\n", 
                    aonum, reg_buffer);
            goto lconf_uploadfail;
        }

        // Set the loop size
        err = LJM_eWriteAddress(handle, reg_loopsize, LJM_UINT32, samples);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write loop size %d to STREAM_OUT%d_LOOP_SIZE (%d)\n", 
                    samples, aonum, reg_enable);
            goto lconf_uploadfail;
        }
        // Update loop settings
        err = LJM_eWriteAddress(handle, reg_setloop, LJM_UINT32, 1);
        if(err){
            fprintf(stderr,"UPLOAD: Failed to write 1 to STREAM_OUT%d_SET_LOOP (%d)\n", 
                    aonum, reg_setloop);
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
    int err, aonum, ainum, reg_temp, type_temp, reg_negative, reg_resolution, reg_range;
    double ftemp;
    int itemp;
    int handle, naich, naoch;
    int DeviceType,ConnectionType,SerialNumber,IPAddress,Port,MaxBytesPerMB;
    char NAME[50];

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
        out->aich[ainum].channel = dconf[devnum].aich[ainum].channel;

        // Get the addresses for that channel's configuration data
        airegisters(out->aich[ainum].channel,
                    &reg_negative, &reg_range, &reg_resolution);
        // Read the negative channel
        err = LJM_eReadAddress(  handle,
                            reg_negative,
                            LJM_UINT16,
                            &ftemp);
        if(err){
            fprintf(stderr,"DOWNLOAD: Failed to read the negative AI channel number at address %d\n", 
                    reg_negative);
            goto lconf_downloadfail;
        }
        out->aich[ainum].nchannel = (int) ftemp;

        // Read the range
        err = LJM_eReadAddress( handle,
                            reg_range,
                            LJM_FLOAT32,
                            &ftemp);
        if(err){
            fprintf(stderr,"DOWNLOAD: Failed to read the AI channel range at address %d\n", 
                    reg_range);
            goto lconf_downloadfail;
        }
        out->aich[ainum].range = ftemp;

        // Read the resolution
        err = LJM_eReadAddress( handle,
                            reg_resolution,
                            LJM_UINT16,
                            &ftemp);
        if(err){
            fprintf(stderr,"DOWNLOAD: Failed to read the AI channel resolution index at address %d\n", 
                    reg_resolution);
            goto lconf_downloadfail;
        }
        out->aich[ainum].resolution = (int) ftemp;
    }

    // Loop through the analog output channels
    for(aonum=0; aonum<naoch; aonum++){
        // Determine the channel from the STREAM_OUT#_TARGET
        sprintf(NAME, "STREAM_OUT%d_TARGET", aonum);
        LJM_eReadName(handle, NAME, &ftemp);
        LJM_NameToAddress("DAC0",&reg_temp,&type_temp);
        // calculate a channel number based on the target's distance from DAC0
        out->aoch[aonum].channel = ((int)ftemp - reg_temp)/2;
        // calculate a signal frequency based on the loop size
        sprintf(NAME, "STREAM_OUT%d_LOOP_SIZE", aonum);
        LJM_eReadName(handle, NAME, &ftemp);
        // borrow the samplehz parameter to estimate signal frequency
        if(ftemp>0)
            out->aoch[aonum].frequency = dconf[devnum].samplehz / ftemp;
        else
            out->aoch[aonum].frequency = -1;
    }

    return LCONF_NOERR;

    lconf_downloadfail:
    fprintf(stderr,"LCONFIG: Error downloading from device %d\n", devnum);
    LJM_ErrorToString(err, err_str);
    fprintf(stderr,"%s\n",err_str);
    return LCONF_ERROR;
}





void show_config(DEVCONF* dconf, const unsigned int devnum){
    int ainum,aonum,metanum,metacnt;
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
        if(dconf[devnum].aoch[aonum].signal==CONSTANT)  print_color("Signal", "constant", "?");
        else if(dconf[devnum].aoch[aonum].signal==SINE)  print_color("Signal", "sine", "?");
        else if(dconf[devnum].aoch[aonum].signal==SQUARE)  print_color("Signal", "square", "?");
        else if(dconf[devnum].aoch[aonum].signal==TRIANGLE)  print_color("Signal", "triangle", "?");
        else if(dconf[devnum].aoch[aonum].signal==NOISE)  print_color("Signal", "noise", "?");
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



