
/*
  This file is part of the LCONFIG laboratory configuration system.

    LCONFIG is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LCONFIG is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <https://www.gnu.org/licenses/>.

    Authored by C.Martin crm28@psu.edu
*/

#include <stdio.h>      // 
#include <stdlib.h>     // for rand, malloc, and free
#include <unistd.h>     // for sleep
#include <string.h>     // for strncmp and strncpy
#include <math.h>       // for sin and cos
#include <limits.h>     // for MIN/MAX values of integers
#include <time.h>       // For time stamp formatting
#include <sys/time.h>   // For high-resolution time in COM timeout
#include <LabJackM.h>   // duh
#include <stdint.h>     // being careful about bit widths
#include <sys/sysinfo.h>    // for ram overload checking
#include "lconfig.h"
#include "lcmap.h"

// macros for writing lines to configuration files in WRITE_CONFIG()
#define write_str(param,child) dconf->child[0]!='\0' ? fprintf(ff, "%s \"%s\"\n", #param, dconf->child) : (int) 0
#define write_int(param,child) fprintf(ff, "%s %d\n", #param, dconf->child)
#define write_flt(param,child) fprintf(ff, "%s %f\n", #param, dconf->child)
#define write_aistr(param,child) dconf->aich[ainum].child[0]!='\0' ? fprintf(ff, "%s \"%s\"\n", #param, dconf->aich[ainum].child) : (int) 0
#define write_aiint(param,child) fprintf(ff, "%s %d\n", #param, dconf->aich[ainum].child)
#define write_aiflt(param,child) fprintf(ff, "%s %f\n", #param, dconf->aich[ainum].child)
#define write_aostr(param,child) dconf->aoch[aonum].child[0]!='\0' ? fprintf(ff, "%s \"%s\"\n", #param, dconf->aoch[aonum].child) : (int) 0
#define write_aoint(param,child) fprintf(ff, "%s %d\n", #param, dconf->aoch[aonum].child)
#define write_aoflt(param,child) fprintf(ff, "%s %f\n", #param, dconf->aoch[aonum].child)
#define write_efstr(param,child) dconf->efch[efnum].child[0]!='\0' ? fprintf(ff, "%s \"%s\"\n", #param, dconf->efch[efnum].child) : (int) 0
#define write_efint(param,child) fprintf(ff, "%s %d\n", #param, dconf->efch[efnum].child)
#define write_efflt(param,child) fprintf(ff, "%s %f\n", #param, dconf->efch[efnum].child)
// macro for testing strings
#define streq(a,b) strncmp(a,b,LCONF_MAX_STR)==0

// Macros for error handling
#define print_error(message, ...)   fprintf(stderr, LC_FONT_RED message LC_FONT_NULL, ##__VA_ARGS__)
#define print_warning(message, ...) fprintf(stderr, LC_FONT_CYAN message LC_FONT_NULL, ##__VA_ARGS__)

#define loadfail(){\
    print_error("LOAD: Connection Number: %d, File Name: \"%s\"\n",\
                    devnum,filename);\
    fclose(ff);\
    return LCONF_ERROR;\
}

#define openfail(){\
    print_error("OPEN: IP: %s, Serial: %s, Name: \"%s\"\n",\
            dconf->ip, dconf->serial, dconf->name);\
    LJM_ErrorToString(err, err_str);\
    print_error("LJM Error Code: %s\n",err_str);\
    lc_close(dconf);\
    return LCONF_ERROR;\
}

#define uploadfail(){\
    print_error("UPLOAD: IP: %s, Serial: %s, Name: \"%s\"\n",\
            dconf->ip, dconf->serial, dconf->name);\
    LJM_ErrorToString(err, err_str);\
    print_error("LJM Error Code: %s\n",err_str);\
    return LCONF_ERROR;\
}

#define startfail(){\
    print_error("START_DATA_STREAM: IP: %s, Serial: %s, Name: \"%s\"\n",\
            dconf->ip, dconf->serial, dconf->name);\
    LJM_ErrorToString(err, err_str);\
    print_error("%s\n",err_str);\
    return LCONF_ERROR;\
}


/*......................
. Codes for color/font
.......................*/
// Help on Linux terminal control characters
// $man console_codes
// gets the job done.

#define LC_FONT_NULL "\x1b[0m"
#define LC_FONT_RED "\x1b[31m"
#define LC_FONT_GREEN "\x1b[32m"
#define LC_FONT_YELLOW "\x1b[33m"
#define LC_FONT_BLUE "\x1b[34m"
#define LC_FONT_MAGENTA "\x1b[35m"
#define LC_FONT_CYAN "\x1b[36m"
#define LC_FONT_BOLD "\x1b[1m"

// These constants are available for run-time evaluation
// Clear all formatting; return to console defaults
static const char lc_font_null[] = LC_FONT_NULL;
// Font colors
static const char lc_font_red[] =  LC_FONT_RED;
static const char lc_font_green[] =  LC_FONT_GREEN;
static const char lc_font_yellow[] =  LC_FONT_YELLOW;
static const char lc_font_blue[] =  LC_FONT_BLUE;
static const char lc_font_magenta[] =  LC_FONT_MAGENTA;
static const char lc_font_cyan[] =  LC_FONT_CYAN;
// Bold font
static const char lc_font_bold[] =  LC_FONT_BOLD;



#define SHOW_PARAM "    %-18s : "

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


// Helper funciton to calculate elapsed time in milliseconds from two timeval 
// structs.  
double difftime_ms(struct timeval *a, struct timeval *b){
    return ((double)((a->tv_sec - b->tv_sec)*1000)) - ((double)(a->tv_usec - b->tv_usec))/1000;
}

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



void read_param(FILE *source, char *param){
    int length, quote;
    int charin;
    // clear the parameter
    // if the fscanf operation fails, this will return gracefully
    param[0] = '\0';
    length = 0;
    quote = 0;
    
    while((charin=fgetc(source)) > 0 && length < LCONF_MAX_STR-1){
        // Toggle the quote state if this is a quotation mark
        if(charin == '"'){
            if(quote){
                quote = 0;
                break;
            }else{
                quote = 1;
            }
        // If not in a quote and we found a comment character
        }else if(!quote && charin == '#'){
            charin = fgetc(source);
            // If there are two ## in a row, this is the end of the 
            // configuraiton header.  Force EOF condition and break
            if(charin=='#'){
                // Rewind so the read_param operation will be stuck on
                // the ## character combination
                fseek(source, -2, SEEK_CUR);
                break;
            }
            // Read in the rest of the line
            while(charin>0 && charin!='\n'){
                charin = fgetc(source);
            }
            // If the word is empty keep reading, otherwise, get out.
            if(length>0){
                break;
            }
        // If not in quotes, force lower-case
        }else if(!quote && charin>='A' && charin<='Z'){
            charin += ('a' - 'A');
            param[length++] = charin;
        // If this is a whitespace character
        }else if(strchr(" \n\r\t", charin)){
            // In quotes, witespace is allowed
            if(quote){
                param[length++] = charin;
            // If no text has been discovered yet, keep reading.  
            // Otherwise that's the end of the parameter.
            }else if(length>0){
                break;
            }
        // If this is in the ascii set
        }else if(charin >= 0x20 && charin <= 0x7F){
            param[length++] = charin;
        // For non-ascii or non-whitespace control characters, panic!
        }else{
            break;
        }
    }
    param[length] = '\0';
}


void init_config(lc_devconf_t* dconf){
    int ainum, aonum, metanum, efnum, comnum;
    // Global configuration
    dconf->connection = LC_CON_NONE;
    dconf->connection_act = LC_CON_NONE;
    dconf->device = LC_DEV_ANY;
    dconf->device_act = LC_DEV_NONE;
    dconf->serial[0] = '\0';
    dconf->name[0] = '\0';
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
    dconf->trigmem = 0;
    dconf->trigstate = LC_TRIG_IDLE;
    dconf->trigedge = LC_EDGE_RISING;
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
        dconf->aich[ainum].calslope = 1.;
        dconf->aich[ainum].calzero = 0.;
        dconf->aich[ainum].label[0] = '\0';
        dconf->aich[ainum].calunits[0] = '\0';
    }
    // Digital input streaming
    dconf->distream = 0;
    // Analog Outputs
    dconf->naoch = 0;
    for(aonum=0; aonum<LCONF_MAX_NAOCH; aonum++){
        dconf->aoch[aonum].channel = -1;
        dconf->aoch[aonum].frequency = -1;
        dconf->aoch[aonum].signal = LC_AO_CONSTANT;
        dconf->aoch[aonum].amplitude = LCONF_DEF_AO_AMP;
        dconf->aoch[aonum].offset = LCONF_DEF_AO_OFF;
        dconf->aoch[aonum].duty = LCONF_DEF_AO_DUTY;
        dconf->aoch[aonum].label[0] = '\0';
    }
    // Flexible IO
    dconf->nefch = 0;
    dconf->effrequency = 0.;
    for(efnum=0; efnum<LCONF_MAX_NEFCH; efnum++){
        dconf->efch[efnum].channel = -1;
        dconf->efch[efnum].signal = LC_EF_NONE;
        dconf->efch[efnum].edge = LC_EDGE_RISING;
        dconf->efch[efnum].debounce = LC_EF_DEBOUNCE_NONE;
        dconf->efch[efnum].direction = LC_EF_INPUT;
        dconf->efch[efnum].time = 0.;
        dconf->efch[efnum].duty = .5;
        dconf->efch[efnum].phase = 0.;
        dconf->efch[efnum].counts = 0;
        dconf->efch[efnum].label[0] = '\0';
    }
    // Communications
    dconf->ncomch = 0;
    for(comnum=0; comnum<LCONF_MAX_NCOMCH; comnum++){
        dconf->comch[comnum].type = LC_COM_NONE;
        dconf->comch[comnum].pin_in = -1;
        dconf->comch[comnum].pin_out = -1;
        dconf->comch[comnum].pin_clock = -1;
        dconf->comch[comnum].rate = -1;
    }
    dconf->RB.buffer=NULL;
}


// Initialization declares the buffer memory and initializes all
// internal variables to describe the buffer's size and read/write oeprations
int init_buffer(lc_ringbuf_t* RB,    // Ring buffer struct to initialize
                const unsigned int channels, // The number of channels in the stream
                const unsigned int samples_per_read, // The samples (scans) per R/W block
                const unsigned int blocks){ // The number of R/W blocks to buffer
    
    struct sysinfo sinf;
    long unsigned int bytes;

    if(RB->buffer){
        print_error("lc_ringbuf_t: Buffer not free!\n");
        return LCONF_ERROR;
    }

    RB->samples_per_read = samples_per_read;
    RB->samples_read = 0;
    RB->samples_streamed = 0;
    RB->channels = channels;
    RB->blocksize_samples = samples_per_read * channels;
    RB->size_samples = RB->blocksize_samples * blocks;
    RB->read = 0;
    RB->write = 0;
    // Do some sanity checking on the buffer size
    sysinfo(&sinf);
    bytes = RB->size_samples * sizeof(double);
    if(sinf.freeram * 0.9 < bytes){
        print_error("INIT_BUFFER: Not enough available memory: aborting!\n");
        RB->buffer = NULL;
        return LCONF_ERROR;
    }
    RB->buffer = malloc(bytes);
    return LCONF_NOERR;
}

int isempty_buffer(lc_ringbuf_t* RB){
    return (RB->read == RB->write);
}

int isfull_buffer(lc_ringbuf_t* RB){
    return (RB->read == RB->size_samples);
}

// Returns a pointer to the start of the next block to be written
double* get_write_buffer(lc_ringbuf_t* RB){
    if(RB->buffer == NULL)
        return NULL;
    return &RB->buffer[RB->write];
}

// Writing to the buffer is done externally by the LJM module, but after the
// data is written, the buffer variables need to be advanced appropriately.
void service_write_buffer(lc_ringbuf_t* RB){
    // advance the write index
    RB->write += RB->blocksize_samples;
    RB->samples_streamed += RB->samples_per_read;
    // Check for wrapping
    if(RB->write + RB->blocksize_samples > RB->size_samples)
        RB->write = 0;
    // If the buffer was just filled
    if(RB->write == RB->read)
        RB->read = RB->size_samples;
}

// Returns a pointer to the start of the next block to be read
// returns NULL if the buffer is empty
double* get_read_buffer(lc_ringbuf_t* RB){
    if(RB->buffer == NULL)
        return NULL;
    // if the buffer is full
    if(RB->read == RB->size_samples)
        return &RB->buffer[RB->write];
    // if the buffer is empty
    else if(RB->read == RB->write)
        return NULL;
    // Normal operation
    else
        return &RB->buffer[RB->read];
}

// Updates the buffer's read index once a read operation is complete.
void service_read_buffer(lc_ringbuf_t *RB){
    // if the buffer is full
    if(RB->read == RB->size_samples)
        RB->read = RB->write;
    // If the buffer is empty
    else if(RB->read == RB->write)
        return;
    RB->read += RB->blocksize_samples;
    RB->samples_read += RB->samples_per_read;
    // Check for wrapping
    if(RB->read + RB->blocksize_samples > RB->size_samples)
        RB->read = 0;
}

// Free the buffer's memory
void clean_buffer(lc_ringbuf_t* RB){
    if(RB->buffer){
        free(RB->buffer);
        RB->buffer = NULL;
    }
    RB->samples_per_read = 0;
    RB->channels = 0;
    RB->blocksize_samples = 0;
    RB->size_samples = 0;
    RB->read = 0;
    RB->write = 0;
}


/*....................................
.
.   Diagnostic
.       "Public" functions
.
......................................*/

int lc_nistream(lc_devconf_t* dconf){
    int out;
    out = dconf->naich;
    if(dconf->distream)
        out += 1;
    return out;
}

int lc_isopen(lc_devconf_t* dconf){
    return (dconf->handle >= 0);
}

int lc_nostream(lc_devconf_t* dconf){
    int out;
    out = dconf->naoch;
    return out;
}

int lc_ndev(lc_devconf_t* dconf, const unsigned int devmax){
    int ii;
    for(ii=0; ii<devmax; ii++)
        if(dconf[ii].connection<0)
            return ii;
    return devmax;
}

void lc_aichannels(const lc_devconf_t* dconf, int *min, int *max){
    switch(dconf->device_act){
    case LJM_dtT4:
        *min = 0;
        *max = 11;
    break;
    case LJM_dtT7:
        *min = 0;
        *max = 14;
    break;
    default:
        *min = -1;
        *max = -1;
    }
}

void lc_aochannels(const lc_devconf_t* dconf, int *min, int *max){
    switch(dconf->device_act){
    case LJM_dtT4:
        *min = 0;
        *max = 1;
    break;
    case LJM_dtT7:
        *min = 0;
        *max = 1;
    break;
    default:
        *min = -1;
        *max = -1;
    }
}

void lc_efchannels(const lc_devconf_t* dconf, int *min, int *max){
    switch(dconf->device_act){
    case LJM_dtT4:
        *min = 4;
        *max = 9;
    break;
    case LJM_dtT7:
        *min = 0;
        *max = 7;
    break;
    default:
        *min = -1;
        *max = -1;
    }
}

void lc_diochannels(const lc_devconf_t* dconf, int *min, int *max){
    switch(dconf->device_act){
    case LJM_dtT4:
        *min = 4;
        *max = 19;
    break;
    case LJM_dtT7:
        *min = 0;
        *max = 22;
    break;
    default:
        *min = -1;
        *max = -1;
    }
}


/*....................................
.
.   Algorithm Definition
.       "Public" functions
.
......................................*/


int lc_load_config(lc_devconf_t* dconf, const unsigned int devmax, const char* filename){
    int devnum=-1, ainum=-1, aonum=-1, efnum=-1, comnum=-1;
    int itemp, itemp2;
    float ftemp;
    char param[LCONF_MAX_STR], value[LCONF_MAX_STR];
    char metatype;
    char ctemp;
    FILE* ff;

    if(devmax>LCONF_MAX_NDEV){
        print_error( "LCONFIG: LOAD_CONFIG called with %d devices, but only %d are supported.\n",
                devmax, LCONF_MAX_NDEV);
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
        print_error( "LOAD: Failed to open the configuration file \"%s\".\n",filename);
        return LCONF_ERROR;
    }
    // Read the entire file
    while(!feof(ff)){
        // Read in the parameter name
        read_param(ff,param);
        // If the read is finished, then dump out
        if(param[0] == '\0')
            break;
        
        // Read in the parameter's value
        read_param(ff,value);
        // If we find EOF while looking for a value, raise an error
        if(value[0] == '\0'){
            print_error( "LOAD: Unexpected end of file while reading parameter: %s\n", param);
            loadfail();
        }
        

        // Case out the parameters
        //
        // The CONNECTION parameter
        //
        if(streq(param,"connection")){
            // increment the device index
            devnum++;
            if(devnum>=devmax){
                print_error("LOAD: Too many devices specified. Maximum allowed is %d.\n", devmax);
                loadfail();
            }
            if(lcm_get_value(lcm_connection, value, &dconf[devnum].connection)){
                print_error("LOAD: Unrecognized connection type: %s\n",value);
                print_error("%s\n","Expected \"usb\", \"eth\", or \"any\".");
                loadfail();
            }
        //
        // What if CONNECTION isn't first?
        //
        }else if(devnum<0){
            print_error("LOAD: The first configuration parameter must be \"connection\".\n\
Found \"%s\"\n", param);
            return LCONF_ERROR;
        }else if(streq(param,"device")){
            if(lcm_get_value(lcm_device, value, &dconf[devnum].device)){
                print_error( "LOAD: Unrecognized device type: %s\n", value);
                print_error( "Expected \"any\" \"t7\" or \"t4\"\n");
                loadfail();
            }
        //
        // The NAME parameter
        //
        }else if(streq(param,"name")){
            strncpy(dconf[devnum].name,value,LCONF_MAX_NAME);
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
                print_error("LOAD: Illegal SAMPLEHZ value \"%s\". Expected float.\n",value);
                loadfail();
            }
            dconf[devnum].samplehz = ftemp;
        //
        // The SETTLEUS parameter
        //
        }else if(streq(param,"settleus")){
            if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: Illegal SETTLEUS value \"%s\". Expected float.\n",value);
                loadfail();
            }
            dconf[devnum].settleus = ftemp;
        //
        // The NSAMPLE parameter
        //
        }else if(streq(param,"nsample")){
            if(sscanf(value,"%d",&itemp)!=1){
                print_error("LOAD: Illegal NSAMPLE value \"%s\".  Expected integer.\n",value);
                loadfail();
            }else if(itemp <= 0)
                print_error("LOAD: **WARNING** NSAMPLE value was less than or equal to zero.\n"
                       "      Fix this before initiating a stream!\n");
            dconf[devnum].nsample = itemp;
        //
        // The AICHANNEL parameter
        //
        }else if(streq(param,"aichannel")){
            ainum = dconf[devnum].naich;
            // Check for an array overrun
            if(ainum>=LCONF_MAX_NAICH){
                print_error("LOAD: Too many AIchannel definitions.  Only %d are allowed.\n",LCONF_MAX_NAICH);
                loadfail();
            // Make sure the channel number is a valid integer
            }else if(sscanf(value,"%d",&itemp)!=1){
                print_error("LOAD: Illegal AIchannel number \"%s\". Expected integer.\n",value);
                loadfail();
            }
            // Make sure the channel number is in range for the T7.
            if(itemp<0 || itemp>LCONF_MAX_AICH){
                print_error("LOAD: AIchannel number %d is out of range [0-%d].\n", itemp, LCONF_MAX_AICH);
                loadfail();
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
            // Test that there is a configured channel already
            if(ainum<0){
                print_error("LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                loadfail();
            }
            if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: Illegal AIrange number \"%s\". Expected a float.\n",value);
                loadfail();
            }
            dconf[devnum].aich[ainum].range = ftemp;
        //
        // The AINEGATIVE parameter
        //
        }else if(streq(param,"ainegative")){
            // Test that there is a configured channel already
            if(ainum<0){
                print_error("LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                loadfail();
            }
            // if value is a string specifying ground
            if(strncmp(value,"ground",LCONF_MAX_STR)==0){
                itemp = LJM_GND;
            // If the value is a string specifying a differential connection
            }else if(strncmp(value,"differential",LCONF_MAX_STR)==0){
                // set the channel to be one greater than the primary aichannel.
                itemp = dconf[devnum].aich[ainum].channel+1;
            }else if(sscanf(value,"%d",&itemp)!=1){
                print_error("LOAD: Illegal AIneg index \"%s\". Expected an integer, \"differential\", or \"ground\".\n",value);
                loadfail();
            }else if(itemp == LJM_GND){
                // bypass further error checking if the index referrs to ground.
            }else if(itemp < 0 || itemp > LCONF_MAX_AICH){
                print_error("LOAD: AIN negative channel number %d is out of range [0-%d].\n", itemp, LCONF_MAX_AICH);
                loadfail();
            }else if(itemp%2-1){
                print_error("LOAD: Even channels cannot serve as negative channels\n\
and negative channels cannot opperate in differential mode.\n");
                loadfail();
            }else if(itemp-dconf[devnum].aich[ainum].channel!=1){
                print_error("LOAD: Illegal choice of negative channel %d for positive channel %d.\n\
Negative channels must be the odd channel neighboring the\n\
even channels they serve.  (e.g. AI0/AI1)\n", itemp, dconf[devnum].aich[ainum].channel);
                loadfail();
            }
            dconf[devnum].aich[ainum].nchannel = itemp;
        //
        // The AIRESOLUTION parameter
        //
        }else if(streq(param,"airesolution")){
            if(ainum<0){
                print_error("LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                loadfail();
            }

            if(sscanf(value,"%d",&itemp)!=1){
                print_error("LOAD: Illegal AIres index \"%s\". Expected an integer.\n",value);
                loadfail();
            }else if(itemp < 0 || itemp > LCONF_MAX_AIRES){
                print_error("LOAD: AIres index %d is out of range [0-%d].\n", itemp, LCONF_MAX_AIRES);
                loadfail();
            }
            dconf[devnum].aich[ainum].resolution = itemp;
        //
        // The AICALSLOPE parameter
        //
        }else if(streq(param,"aicalslope")){
            ainum = dconf[devnum].naich-1;
            if(ainum<0){
                print_error("LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                loadfail();
            // Make sure the coefficient is a valid float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: Illegal AIcalslope number \"%s\". Expected float.\n",value);
                loadfail();
            }
            dconf[devnum].aich[ainum].calslope = ftemp;
        //
        // The AICALZERO parameter
        //
        }else if(streq(param,"aicalzero")){
            ainum = dconf[devnum].naich-1;
            if(ainum<0){
                print_error("LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                loadfail();
            // Make sure the offset is a valid float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: Illegal AIcaloffset number \"%s\". Expected float.\n",value);
                loadfail();
            }
            dconf[devnum].aich[ainum].calzero = ftemp;
        //
        // The AICALUNITS parameter
        //
        }else if(streq(param,"aicalunits")){
            ainum = dconf[devnum].naich-1;
            if(ainum<0){
                print_error("LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                loadfail();
            }
            strncpy(dconf[devnum].aich[ainum].calunits, value, LCONF_MAX_STR);
        //
        // The AILABEL parameter
        //
        }else if(streq(param,"ailabel")){
            ainum = dconf[devnum].naich-1;
            if(ainum<0){
                print_error("LOAD: Cannot set analog input parameters before the first AIchannel parameter.\n");
                loadfail();
            }
            strncpy(dconf[devnum].aich[ainum].label, value, LCONF_MAX_STR);
        //
        // The DISTREAM parameter
        //
        }else if(streq(param,"distream")){
            if(sscanf(value, "%d", &itemp)!=1){
                print_error(
                        "LOAD: Expected integer DISTREAM, but found \"%s\"\n", 
                        value);
                loadfail();
            }else if(itemp < 0 || itemp > 0xFFFF){
                print_error(
                        "LOAD: DISTREAM must be a positive 16-bit mask. Found %d\n",
                        itemp);
            }
            dconf[devnum].distream = itemp;
        //
        // The AOCHANNEL parameter
        //
        }else if(streq(param,"aochannel")){
            aonum = dconf[devnum].naoch;
            // Check for an array overrun
            if(aonum>=LCONF_MAX_NAOCH){
                print_error("LOAD: Too many AOchannel definitions.  Only %d are allowed.\n",LCONF_MAX_NAOCH);
                loadfail();
            // Make sure the channel number is a valid integer
            }else if(sscanf(value,"%d",&itemp)!=1){
                print_error("LOAD: Illegal AOchannel number \"%s\". Expected integer.\n",value);
                loadfail();
            }
            // Make sure the channel number is in range for the T7.
            if(itemp<0 || itemp>LCONF_MAX_AOCH){
                print_error("LOAD: AOchannel number %d is out of range [0-%d].\n", itemp, LCONF_MAX_AOCH);
                loadfail();
            }
            // increment the number of active channels
            dconf[devnum].naoch++;
            // Set the channel and all the default parameters
            dconf[devnum].aoch[aonum].channel = itemp;
            dconf[devnum].aoch[aonum].signal = LC_AO_CONSTANT;
            dconf[devnum].aoch[aonum].amplitude = LCONF_DEF_AO_AMP;
            dconf[devnum].aoch[aonum].offset = LCONF_DEF_AO_OFF;
            dconf[devnum].aoch[aonum].duty = LCONF_DEF_AO_DUTY;
        //
        // The AOSIGNAL parameter
        //
        }else if(streq(param,"aosignal")){
            if(aonum<0){
                print_error("LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                loadfail();
            }
            if(lcm_get_value(lcm_aosignal, value, &dconf[devnum].aoch[aonum].signal)){
                print_error("LOAD: Illegal AOsignal type: %s\n",value);
                loadfail();
            }
        //
        // AOFREQUENCY
        //
        }else if(streq(param,"aofrequency")){
            if(aonum<0){
                print_error("LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                loadfail();
            // Convert the string into a float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: AOfrequency expected float, but found: %s\n", value);
                loadfail();
            }else if(ftemp<=0){
                print_error("LOAD: AOfrequency must be positive.  Found: %f\n", ftemp);
                loadfail();
            }
            dconf[devnum].aoch[aonum].frequency = ftemp;
        //
        // AOAMPLITUDE
        //
        }else if(streq(param,"aoamplitude")){
            if(aonum<0){
                print_error("LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                loadfail();
            // Convert the string into a float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: AOamplitude expected float, but found: %s\n", value);
                loadfail();
            }
            dconf[devnum].aoch[aonum].amplitude = ftemp;
        //
        // AOOFFSET
        //
        }else if(streq(param,"aooffset")){
            aonum = dconf[devnum].naoch-1;
            if(aonum<0){
                print_error("LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                loadfail();
            // Convert the string into a float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: AOoffset expected float, but found: %s\n", value);
                loadfail();
            }else if(ftemp<0. || ftemp>5.){
                print_error("LOAD: AOoffset must be between 0 and 5 volts.  Found %f.", ftemp);
                loadfail();
            }
            dconf[devnum].aoch[aonum].offset = ftemp;
        //
        // AODUTY parameter
        //
        }else if(streq(param,"aoduty")){
            aonum = dconf[devnum].naoch-1;
            if(aonum<0){
                print_error("LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                loadfail();
            // Convert the string into a float
            }else if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: AOduty expected float but found: %s\n", value);
                loadfail();
            }else if(ftemp<0. || ftemp>1.){
                print_error("LOAD: AOduty must be between 0. and 1.  Found %f.\n", ftemp);
                loadfail();
            }
            dconf[devnum].aoch[aonum].duty = ftemp;
        //
        // The AOLABEL parameter
        //
        }else if(streq(param,"aolabel")){
            aonum = dconf[devnum].naoch-1;
            if(aonum<0){
                print_error("LOAD: Cannot set analog output parameters before the first AOchannel parameter.\n");
                loadfail();
            }
            strncpy(dconf[devnum].aoch[aonum].label, value, LCONF_MAX_STR);
        //
        // TRIGCHANNEL parameter
        //
        }else if(streq(param,"trigchannel")){
            if(streq(value,"hsc")){
                itemp = LCONF_TRIG_HSC;
            }else if(sscanf(value,"%d",&itemp)!=1){
                print_error("LOAD: TRIGchannel expected an integer channel number but found: %s\n", 
                    value);
                loadfail();
            }else if(itemp<0){
                print_error("LOAD: TRIGchannel must be non-negative.\n");
                loadfail();
            }
            dconf[devnum].trigchannel = itemp;
        //
        // TRIGLEVEL parameter
        //
        }else if(streq(param,"triglevel")){
            if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: TRIGlevel expected a floating point voltage but found: %s\n", 
                    value);
                loadfail();
            }
            dconf[devnum].triglevel = ftemp;
        //
        // TRIGEDGE parameter
        //
        }else if(streq(param,"trigedge")){
            if(lcm_get_value(lcm_edge, value, &dconf[devnum].trigedge)){
                print_error("LOAD: Unrecognized TRIGedge parameter: %s\n", value);
                loadfail();
            }
        //
        // TRIGPRE parameter
        //
        }else if(streq(param,"trigpre")){
            if(sscanf(value,"%d",&itemp)!=1){
                print_error("LOAD: TRIGpre expected an integer pretrigger sample count but found: %s\n", 
                    value);
                loadfail();
            }
            if(itemp < 0){
                print_error("LOAD: TRIGpre must be non-negative.\n");
                loadfail();
            }
            dconf[devnum].trigpre = itemp;
        //
        // EFFREQUENCY parameter
        //
        }else if(streq(param,"effrequency")){
            if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: Got illegal EFfrequency: %s\n",value);
                loadfail(); 
            }else if(ftemp <= 0.){
                print_error("LOAD: EFfrequency must be positive!\n");
                loadfail();
            }
            dconf[devnum].effrequency = ftemp;
        //
        // EFCHANNEL parameter
        //
        }else if(streq(param,"efchannel")){
            efnum = dconf[devnum].nefch;
            // Check for an array overrun
            if(efnum>=LCONF_MAX_EFCH){
                print_error("LOAD: Too many EFchannel definitions.  Only %d are allowed.\n",LCONF_MAX_EFCH);
                loadfail();
            // Make sure the channel number is a valid integer
            }else if(sscanf(value,"%d",&itemp)!=1){
                print_error("LOAD: Illegal EFchannel number \"%s\". Expected integer.\n",value);
                loadfail();
            }
            // Make sure the channel number is in range for the T7.
            if(itemp<0 || itemp>LCONF_MAX_EFCH){
                print_error("LOAD: EFchannel number %d is out of range [0-%d].\n", itemp, LCONF_MAX_EFCH);
                loadfail();
            }
            // increment the number of active channels
            dconf[devnum].nefch++;
            // set the channel number
            dconf[devnum].efch[efnum].channel = itemp;
            // initialize the other parameters
            dconf[devnum].efch[efnum].signal = LC_EF_NONE;
            dconf[devnum].efch[efnum].edge = LC_EDGE_RISING;
            dconf[devnum].efch[efnum].debounce = LC_EF_DEBOUNCE_NONE;
            dconf[devnum].efch[efnum].direction = LC_EF_INPUT;
            dconf[devnum].efch[efnum].time = LCONF_DEF_EF_TIMEOUT;
            dconf[devnum].efch[efnum].phase = 0.;
            dconf[devnum].efch[efnum].duty = 0.5;
        //
        // EFSIGNAL parameter
        //
        }else if(streq(param,"efsignal")){
            if(lcm_get_value(lcm_ef_signal, value, &dconf[devnum].efch[efnum].signal)){
                print_error("LOAD: Illegal EF signal: %s\n",value);
                loadfail(); 
            }
        //
        // EFEDGE
        //
        }else if(streq(param,"efedge")){
            if(streq(value,"rising")){
                dconf[devnum].efch[efnum].edge = LC_EDGE_RISING;
            }else if(streq(value,"falling")){
                dconf[devnum].efch[efnum].edge = LC_EDGE_FALLING;
            }else if(streq(value,"all")){
                dconf[devnum].efch[efnum].edge = LC_EDGE_ANY;
            }else{
                print_error("LOAD: Illegal EF edge: %s\n",value);
                loadfail(); 
            }
        //
        // EFDEBOUNCE
        //
        }else if(streq(param,"efdebounce")){
            if(streq(value,"none")){
                dconf[devnum].efch[efnum].debounce = LC_EF_DEBOUNCE_NONE;
            }else if(streq(value,"fixed")){
                dconf[devnum].efch[efnum].debounce = LC_EF_DEBOUNCE_FIXED;
            }else if(streq(value,"restart")){
                dconf[devnum].efch[efnum].debounce = LC_EF_DEBOUNCE_RESET;
            }else if(streq(value,"minimum")){
                dconf[devnum].efch[efnum].debounce = LC_EF_DEBOUNCE_MINIMUM;
            }else{
                print_error("LOAD: Illegal EF debounce mode: %s\n",value);
                loadfail();
            }
        //
        // EFDIRECTION
        //
        }else if(streq(param,"efdirection")){
            if(streq(value,"input")){
                dconf[devnum].efch[efnum].direction = LC_EF_INPUT;
            }else if(streq(value,"output")){
                dconf[devnum].efch[efnum].direction = LC_EF_OUTPUT;
            }else{
                print_error("LOAD: Illegal EF direction: %s\n",value);
                loadfail();
            }
        //
        // EFusec
        //
        }else if(streq(param,"efusec")){
            if(sscanf(value,"%f",&ftemp)==1){
                dconf[devnum].efch[efnum].time = ftemp;
            }else{
                print_error("LOAD: Illegal EF time parameter. Found: %s\n",value);
                loadfail();
            }
        //
        // EFdegrees
        //
        }else if(streq(param,"efdegrees")){
            if(sscanf(value,"%f",&ftemp)==1){
                dconf[devnum].efch[efnum].phase = ftemp;
            }else{
                print_error("LOAD: Illegal EF phase parameter. Found: %s\n",value);
                loadfail();
            }
        //
        // EFDUTY
        //
        }else if(streq(param,"efduty")){
            if(sscanf(value,"%f",&ftemp)!=1){
                print_error("LOAD: Illegal EF duty cycle. Found: %s\n",value);
                loadfail();
            }else if(ftemp<0. || ftemp>1.){
                print_error("LOAD: Illegal EF duty cycles must be between 0 and 1. Found %f\n",ftemp);
                loadfail();
            }
            dconf[devnum].efch[efnum].duty = ftemp;
        //
        // EFLABEL
        //
        }else if(streq(param,"eflabel")){
            if(efnum<0){
                print_error("LOAD: Cannot set flexible input-output parameters before the first EFchannel parameter.\n");
                loadfail();
            }
            strncpy(dconf[devnum].efch[efnum].label, value, LCONF_MAX_STR);
        //
        // COMCHANNEL
        //
        }else if(streq(param,"comchannel")){
            comnum = dconf[devnum].ncomch;
            // Check for an overrun
            if(comnum>=LCONF_MAX_NCOMCH){
                print_error("LOAD: Too many COMchannel definitions.  Only %d are allowed.\n",LCONF_MAX_NCOMCH);
                loadfail();
            }
            if(lcm_get_value(lcm_com_channel, value, &dconf[devnum].comch[comnum].type)){
                print_error( "LOAD: Unsupported COMchannel mode: %s\n", value);
                loadfail();
            }

            // Case out the legal modes to apply the channel defaults
            // UART
            switch(dconf[devnum].comch[comnum].type){
            case LC_COM_UART:
                // Apply the UART defaults
                dconf[devnum].comch[comnum].rate = 9600.;
                dconf[devnum].comch[comnum].options.uart.bits = 8;
                dconf[devnum].comch[comnum].options.uart.parity = LC_PARITY_NONE;
                dconf[devnum].comch[comnum].options.uart.stop = 1;
                break;
            default:
                print_error( "LOAD: %s COM channels are not yet implemented.\n", 
                        lcm_get_message(lcm_com_channel, dconf[devnum].comch[comnum].type));
                loadfail();
            }
            // Increment the number of active channels
            dconf[devnum].ncomch++;
        //
        // COMLABEL
        //
        }else if(streq(param,"comlabel")){
            if(comnum<0){
                print_error("LOAD: Cannot set digital communication parameters before the first COMchannel parameter.\n");
                loadfail();
            }
            strncpy(dconf[devnum].comch[comnum].label, value, LCONF_MAX_STR);
        //
        // COMRATE
        //
        }else if(streq(param,"comrate")){
            if(comnum<0){
                print_error("LOAD: Cannot set digital communication parameters before the first COMchannel parameter.\n");
                loadfail();
            }else if(sscanf(value, "%f", &ftemp)!=1){
                print_error("LOAD: The COMRATE parameter expects a numerical data rate in bits per second.\n    Received : %s\n", value);
                loadfail();
            }
            dconf[devnum].comch[comnum].rate = ftemp;
        //
        // COMIN
        //
        }else if(streq(param,"comin")){
            if(comnum<0){
                print_error("LOAD: Cannot set digital communication parameters before the first COMchannel parameter.\n");
                loadfail();
            }else if(sscanf(value, "%d", &itemp)!=1){
                print_error("LOAD: The COMIN parameter expects an integer channel number.\n    Received : %s\n", value);
                loadfail();
            }else if(itemp < 0 || itemp > LCONF_MAX_COMCH){
                print_error("LOAD: The COMIN channel must be between 0 and %d, but was set to %d.\n", LCONF_MAX_COMCH, itemp);
                loadfail();
            }
            dconf[devnum].comch[comnum].pin_in = itemp;
        //
        // COMOUT
        //
        }else if(streq(param,"comout")){
            if(comnum<0){
                print_error("LOAD: Cannot set digital communication parameters before the first COMout parameter.\n");
                loadfail();
            }else if(sscanf(value, "%d", &itemp)!=1){
                print_error("LOAD: The COMOUT parameter expects an integer channel number.\n    Received : %s\n", value);
                loadfail();
            }else if(itemp < 0 || itemp > LCONF_MAX_COMCH){
                print_error("LOAD: The COMOUT channel must be between 0 and %d, but was set to %d.\n", LCONF_MAX_COMCH, itemp);
                loadfail();
            }
            dconf[devnum].comch[comnum].pin_out = itemp;
        //
        // COMOPTIONS
        //
        }else if(streq(param,"comoptions")){
            if(comnum<0){
                print_error("LOAD: Cannot set digital communication parameters before the first COMout parameter.\n");
                loadfail();
            }
            switch(dconf[devnum].comch[comnum].type){
            // == UART OPTIONS == //
            case LC_COM_UART:
                if(strlen(value)!=3 || sscanf(value, "%1d%c%1d", &itemp, &ctemp, &itemp2) != 3){
                    print_error( "LOAD: UART COMOPTIONS parameters must be in 8N1 (BIT PARITY STOP) notation.\n    Received: %s\n", value);
                    loadfail();
                }else if(itemp < 0 || itemp > 8){
                    print_error( "LOAD: UART COMOPTIONS requires that the bit count be from 0 to 8. Received: %d\n", itemp);
                    loadfail();
                }else if(itemp2 < 0 || itemp2 > 2){
                    print_error( "LOAD: UART COMOPTIONS requires that the stop bit count be 0, 1, or 2.  Received: %d\n", itemp2);
                    loadfail();
                }
                dconf[devnum].comch[comnum].options.uart.bits = itemp;
                dconf[devnum].comch[comnum].options.uart.stop = itemp2;

                // Modify value to form a string for lcm_get_value
                value[0] = ctemp;
                value[1] = '\0';
                if(lcm_get_value(lcm_com_parity, value, &dconf->comch[comnum].options.uart.parity)){
                    print_error( "LOAD: UART COMOPTIONS parity character must be N, E, or O.  Received: %c\n", ctemp);
                    loadfail();
                }
                break;
            // == UNHANDLED TYPE == //
            default:
                print_error( "LOAD: Unsupported COMCHANNEL value: %d\n", dconf[devnum].comch[comnum].type);
                loadfail();
            }
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
                print_error("LOAD: Illegal meta type: %s.\n",value);
                loadfail();
            }
        // META integer configuration
        }else if(strncmp(param,"int:",4)==0){
            sscanf(value,"%d",&itemp);
            lc_put_meta_int(dconf, &param[4], itemp);
        // META float configuration
        }else if(strncmp(param,"flt:",4)==0){
            sscanf(value,"%f",&ftemp);
            lc_put_meta_flt(dconf, &param[4], ftemp);
        // META string configuration
        }else if(strncmp(param,"str:",4)==0){
            lc_put_meta_str(dconf, &param[4], value);
        //
        // META values defined in a stanza
        //
        }else if(metatype == 'i'){
            sscanf(value,"%d",&itemp);
            lc_put_meta_int(dconf, param, itemp);
        }else if(metatype == 'f'){
            sscanf(value,"%f",&ftemp);
            lc_put_meta_flt(dconf, param, ftemp);
        }else if(metatype == 's'){
            lc_put_meta_str(dconf, param, value);
        // Deal gracefully with empty cases; usually EOF.
        }else if(   strncmp(param,"",LCONF_MAX_STR)==0 &&
                    strncmp(value,"",LCONF_MAX_STR)==0){
            //ignore empty strings
        // Deal with unhandled parameters
        }else{
            print_error("LOAD: Unrecognized parameter name: \"%s\"\n", param);
            loadfail();
        }
    }

    fclose(ff);
    return LCONF_NOERR;
}




void lc_write_config(lc_devconf_t* dconf, FILE* ff){
    int ainum,aonum, efnum, comnum, metanum;
    char mflt, mint, mstr;

    fprintf(ff,"# Configuration automatically generated by WRITE_CONFIG()\n");

    fprintf(ff, "connection %s\n", 
            lcm_get_config(lcm_connection, dconf->connection));
    fprintf(ff, "#connection (actual) %s\n", 
            lcm_get_config(lcm_connection, dconf->connection_act));
    fprintf(ff, "device %s\n", 
            lcm_get_config(lcm_device, dconf->device));
    fprintf(ff, "#device (actual) %s\n", 
            lcm_get_config(lcm_device, dconf->device_act));
            
    write_str(name,name);
    write_str(serial,serial);
    write_str(ip,ip);
    write_str(gateway,gateway);
    write_str(subnet,subnet);
    write_flt(samplehz,samplehz);
    write_flt(settleus,settleus);
    write_int(nsample,nsample);
    
    // Analog inputs
    if(dconf->naich)
        fprintf(ff,"\n# Analog Inputs\n");
    for(ainum=0; ainum<dconf->naich; ainum++){

        write_aiint(aichannel,channel);
        write_aistr(ailabel,label);
        write_aiint(ainegative,nchannel);
        write_aiflt(airange,range);
        write_aiint(airesolution,resolution);
        write_aiflt(aicalslope,calslope);
        write_aiflt(aicalzero,calzero);
        write_aistr(aicalunits,calunits);
        fprintf(ff,"\n");
    }
    
    // Digital Input streaming
    if(dconf->distream)
        write_int(distream,distream);

    // Analog outputs
    if(dconf->naoch)
        fprintf(ff,"# Analog Outputs\n");
    for(aonum=0; aonum<dconf->naoch; aonum++){
        write_aoint(aochannel,channel);
        write_aostr(aolabel,label);
        fprintf(ff, "aosignal %s\n",
                lcm_get_config(lcm_aosignal, dconf->aoch[aonum].signal));
        write_aoflt(aofrequency,frequency);
        write_aoflt(aoamplitude,amplitude);
        write_aoflt(aooffset,offset);
        write_aoflt(aoduty,duty);
        fprintf(ff,"\n");
    }
    // Trigger settings
    if(dconf->trigchannel >= 0){
        fprintf(ff,"# Trigger Settings\n");
        if(dconf->trigchannel == LCONF_TRIG_HSC)
            fprintf(ff,"trigchannel hsc\n");
        else
            write_int(trigchannel,trigchannel);
        write_flt(triglevel,triglevel);
        fprintf(ff, "trigedge %s\n",
                lcm_get_config(lcm_edge, dconf->trigedge));
        write_int(trigpre,trigpre);
    }

    // EF CHANNELS
    if(dconf->nefch){
        fprintf(ff,"# Flexible Input/Output\n");
        // Frequency
        write_flt(effrequency,effrequency);
    }
    for(efnum=0; efnum<dconf->nefch; efnum++){
        // Channel Number
        write_efint(efchannel,channel);
        write_efstr(eflabel,label);

        // Input/output
        fprintf(ff, "efdirection %s\n",
                lcm_get_config(lcm_ef_direction, dconf->efch[efnum].direction));
        // Signal type
        fprintf(ff, "efsignal %s\n",
                lcm_get_config(lcm_ef_signal, dconf->efch[efnum].signal));
        // Debounce
        if(dconf->efch[efnum].debounce != LC_EF_DEBOUNCE_NONE)
            fprintf(ff, "efdebounce %s\n",
                    lcm_get_config(lcm_ef_debounce, dconf->efch[efnum].debounce));
        // Edge type
        if(dconf->efch[efnum].edge)
            fprintf(ff, "efedge %s\n",
                    lcm_get_config(lcm_edge, dconf->efch[efnum].edge));
        // Timeout
        if(dconf->efch[efnum].time)
            write_efflt(efusec,time);
    }

    // COM parameters
    for(comnum=0; comnum<dconf->ncomch; comnum++){
        switch(dconf->comch[comnum].type){
        case LC_COM_UART:
            fprintf(ff, "comchannel uart\n");
            fprintf(ff, "comin %d\n", dconf->comch[comnum].pin_in);
            fprintf(ff, "comout %d\n", dconf->comch[comnum].pin_out);
            fprintf(ff, "comrate %f\n", dconf->comch[comnum].rate);
            fprintf(ff, "comoptions %d%s%d\n", 
                    dconf->comch[comnum].options.uart.bits,
                    lcm_get_config(lcm_com_parity, dconf->comch[comnum].options.uart.parity),
                    dconf->comch[comnum].options.uart.stop);
            break;
        }
        
    }

    // Write the meta parameters in stanzas
    // First, detect whether and which meta parameters there are
    mflt = 0; mint = 0; mstr = 0;
    for(metanum=0; metanum<LCONF_MAX_META && \
            dconf->meta[metanum].param[0]!='\0'; metanum++){
        if(dconf->meta[metanum].type=='i')
            mint = 1;
        else if(dconf->meta[metanum].type=='f')
            mflt = 1;
        else if(dconf->meta[metanum].type=='s')
            mstr = 1;
    }

    if(mflt || mint || mstr)
        fprintf(ff,"# Meta Parameters");

    // Write the integers
    if(mint){
        fprintf(ff,"\nmeta integer\n");
        for(metanum=0; metanum<LCONF_MAX_META && \
                dconf->meta[metanum].param[0]!='\0'; metanum++)
            if(dconf->meta[metanum].type=='i')
                fprintf(ff,"%s %d\n",
                    dconf->meta[metanum].param,
                    dconf->meta[metanum].value.ivalue);
    }

    // Write the floats
    if(mflt){
        fprintf(ff,"\nmeta float\n");
        for(metanum=0; metanum<LCONF_MAX_META && \
                dconf->meta[metanum].param[0]!='\0'; metanum++)
            if(dconf->meta[metanum].type=='f')
                fprintf(ff,"%s %f\n",
                    dconf->meta[metanum].param,
                    dconf->meta[metanum].value.fvalue);
    }

    // Write the strings
    if(mstr){
        fprintf(ff,"\nmeta string\n");
        for(metanum=0; metanum<LCONF_MAX_META && \
                dconf->meta[metanum].param[0]!='\0'; metanum++)
            if(dconf->meta[metanum].type=='s')
                fprintf(ff,"%s %s\n",
                    dconf->meta[metanum].param,
                    dconf->meta[metanum].value.svalue);
    }

    // End the stanza
    if(mflt || mint || mstr)
        fprintf(ff,"meta end\n");
    fprintf(ff,"\n## End Configuration ##\n");
}



int lc_open(lc_devconf_t* dconf){
    char *id;
    const static char any[4] = "ANY";
    char stemp[LCONF_MAX_STR];
    int err, serial, ip, dummy;

    // Device identification method precedence depends on the conneciton
    // type:
    //  USB/ANY: Serial, Name
    //  Ethernet: IP, Serial, Name
    // IP specified with USB causes IP parameters to be written
    // IP specified with ANY raises a warning and is ignored
    if(dconf->connection == LC_CON_ETH && dconf->ip[0]!='\0')
        id = dconf->ip;
    else if(dconf->serial[0]!='\0')
        id = dconf->serial;
    else if(dconf->name[0]!='\0')
        id = dconf->name;
    else
        id = (char*) any;
    //
    // WARNINGS
    //
    // If no device id was specified
    if(id==any)
        print_warning("OPEN::WARNING:: Device was requested without a device identifier.\n"\
"Results may be inconsistent; IP, SERIAL, or NAME should be set.\n");

    // If an IP address was specified with ANY connection, raise a warning
    // and clear the IP address
    if(dconf->connection == LC_CON_ANY && dconf->ip[0]!='\0'){
        print_warning( "OPEN::WARNING:: Specifying an ip address with ANY connection is ambiguous.  Ignoring.\n");
        dconf->ip[0] = '\0';
    }

    //
    // OPEN
    //
    err = LJM_Open(dconf->device, dconf->connection, \
            id, &dconf->handle);
    if(err){
        print_error( "OPEN: Failed to open the device: %s.\n", id);
        openfail();
    }
    
    //
    // DEV INFO
    //
    err = LJM_GetHandleInfo(dconf->handle, 
                            &dconf->device_act,
                            &dconf->connection_act,
                            &serial,
                            &ip,
                            &dummy,
                            &dummy);
    if(err){
        print_error( "OPEN: Failed to obtain device information.\n");
        openfail();
    }
    
    //
    // SERIAL
    //
    sprintf(stemp, "%d", serial);
    if(dconf->serial[0] == '\0'){
        strncpy(dconf->serial, stemp, LCONF_MAX_STR);
    }else if(!streq(dconf->serial, stemp)){
        print_error( 
                "OPEN: Requested serial: %s, connected to serial: %s\n"\
                "  Conflicting device identifiers!\n",\
                dconf->serial, stemp);
        openfail();
    }

    //
    // IP
    //
    if(dconf->connection != LC_CON_USB){
        LJM_NumberToIP(ip, stemp);
        if(dconf->ip[0] == '\0'){
            strncpy(dconf->ip, stemp, LCONF_MAX_STR);
        }else if(!streq(dconf->ip, stemp)){
            print_error( 
                    "OPEN: Requested ip: %s, connected to ip: %s\n"\
                    "  Conflicting device identifiers!\n",\
                    dconf->ip, stemp);
            openfail();
        }
    }
    
    //
    // NAME
    //
    err = LJM_eReadNameString(dconf->handle, \
            "DEVICE_NAME_DEFAULT", stemp);
    if(err){
        print_error( "OPEN: Failed to read the device name.\n");
        openfail();
    }
    
    if(dconf->name[0] == '\0'){
        strncpy(dconf->name, stemp, LCONF_MAX_NAME);
    }else if(!streq(dconf->name, stemp)){
        print_error( 
                "OPEN: Requested device name: \"%s\", connected to device name: \"%s\"\n"\
                "  Conflicting device identifiers!\n",
                dconf->name, stemp);
        openfail();
    }
    
    return LCONF_NOERR;
}




int lc_close(lc_devconf_t* dconf){
    int err;
    if(lc_isopen(dconf)){
        err = LJM_Close(dconf->handle);
        if(err){
            print_error("LCONFIG: Error closing device.\n");
            LJM_ErrorToString(err, err_str);
            print_error("%s\n",err_str);
        }
    }
    dconf->handle = -1;
    dconf->connection_act = -1;
    dconf->device_act = -1;
    // Clean up the buffer
    clean_buffer(&dconf->RB);
    return err;
}





int lc_upload_config(lc_devconf_t* dconf){
    // integers for interacting with the device
    int err,handle;
    // Registers for holding register addresses and memory type
    int reg_temp, type_temp;
    // Regsisters for analog input
    int ainum, naich;
    unsigned int channel;
    // Registers for analog output
    int aonum,naoch;
    // Registers for ef configuration
    int efnum,nefch;
    unsigned int ef_clk_roll, ef_clk_div;
    // Channel range registers
    int minch, maxch;
    // misc
    int samples;
    char flag;
    unsigned int index;   // counter for the stream channel index
    // exponent and generator for sine signals (real and imaginary)
    double ei,er,gi,gr;
    // Temporary/intermediate registers
    double ftemp;           // temporary floating points
    unsigned int itemp, itemp1, itemp2;
    char stemp[LCONF_MAX_STR];   // temporary string


    naich = dconf->naich;
    naoch = dconf->naoch;
    nefch = dconf->nefch;
    handle = dconf->handle;

    // Verify that the device is open
    if(!lc_isopen(dconf)){
        print_error( "UPLOAD: The device connection is not open.\n");
        return LCONF_ERROR;
    }


    // Global parameters...

    // If the requested conneciton was USB, assert the IP parameters
    if(dconf->connection == LC_CON_USB){
        flag=0;
        err = 0;
        if(dconf->ip[0] != '\0'){
            printf("IP: %s\n", dconf->ip);
            err = LJM_IPToNumber(dconf->ip, &itemp);
            err = err ? err : \
                    LJM_eWriteName(handle, "ETHERNET_IP_DEFAULT", (double) itemp);
            flag=1;
        }
        if(!err && dconf->gateway[0] != '\0'){
            printf("IP: %s\n", dconf->gateway);
            err = LJM_IPToNumber(dconf->ip, &itemp);
            err = err ? err : \
                    LJM_eWriteName(handle, "ETHERNET_GATEWAY_DEFAULT", (double) itemp);
            flag=1;
        }
        if(!err && dconf->subnet[0] != '\0'){
            printf("IP: %s\n", dconf->subnet);
            LJM_IPToNumber(dconf->ip, &itemp);
            err = err ? err : \
                    LJM_eWriteName(handle, "ETHERNET_SUBNET_DEFAULT", (double) itemp);
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
            print_error("UPLOAD: Failed to assert ethernet parameters on device with handle %d.\n",handle);
            uploadfail();
        }
    }

    // Get the max/min ai channel numbers for later
    lc_aichannels(dconf, &minch, &maxch);
    
    // Loop through the analog input channels.
    for(ainum=0;ainum<naich;ainum++){
        channel = dconf->aich[ainum].channel;
        // Perform a sanity check on the channel number
        if(channel>maxch || channel<minch){
            print_error("UPLOAD: Analog input %d points to channel %d, which is out of range [%d-%d]\n",
                    ainum, channel, minch, maxch);
            uploadfail();
        }
        /* Determine AI register addresses - depreciated - the new code relies on the LJM naming system
        airegisters(channel,&reg_negative,&reg_range,&reg_resolution);
        */

        // Write the negative channel
        stemp[0] = '\0';
        sprintf(stemp, "AIN%d_NEGATIVE_CH", channel);
        err = LJM_eWriteName( handle, stemp, 
                dconf->aich[ainum].nchannel);
        /* Depreciated - used the address instead of the register name
        err = LJM_eWriteAddress(  handle,
                            reg_negative,
                            LJM_UINT16,
                            dconf->aich[ainum].nchannel);
        */
        if(err){
            print_error("UPLOAD: Failed to update analog input %d, AI%d negative input to %d\n", 
                    ainum, channel, dconf->aich[ainum].nchannel);
            uploadfail();
        }
        // Write the range
        stemp[0] = '\0';
        sprintf(stemp, "AIN%d_RANGE", channel);
        err = LJM_eWriteName( handle, stemp, 
                dconf->aich[ainum].range);
        /* Depreciated - used address instead of register name
        err = LJM_eWriteAddress(  handle,
                            reg_range,
                            LJM_FLOAT32,
                            dconf->aich[ainum].range);
        */
        if(err){
            print_error("UPLOAD: Failed to update analog input %d, AI%d range input to %f\n", 
                    ainum, channel, dconf->aich[ainum].range);
            uploadfail();
        }
        // Write the resolution
        stemp[0] = '\0';
        sprintf(stemp, "AIN%d_RESOLUTION_INDEX", channel);
        err = LJM_eWriteName( handle, stemp, 
                dconf->aich[ainum].resolution);
        /* Depreciated - used the address instead of the register name
        err = LJM_eWriteAddress(  handle,
                            reg_resolution,
                            LJM_UINT16,
                            dconf->aich[ainum].resolution);
        */
        if(err){
            print_error("UPLOAD: Failed to update analog input %d, AI%d resolution index to %d\n", 
                    ainum, channel, dconf->aich[ainum].resolution);
            uploadfail();
        }

    }

    // Set the digital direction based on the DISTREAM register
    if(dconf->distream){
        err = LJM_eWriteName(dconf->handle,
                "DIO_INHIBIT", 0xFFFF0000);
        err = err ? err : LJM_eWriteName(dconf->handle,
                "DIO_DIRECTION", ~dconf->distream);
        err = err ? err : LJM_eWriteName(dconf->handle,
                "DIO_INHIBIT", 0x00000000);
        if(err){
            print_error( 
                    "UPLOAD: Failed to set DIO direction from DISTREAM: %d\n",
                    dconf->distream);
            uploadfail();
        }
    }

    // Set up all analog outputs
    for(aonum=0;aonum<naoch;aonum++){
        /*
        // Find the address of the corresponding channel number
        // Start with DAC0 and increment by 2 for each channel number
        LJM_NameToAddress("DAC0",&reg_temp,&type_temp);
        channel = reg_temp + 2*dconf->aoch[aonum].channel;
        */
        channel = dconf->aoch[aonum].channel;
        stemp[0] = '\0';
        sprintf(stemp, "DAC%d", channel);
        LJM_NameToAddress(stemp, &reg_temp, &type_temp);
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
            print_error("UPLOAD: Failed to disable analog output buffer STREAM_OUT%d_ENABLE\n", 
                    aonum);
            uploadfail();
        }
        // Configure the stream target to point to the DAC channel
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_TARGET", aonum);
        err = LJM_eWriteName( handle, stemp, reg_temp);
        //err = LJM_eWriteAddress(handle, reg_target, LJM_UINT32, target);
        if(err){
            print_error("UPLOAD: Failed to write target AO%d (%d), to STREAM_OUT%d_TARGET\n", 
                    channel, reg_temp, aonum);
            uploadfail();
        }
        // Configure the buffer size
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_BUFFER_SIZE", aonum);
        err = LJM_eWriteName( handle, stemp, LCONF_MAX_AOBUFFER);
        //err = LJM_eWriteAddress(handle, reg_buffersize, LJM_UINT32, LCONF_MAX_AOBUFFER);
        if(err){
            print_error("UPLOAD: Failed to write AO%d buffer size, %d, to STREAM_OUT%d_BUFFER_SIZE\n", 
                    channel, LCONF_MAX_AOBUFFER, aonum);
            uploadfail();
        }

        // Enable the output buffer
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_ENABLE", aonum);
        err = LJM_eWriteName( handle, stemp, 1);
        //err = LJM_eWriteAddress(handle, reg_enable, LJM_UINT32, 1);
        if(err){
            print_error("UPLOAD: Failed to enable analog output buffer STREAM_OUT%d_ENABLE\n", 
                    aonum);
            uploadfail();
        }

        // Construct the buffer data
        // Restrict the signal period to an integer multiple of sample periods
        if(dconf->aoch[aonum].frequency<=0.){
            print_error("UPLOAD: Analog output %d has zero or negative frequency.\n",aonum);
            uploadfail();
        }
        samples = (unsigned int)(dconf->samplehz / dconf->aoch[aonum].frequency);
        // samples * 2 will be the buffer size
        if(samples*2 > LCONF_MAX_AOBUFFER){
            print_error("UPLOAD: Analog output %d signal will require too many samples at this frequency.\n",aonum);
            uploadfail();
        }
        // Get the buffer address
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_BUFFER_F32", aonum);
        LJM_NameToAddress(stemp, &reg_temp, &type_temp);

        // Case out the different signal types
        // AO_CONSTANT
        if(dconf->aoch[aonum].signal==LC_AO_CONSTANT){
            for(index=0; index<samples; index++)
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp,\
                    dconf->aoch[aonum].offset);
        // AO_SINE
        }else if(dconf->aoch[aonum].signal==LC_AO_SINE){
            // initialize generators
            gr = 1.; gi = 0;
            ftemp = TWOPI / samples;
            er = cos( ftemp );
            ei = sin( ftemp );
            for(index=0; index<samples; index++){
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp,\
                    dconf->aoch[aonum].amplitude*gi + \
                    dconf->aoch[aonum].offset);
                // Update the complex arithmetic sine generator
                // g *= e (only complex arithmetic)
                // where e = exp(j*T*2pi*f) and g is initially 1+0i
                // will cause g to orbit the unit circle
                ftemp = er*gr - gi*ei;
                gi = gi*er + gr*ei;
                gr = ftemp;
            }
        // AO_SQUARE
        }else if(dconf->aoch[aonum].signal==LC_AO_SQUARE){
            itemp = samples*dconf->aoch[aonum].duty;
            if(itemp>samples) itemp = samples;
            // Calculate the high level
            ftemp = dconf->aoch[aonum].offset + \
                        dconf->aoch[aonum].amplitude;
            for(index=0; index<itemp; index++)
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, ftemp);
            // Calculate the low level
            ftemp = dconf->aoch[aonum].offset - \
                        dconf->aoch[aonum].amplitude;
            for(;index<samples;index++)
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, ftemp);
        // AO_TRIANGLE
        }else if(dconf->aoch[aonum].signal==LC_AO_TRIANGLE){
            itemp = samples*dconf->aoch[aonum].duty;
            if(itemp>=samples) itemp = samples-1;
            else if(itemp<=0) itemp = 1;
            // Calculate the slope (use the generator constants)
            er = 2.*dconf->aoch[aonum].amplitude/itemp;
            // Initialize the generator
            gr = dconf->aoch[aonum].offset - \
                    dconf->aoch[aonum].amplitude;
            for(index=0; index<itemp; index++){
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, gr);
                gr += er;
            }
            // Calculate the new slope
            er = 2.*dconf->aoch[aonum].amplitude/(samples-itemp);
            for(; index<samples; index++){
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, gr);
                gr -= er;
            }
        // AO_NOISE
        }else if(dconf->aoch[aonum].signal==LC_AO_NOISE){
            for(index=0; index<samples; index++){
                // generate a random number between -1 and 1
                ftemp = (2.*((double)rand())/RAND_MAX - 1.);
                ftemp *= dconf->aoch[aonum].amplitude;
                ftemp += dconf->aoch[aonum].offset;
                // Only write to the buffer if an error has not been encountered
                err = err ? err : LJM_eWriteAddress(handle, reg_temp, type_temp, ftemp);
            }
        }

        // Check for a buffer write error
        if(err){
            print_error("UPLOAD: Failed to write data to the OStream STREAM_OUT%d_BUFFER_F32\n", 
                    aonum);
            uploadfail();
        }

        // Set the loop size
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_LOOP_SIZE", aonum);
        err = LJM_eWriteName( handle, stemp, samples);
        //err = LJM_eWriteAddress(handle, reg_loopsize, LJM_UINT32, samples);
        if(err){
            print_error("UPLOAD: Failed to write loop size %d to STREAM_OUT%d_LOOP_SIZE\n", 
                    samples, aonum);
            uploadfail();
        }
        // Update loop settings
        stemp[0] = '\0';
        sprintf(stemp, "STREAM_OUT%d_SET_LOOP", aonum);
        err = LJM_eWriteName( handle, stemp, 1);
        //err = LJM_eWriteAddress(handle, reg_setloop, LJM_UINT32, 1);
        if(err){
            print_error("UPLOAD: Failed to write 1 to STREAM_OUT%d_SET_LOOP\n", 
                    aonum);
            uploadfail();
        }
    }

    // Check for an HSC trigger setting
    if(dconf->trigchannel == LCONF_TRIG_HSC){
        err = LJM_eWriteName( handle, "DIO18_EF_ENABLE", 0);
        err = err ? err : LJM_eWriteName( handle, "DIO18_EF_INDEX", 7);
        err = err ? err : LJM_eWriteName( handle, "DIO18_EF_ENABLE", 1);
        if(err){
            print_error("UPLOAD: Failed to configure high speed counter 2 for the HSC trigger.\n");
            uploadfail();
        }
    }

    //
    // Upload the EF parameters
    // First, deactivate all of the extended features
    lc_efchannels(dconf, &minch, &maxch);
    for(efnum=minch; efnum<=maxch; efnum++){
        sprintf(stemp, "DIO%d_EF_ENABLE", efnum);
        err = err ? err : LJM_eWriteName(handle, stemp, 0);
    }
    if(err){
        print_error("UPLOAD: Failed to disable DIO extended features\n");
        uploadfail();
    }
    // Determine the clock 0 rollover and clock divisor from the EFFREQUENCY parameter
    if(dconf->effrequency>0)
        ftemp = 1e6 * LCONF_CLOCK_MHZ / dconf->effrequency;
    else
        ftemp = 0xFFFFFFFF; // If EFFREQUENCY is not configured, use a default
    ef_clk_div = 1;
    // The roll value is a 32-bit integer, and the maximum divisor is 256
    while(ftemp > 0xFFFFFFFF && ef_clk_div < 256){
        ftemp /= 2;
        ef_clk_div *= 2;
    }
    // Do some sanity checking
    if(ftemp > 0xFFFFFFFF){
        print_error("UPLOAD: EF frequency was too low (%f Hz). Minimum is %f uHz.\n", 
            dconf->effrequency, 1e12 * LCONF_CLOCK_MHZ / 0xFFFFFFFF / 256);
        uploadfail();
    }else if(ftemp < 0x8){
        print_error("UPLOAD: EF frequency was too high (%f Hz).  Maximum is %f MHz.\n",
            dconf->effrequency, LCONF_CLOCK_MHZ / 0x8);
        uploadfail();
    }
    // Convert the rollover value to unsigned int
    ef_clk_roll = (unsigned int) ftemp;
    // Write the clock configuration
    // disable the clock
    err = LJM_eWriteName( handle, "DIO_EF_CLOCK0_ENABLE", 0);
    // Only configure the clock if EF channels have been configured.
    if(dconf->nefch>0){
        err = err ? err : LJM_eWriteName( handle, "DIO_EF_CLOCK0_ROLL_VALUE", ef_clk_roll);
        // set the clock divisor
        err = err ? err : LJM_eWriteName(handle, "DIO_EF_CLOCK0_DIVISOR", ef_clk_div);
        // re-enable the clock
        err = err ? err : LJM_eWriteName(handle, "DIO_EF_CLOCK0_ENABLE", 1);
        if(err){
            print_error("UPLOAD: Failed to configure EF Clock 0\n");
            uploadfail();
        }
        // If the clock setup was successful, then back-calculate the actual frequency
        // and store it in the effrequency parameter.
        dconf->effrequency = 1e6 * LCONF_CLOCK_MHZ / ef_clk_roll / ef_clk_div;
    }
    
    // Configure the EF channels
    for(efnum=0; efnum<nefch; efnum++){
        channel = dconf->efch[efnum].channel;
        switch(dconf->efch[efnum].signal){
        // Pulse-width modulation
        case LC_EF_PWM:
            // PWM input
            if(dconf->efch[efnum].direction==LC_EF_INPUT){
                // Permitted channels are 0 and 1
                if(channel > 1){
                    print_error("UPLOAD: Pulse width input only permitted on channels 0 and 1. Found ch %d.\n",
                        dconf->efch[efnum].channel);
                    uploadfail();
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
                    print_error("UPLOAD: Pulse width output only permitted on channels 0, 2, 3, 4, and 5. Found ch %d.\n",
                        dconf->efch[efnum].channel);
                    uploadfail();
                }
                // Calculate the start index
                // Unwrap the phase first (lazy method)
                while(dconf->efch[efnum].phase<0.)
                    dconf->efch[efnum].phase += 360.;
                itemp1 = ef_clk_roll * (dconf->efch[efnum].phase / 360.);
                itemp1 %= ef_clk_roll;
                // Calculate the stop index
                itemp2 = ef_clk_roll * dconf->efch[efnum].duty;
                itemp2 = (((long int) itemp1) + ((long int) itemp2))%ef_clk_roll;
                // Measurement index 1
                sprintf(stemp, "DIO%d_EF_INDEX", channel);
                err = err ? err : LJM_eWriteName( handle, stemp, 1);
                // Options - use clock 0
                sprintf(stemp, "DIO%d_EF_OPTIONS", channel);
                err = err ? err : LJM_eWriteName( handle, stemp, 0x00000000);
                if(dconf->efch[efnum].edge==LC_EDGE_FALLING){
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
        case LC_EF_COUNT:
            // counter input
            if(dconf->efch[efnum].direction == LC_EF_INPUT){
                // check for valid channels
                if(channel > 7 || channel == 4 || channel ==5){
                    print_error( "UPLOAD: EF Counter input on channel %d is not supported.\n Use EF channel 0,1,2,3,6,or 7.",
                        channel);
                    uploadfail();
                }
                switch(dconf->efch[efnum].debounce){
                    case LC_EF_DEBOUNCE_NONE:
                        // Use a counter with no debounce algorithm
                        sprintf(stemp, "DIO%d_EF_INDEX", channel);
                        err = err ? err : LJM_eWriteName(handle, stemp, 8);                            
                    break;
                    case LC_EF_DEBOUNCE_FIXED:
                        sprintf(stemp, "DIO%d_EF_INDEX", channel);
                        err = err ? err : LJM_eWriteName(handle, stemp, 9);
                        sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                        if(dconf->efch[efnum].edge == LC_EDGE_FALLING)
                            err = err ? err : LJM_eWriteName(handle, stemp, 3);
                        else if(dconf->efch[efnum].edge == LC_EDGE_RISING)
                            err = err ? err : LJM_eWriteName(handle, stemp, 4);
                        else{
                            print_error("UPLOAD: Fixed interval debounce is not supported on  'all' edges\n");
                            uploadfail();
                        }
                        // Write the timeout interval
                        sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                        err = err ? err : LJM_eWriteName(handle, stemp, 
                            dconf->efch[efnum].time);
                    break;
                    case LC_EF_DEBOUNCE_RESET:
                        sprintf(stemp, "DIO%d_EF_INDEX", channel);
                        err = err ? err : LJM_eWriteName(handle, stemp, 9);
                        sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                        if(dconf->efch[efnum].edge == LC_EDGE_FALLING)
                            err = err ? err : LJM_eWriteName(handle, stemp, 0);
                        else if(dconf->efch[efnum].edge == LC_EDGE_RISING)
                            err = err ? err : LJM_eWriteName(handle, stemp, 1);
                        else
                            err = err ? err : LJM_eWriteName(handle, stemp, 2);
                        // Write the timeout interval
                        sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                        err = err ? err : LJM_eWriteName(handle, stemp, 
                            dconf->efch[efnum].time);
                    break;
                    case LC_EF_DEBOUNCE_MINIMUM:
                        sprintf(stemp, "DIO%d_EF_INDEX", channel);
                        err = err ? err : LJM_eWriteName(handle, stemp, 9);
                        sprintf(stemp, "DIO%d_EF_CONFIG_B", channel);
                        if(dconf->efch[efnum].edge == LC_EDGE_FALLING)
                            err = err ? err : LJM_eWriteName(handle, stemp, 5);
                        else if(dconf->efch[efnum].edge == LC_EDGE_RISING)
                            err = err ? err : LJM_eWriteName(handle, stemp, 6);
                        else{
                            print_error("UPLOAD: Minimum duration debounce is not supported on  'all' edges\n");
                            uploadfail();
                        }
                        // Write the timeout interval
                        sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                        err = err ? err : LJM_eWriteName(handle, stemp, 
                            dconf->efch[efnum].time);
                    break;
                }
                // Enable the EF channel
                sprintf(stemp, "DIO%d_EF_ENABLE", channel);
                err = err ? err : LJM_eWriteName(handle, stemp, 1);
            }else{
                print_error( "UPLOAD: Count output is not currently supported.\n");
                uploadfail();
            }
        break;
        case LC_EF_FREQUENCY:
            // Check for valid channels
            if(channel > 1){
                print_error( "UPLOAD: EF Frequency on channel %d is not supported. 0 and 1 are allowed.\n",
                    channel);
                uploadfail();
            }else if(dconf->efch[efnum].direction == LC_EF_OUTPUT){
                print_error( "UPLOAD: EF Frequency output is not supported. Use PWM.\n");
                uploadfail();
            }
            if(dconf->efch[efnum].edge == LC_EDGE_FALLING){
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
        case LC_EF_PHASE:
            channel = (channel / 2) * 2;
            if(channel != 0){
                print_error( "UPLOAD: Digital phase input is only supported on channels 0 and 1.\n");
                uploadfail();
            }
            if(dconf->efch[efnum].direction == LC_EF_OUTPUT){
                print_error( "UPLOAD: Phase output is not supported\n");
                uploadfail();
            }
            // Measurement index 6
            sprintf(stemp, "DIO%d_EF_INDEX", channel);
            err = err ? err : LJM_eWriteName( handle, stemp, 6);
            sprintf(stemp, "DIO%d_EF_INDEX", channel+1);
            err = err ? err : LJM_eWriteName( handle, stemp, 6);
            if(dconf->efch[efnum].edge == LC_EDGE_RISING){
                sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                err = err ? err : LJM_eWriteName( handle, stemp, 1);
                sprintf(stemp, "DIO%d_EF_CONFIG_A", channel+1);
                err = err ? err : LJM_eWriteName( handle, stemp, 1);
            }else if(dconf->efch[efnum].edge == LC_EDGE_FALLING){
                sprintf(stemp, "DIO%d_EF_CONFIG_A", channel);
                err = err ? err : LJM_eWriteName( handle, stemp, 0);
                sprintf(stemp, "DIO%d_EF_CONFIG_A", channel+1);
                err = err ? err : LJM_eWriteName( handle, stemp, 0);
            }else{
                print_error( "UPLOAD: Phase measurements on ALL edges are not supported.\n");
                uploadfail();
            }
            // Enable the EF channels
            sprintf(stemp, "DIO%d_EF_ENABLE", channel);
            err = err ? err : LJM_eWriteName(handle, stemp, 1);
            sprintf(stemp, "DIO%d_EF_ENABLE", channel+1);
            err = err ? err : LJM_eWriteName(handle, stemp, 1);
        break;
        case LC_EF_QUADRATURE:
            channel = (channel/2) * 2;  // force channel to be even
            // Check for valid channels
            if(channel > 6 || channel == 4){
                print_error( "UPLOAD: EF Quadrature channels %d/%d not supported. 0/1, 2/3, 6/7 are allowed.\n",
                    channel, channel+1);
                uploadfail();
            }else if(dconf->efch[efnum].direction == LC_EF_OUTPUT){
                print_error( "UPLOAD: EF Quadrature output is not allowed.\n");
                uploadfail();
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
        default:
            // No extended features
        break;
        }
        if(err){
            print_error( "UPLOAD: Failed to configure EF channel %d\n.", channel);
            uploadfail();
        }
    }

    return LCONF_NOERR;
}




void lc_show_config(lc_devconf_t* dconf){
    int ainum,aonum,efnum,comnum,metanum,metacnt, itemp, ii;
    char value1[LCONF_MAX_STR];

    // download the live parameters for compairson
    //download_config(dconf, &live);

    // Connection Status
    printf(SHOW_PARAM LC_FONT_BOLD "%s%s\n" LC_FONT_NULL, 
            "Connection Status", 
            dconf->handle<0 ? lc_font_red : lc_font_green,
            lcm_get_message(lcm_connection_status, dconf->handle >= 0));
    // Connection type
    printf(SHOW_PARAM "%s (actual: " LC_FONT_BOLD "%s)\n" LC_FONT_NULL,
            "Connection Type",
            lcm_get_message(lcm_connection, dconf->connection),
            lcm_get_message(lcm_connection, dconf->connection_act));
    // Device type
    printf(SHOW_PARAM "%s (actual: " LC_FONT_BOLD "%s)\n" LC_FONT_NULL,
            "Device Type",
            lcm_get_message(lcm_device, dconf->device),
            lcm_get_message(lcm_device, dconf->device_act));
    // Device Name    
    printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL, 
            "Device Name", dconf->name);
    // Serial Number
    printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL, 
            "Serial Number", dconf->serial);
    
    // Networking
    printf(LC_FONT_YELLOW LC_FONT_BOLD "* Network Parameters *\n" LC_FONT_NULL);
    printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL, 
            "IP Address", dconf->ip);
    printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
            "Gateway", dconf->gateway);
    printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
            "Subnet", dconf->subnet);

    // Analog Inputs
    if(dconf->naich){
        printf(LC_FONT_YELLOW LC_FONT_BOLD "* Analog Input Parameters *\n" LC_FONT_NULL);
        printf(SHOW_PARAM LC_FONT_BOLD "%.1f" LC_FONT_NULL "Hz\n", 
                "Sample Rate", dconf->samplehz);
        printf(SHOW_PARAM LC_FONT_BOLD "%.0f" LC_FONT_NULL "us\n", 
                "Settling Time", dconf->settleus);
        printf(SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL, 
                "Samples", dconf->nsample);
    }
    for(ainum=0;ainum<dconf->naich;ainum++){
        printf(" -> Analog Input [" LC_FONT_BOLD "%d" LC_FONT_NULL "] (%s) <-\n",
                ainum, dconf->aich[ainum].label);
        printf( SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL, 
                "AI Channel", dconf->aich[ainum].channel);
        printf( SHOW_PARAM LC_FONT_BOLD "%d" LC_FONT_NULL " (%s)\n", 
                "AI Negative Chan",
                dconf->aich[ainum].nchannel,
                dconf->aich[ainum].nchannel==LCONF_SE_NCH ? "Single-Ended" : "Differential");
        printf( SHOW_PARAM LC_FONT_BOLD "+/-%.2f" LC_FONT_NULL "V\n", 
                "Range", dconf->aich[ainum].range);
        printf( SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL, 
                "Resolution Index", dconf->aich[ainum].resolution);
    }
    
    // Digital Input Streaming
    if(dconf->distream){
        printf(LC_FONT_YELLOW LC_FONT_BOLD "* Digital Input Stream *\n" LC_FONT_NULL);
        printf(SHOW_PARAM LC_FONT_BOLD "%0.1f" LC_FONT_NULL "Hz (Same as AI rate)\n", 
                "Sample Rate", dconf->samplehz);
        itemp = 0x00FF & dconf->distream;
        for(ii=0; ii<18; ii++){
            value1[17-ii-ii/4] = itemp & 0x0001 ? '1' : '0';
            itemp >>= 1;
        }
        value1[18] = '\0';
        printf( SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
                "DI Stream Mask", value1);
    }
    
    // Trigger
    if(dconf->trigchannel >= 0){
        printf(LC_FONT_YELLOW LC_FONT_BOLD "* Trigger Settings *\n" LC_FONT_NULL);
        printf(SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL,
                "Channel", dconf->trigchannel);
        printf(SHOW_PARAM LC_FONT_BOLD "%0.3f" LC_FONT_NULL "V\n",
                "Threshold", dconf->triglevel);
        printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
                "Edge", lcm_get_message(lcm_edge, dconf->trigedge));
        printf(SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL,
                "Pre-Trigger", dconf->trigpre);
    }
    
    // Analog Outputs
    if(dconf->naoch){
        printf(LC_FONT_YELLOW LC_FONT_BOLD "* Analog Outputs *\n" LC_FONT_NULL);
        printf(SHOW_PARAM LC_FONT_BOLD "%.1f" LC_FONT_NULL "Hz (Same as AI rate)\n", 
                "Sample Rate", dconf->samplehz);
    }
    for(aonum=0; aonum<dconf->naoch; aonum++){
        printf(" -> Analog Output [" LC_FONT_BOLD "%d" LC_FONT_NULL "] (%s) <-\n",
                aonum, dconf->aoch[aonum].label);
        printf(SHOW_PARAM LC_FONT_BOLD "%d" LC_FONT_NULL "\n", 
                "AO Channel", dconf->aoch[aonum].channel);
        printf(SHOW_PARAM LC_FONT_BOLD "%s" LC_FONT_NULL "\n",
                "Signal", lcm_get_message(lcm_aosignal,dconf->aoch[aonum].signal));
        printf(SHOW_PARAM LC_FONT_BOLD "%0.3f" LC_FONT_NULL "V\n", 
                "Amplitude", dconf->aoch[aonum].amplitude);
        printf(SHOW_PARAM LC_FONT_BOLD "%0.3f" LC_FONT_NULL "V\n", 
                "Offset", dconf->aoch[aonum].offset);
        printf(SHOW_PARAM LC_FONT_BOLD "%0.1f" LC_FONT_NULL "Hz\n", 
                "Frequency", dconf->aoch[aonum].frequency);
        printf(SHOW_PARAM LC_FONT_BOLD "%0.3f" LC_FONT_NULL "%%\n", 
                "Duty Cycle", dconf->aoch[aonum].duty);
    }
    
    // COM settings
    if(dconf->ncomch)
        printf(LC_FONT_YELLOW LC_FONT_BOLD "* COM settings *\n" LC_FONT_NULL);
    for(comnum=0; comnum<dconf->ncomch; comnum++){
        printf(" -> COM Channel [" LC_FONT_BOLD "%d" LC_FONT_NULL "] (%s) <-\n",
                comnum, dconf->comch[comnum].label);
        printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
                "Channel Type", lcm_get_message(lcm_com_channel, dconf->comch[comnum].type));
        switch(dconf->comch[comnum].type){
        case LC_COM_UART:
            printf(SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL,
                    "Rx DIO Pin", dconf->comch[comnum].pin_in);
            printf(SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL,
                    "Tx DIO Pin", dconf->comch[comnum].pin_out);
            printf(SHOW_PARAM LC_FONT_BOLD "%.0f\n" LC_FONT_NULL,
                    "Baud", dconf->comch[comnum].rate);
            printf(SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL,
                    "Data Bits", dconf->comch[comnum].options.uart.bits);
            printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
                    "Parity", lcm_get_message(lcm_com_parity, dconf->comch[comnum].options.uart.parity));
            printf(SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL,
                    "Stop Bits", dconf->comch[comnum].options.uart.stop);                    
            break;
        default:
            printf(LC_FONT_RED LC_FONT_BOLD "    CHANNEL TYPE NOT SUPPORTED\n" LC_FONT_NULL);
        }
    }
    
    // EF Settings
    if(dconf->nefch){
        printf(LC_FONT_YELLOW LC_FONT_BOLD "* EF settings *\n" LC_FONT_NULL);
        printf(SHOW_PARAM LC_FONT_BOLD "%.1f" LC_FONT_NULL "Hz  (Applied to all EF channels)\n",
                "EF Frequency", dconf->effrequency);
    }
    for(efnum=0; efnum<dconf->nefch; efnum++){
        printf(" -> Extended Feature [" LC_FONT_BOLD "%d" LC_FONT_NULL "] (%s)\n",
                efnum, dconf->efch[efnum].label);
        printf(SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL,
                "EF Channel", dconf->efch[efnum].channel);
        printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
                "Direction", lcm_get_message(lcm_ef_direction, dconf->efch[efnum].direction));
        printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
                "Signal Type", lcm_get_message(lcm_ef_signal, dconf->efch[efnum].signal));
        printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
                "Signal Type", lcm_get_message(lcm_ef_signal, dconf->efch[efnum].signal));
    }

    // count the meta parameters
    for(metacnt=0;\
            metacnt<LCONF_MAX_META && \
            dconf->meta[metacnt].param[0]!='\0';\
            metacnt++){}
    if(metacnt)
        printf(LC_FONT_YELLOW LC_FONT_BOLD "\nMeta Parameters: %d\n" LC_FONT_NULL,metacnt);
    for(metanum=0; metanum<metacnt; metanum++)
        switch(dconf->meta[metanum].type){
            case 'i':
                printf(SHOW_PARAM LC_FONT_BOLD "%d\n" LC_FONT_NULL,
                    dconf->meta[metanum].param,
                    dconf->meta[metanum].value.ivalue);
            break;
            case 'f':
                printf(SHOW_PARAM LC_FONT_BOLD "%f\n" LC_FONT_NULL,
                    dconf->meta[metanum].param,
                    dconf->meta[metanum].value.fvalue);
            break;
            case 's':
                printf(SHOW_PARAM LC_FONT_BOLD "%s\n" LC_FONT_NULL,
                    dconf->meta[metanum].param,
                    dconf->meta[metanum].value.svalue);
            break;
            default:
                printf(SHOW_PARAM LC_FONT_RED LC_FONT_BOLD "%s\n" LC_FONT_NULL,
                    dconf->meta[metanum].param,
                    "*CORRUPT*");
        }
}



int lc_update_ef(lc_devconf_t* dconf){
    int handle, channel, efnum, itemp1, itemp2;
    double ftemp, ftemp1, ftemp2;
    char stemp[LCONF_MAX_STR];
    unsigned int ef_clk_roll, ef_clk_div;
    int err = 0;

    handle = dconf->handle;

    // Get the roll value
    err = err ? err : LJM_eReadName(handle, "DIO_EF_CLOCK0_ROLL_VALUE", &ftemp);
    ef_clk_roll = (unsigned int) ftemp;
    // Get the divisor
    err = err ? err : LJM_eReadName(handle, "DIO_EF_CLOCK0_DIVISOR", &ftemp);
    ef_clk_div = (unsigned int) ftemp;
    
    // Configure the EF channels
    for(efnum=0; efnum<dconf->nefch; efnum++){
        channel = dconf->efch[efnum].channel;
        switch(dconf->efch[efnum].signal){
        // Pulse-width modulation
        case LC_EF_PWM:
            // PWM input
            if(dconf->efch[efnum].direction==LC_EF_INPUT){
                // Get the time high
                sprintf(stemp, "DIO%d_EF_READ_A", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
                // Get the time low
                sprintf(stemp, "DIO%d_EF_READ_B", channel);
                err = err ? err : LJM_eReadName( handle, stemp, &ftemp1);
                ftemp2 = ftemp + ftemp1;

                dconf->efch[efnum].counts = (unsigned int) ftemp2;
                dconf->efch[efnum].time = ftemp2 * ef_clk_div / LCONF_CLOCK_MHZ;
                dconf->efch[efnum].phase = 0.;
                if(dconf->efch[efnum].edge==LC_EDGE_FALLING){
                    dconf->efch[efnum].duty = ftemp1 / ftemp2;
                }else{
                    dconf->efch[efnum].duty = ftemp / ftemp2;
                }

            // PWM output
            }else{
                // Calculate the start index
                // Unwrap the phase first (lazy method)
                while(dconf->efch[efnum].phase<0.)
                    dconf->efch[efnum].phase += 360.;
                itemp1 = ef_clk_roll * (dconf->efch[efnum].phase / 360.);
                itemp1 %= ef_clk_roll;
                // Calculate the stop index
                itemp2 = ef_clk_roll * dconf->efch[efnum].duty;
                itemp2 = (((long int) itemp1) + ((long int) itemp2))%ef_clk_roll;
                if(dconf->efch[efnum].edge==LC_EDGE_FALLING){
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
        case LC_EF_COUNT:
            sprintf(stemp, "DIO%d_EF_READ_A", channel);
            err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
            dconf->efch[efnum].counts = (unsigned int) ftemp;
            dconf->efch[efnum].time = 0.;
            dconf->efch[efnum].duty = 0.;
            dconf->efch[efnum].phase = 0.;
        break;
        case LC_EF_FREQUENCY:
            sprintf(stemp, "DIO%d_EF_READ_A", channel);
            err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
            dconf->efch[efnum].counts = (unsigned int) ftemp;
            dconf->efch[efnum].time = ftemp * ef_clk_div / LCONF_CLOCK_MHZ;
            dconf->efch[efnum].duty = 0.;
            dconf->efch[efnum].phase = 0.;
        break;
        case LC_EF_PHASE:
            sprintf(stemp, "DIO%d_EF_READ_A", channel);
            err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
            dconf->efch[efnum].counts = (unsigned int) ftemp;
            dconf->efch[efnum].time = ftemp * ef_clk_div / LCONF_CLOCK_MHZ;
            dconf->efch[efnum].duty = 0.;
            dconf->efch[efnum].phase = 0.;
        break;
        case LC_EF_QUADRATURE:
            sprintf(stemp, "DIO%d_EF_READ_A", channel);
            err = err ? err : LJM_eReadName( handle, stemp, &ftemp);
            dconf->efch[efnum].counts = (unsigned int) ftemp;
            dconf->efch[efnum].time = 0;
            dconf->efch[efnum].duty = 0.;
            dconf->efch[efnum].phase = 0.;
        break;
        default:
            // No AOsignal
        break;
        }
        if(err){
            print_error( "EF_UPDATE: Failed while working on EF channel %d\n.", channel);
            break;
        }
    }

    if(err){
        print_error("LCONFIG: Error updating Flexible IO parameters.\n");
        LJM_ErrorToString(err, err_str);
        print_error("%s\n",err_str);
        return LCONF_ERROR;
    }
    return LCONF_NOERR;
}


int lc_communicate(lc_devconf_t* dconf, 
        const unsigned int comchannel,
        const char *txbuffer, const unsigned int txlength, 
        char *rxbuffer, const unsigned int rxlength,
        const int timeout_ms){
    
    if(lc_com_start(dconf, comchannel, rxlength)){
        print_error("COMMUNICATE: Failed to start.\n");
        return LCONF_ERROR;
    }
    if(txlength && lc_com_write(dconf, comchannel, txbuffer, txlength)){
        print_error("COMMUNICATE: Failure durring transmission.\n");
        lc_com_stop(dconf, comchannel);
        return LCONF_ERROR;
    }
    if(rxlength &&
            0 > lc_com_read(dconf, comchannel, rxbuffer, rxlength, timeout_ms)){
        print_error("COMMUNICATE: Error while listening\n");
        lc_com_stop(dconf,comchannel);
        return LCONF_ERROR;
    }
    lc_com_stop(dconf, comchannel);
    return LCONF_NOERR;
}


int lc_com_start(lc_devconf_t* dconf, const unsigned int comchannel,
        const unsigned int rxlength){

    lc_comconf_t *com;
    int err;
    
    // Verify that the channel number is legal
    if(comchannel > LCONF_MAX_NCOMCH){
        print_error( "COM_START: Com channel, %d, is out of range, [0-%d]\n", comchannel, LCONF_MAX_COMCH);
        return LCONF_ERROR;
    }
    // Point to the selected com struct
    com = &dconf->comch[comchannel];
    // Case out the different channel types, and start configuring!
    switch(com->type){
    //
    // UART
    //
    case LC_COM_UART:
        // Test all mandatory parameters
        if(com->pin_in < 0 || com->pin_out < 0 || com->rate < 0){
            print_error( "COM_START: A UART channel requires that COMIN, COMOUT, and COMRATE be configured\n");
            return LCONF_ERROR;
        }
        
        // Start configuring
        err = LJM_eWriteName(dconf->handle, "ASYNCH_ENABLE", 0);
        err = err ? err : LJM_eWriteName(dconf->handle, "ASYNCH_TX_DIONUM", (double) com->pin_out);
        err = err ? err : LJM_eWriteName(dconf->handle, "ASYNCH_RX_DIONUM", (double) com->pin_in);
        err = err ? err : LJM_eWriteName(dconf->handle, "ASYNCH_BAUD", (double) com->rate);
        err = err ? err : LJM_eWriteName(dconf->handle, "ASYNCH_NUM_DATA_BITS", (double) com->options.uart.bits);
        err = err ? err : LJM_eWriteName(dconf->handle, "ASYNCH_NUM_STOP_BITS", (double) com->options.uart.stop);
        err = err ? err : LJM_eWriteName(dconf->handle, "ASYNCH_PARITY", (double) com->options.uart.parity);
        err = err ? err : LJM_eWriteName(dconf->handle, "ASYNCH_RX_BUFFER_SIZE_BYTES", (double) rxlength);
        err = err ? err : LJM_eWriteName(dconf->handle, "ASYNCH_ENABLE", 1);
        if(err){
            LJM_ErrorToString(err, err_str);
            print_error( "COM_START: There was an error configuring the UART interface.\n%s\n", err_str);
            return LCONF_ERROR;
        }
        return LCONF_NOERR;
    default:
        print_error( "COM_START: Channel %d interface, %s, is not supported.\n", 
                comchannel, lcm_get_message(lcm_com_channel, com->type));
        return LCONF_ERROR;
    }
    
    print_error("COM_START: This error should never appear!\n  Bad things have happened!\n");
    return LCONF_ERROR;
}

 
int lc_com_stop(lc_devconf_t* dconf, const unsigned int comchannel){
    int err;
    err = LJM_eWriteName(dconf->handle, "ASYNCH_ENABLE" , 0);
    if(err){
        LJM_ErrorToString(err, err_str);
        print_error( "COM_STOP: There was an error disabling the COM interface.\n%s\n", err_str);
        return LCONF_ERROR;
    }
    return LCONF_NOERR;
}


int lc_com_write(lc_devconf_t* dconf, const unsigned int comchannel,
        const char *txbuffer, const unsigned int txlength){
    int txbytes, err, err2;
    lc_comconf_t *com;
    
    txbytes = txlength >> 1;
    
    // Verify that the channel number is legal
    if(comchannel > LCONF_MAX_NCOMCH){
        print_error( "COM_READ: Com channel, %d, is out of range, [0-%d]\n", comchannel, LCONF_MAX_COMCH);
        return LCONF_ERROR;
    }else if(!txlength){
        return LCONF_NOERR;
    }
    
    com = &dconf->comch[comchannel];
    
    switch(com->type){
    case LC_COM_UART:
        // The UART interface uses 16-bit-wide buffers, so the number of bytes
        // transmitted will be half the buffer length in bytes.
        txbytes = txlength >> 1;
        // If configured to transmit, now's the time.
        err = LJM_eWriteName(dconf->handle, "ASYNCH_NUM_BYTES_TX", (double) txbytes);
        err = err ? err : LJM_eWriteNameByteArray(dconf->handle, 
                "ASYNCH_DATA_TX", txlength, txbuffer, &err2);
        err = err ? err : LJM_eWriteName(dconf->handle, "ASYNCH_TX_GO", 1);
        if(err){
            LJM_ErrorToString(err, err_str);
            print_error( "COM_WRITE: Failed while trying to transmit data over UART\n%s\n", err_str);
            return LCONF_ERROR;
        }
        break;
    default:
        print_error( "COM_WRITE: Channel %d interface, %s, is not supported.\n", 
                comchannel, lcm_get_message(lcm_com_channel, com->type));
        return LCONF_ERROR;
    }
    return LCONF_NOERR;
}


int lc_com_read(lc_devconf_t* dconf, const unsigned int comchannel,
        char *rxbuffer, const unsigned int rxlength, int timeout_ms){
    int rxbytes, waitingbytes, err, err2;
    struct timeval start, now;
    double ftemp;
    lc_comconf_t *com;
    
    rxbytes = rxlength >> 1;
    
    // Verify that the channel number is legal
    if(comchannel > LCONF_MAX_NCOMCH){
        print_error( "COM_READ: Com channel, %d, is out of range, [0-%d]\n", comchannel, LCONF_MAX_COMCH);
        return LCONF_ERROR;
    }else if(!rxlength){
        return 0;
    }
    
    com = &dconf->comch[comchannel];
    
    switch(com->type){
    case LC_COM_UART:
        // The LJM UART uses a 16-bit-wide buffer, so the bytes transmitted will
        // be half the buffer size.
        rxbytes = rxlength >> 1;
        // Mark the time
        gettimeofday(&start, NULL);
        // If RX is configured, loop until a timeout or the data arrive
        while(rxlength){
            // Check on the number of bytes received
            err = LJM_eReadName(dconf->handle, "ASYNCH_NUM_BYTES_RX", &ftemp);
            waitingbytes = (int) ftemp;
            if(err){
                LJM_ErrorToString(err, err_str);
                print_error( "COM_READ: Failed while checking on the bytes waiting\n%s\n", err_str);
                return LCONF_ERROR;
            }
            if(waitingbytes >= rxbytes || timeout_ms < 0){
                err = LJM_eReadNameByteArray(dconf->handle, 
                        "ASYNCH_DATA_RX", rxlength, rxbuffer, &err2);
                if(err){
                    LJM_ErrorToString(err, err_str);
                    print_error( "COM_READ: Failed to read the RX buffer\n%s\n", err_str);
                    return LCONF_ERROR;
                }
                return (waitingbytes > rxbytes ? rxbytes : waitingbytes)<<1;
            }
            // Check for a timeout
            gettimeofday(&now,NULL); 
            if(timeout_ms > 0 && difftime_ms(&now,&start) > timeout_ms){
                print_error( "COM_READ: RX operation timed out (over %dms)\n", timeout_ms);
                return LCONF_ERROR;
            }
            // Wait a 10ms to check again
            usleep(10000);
        }
        break;
    default:
        print_error( "COM_READ: Channel %d interface, %s, is not supported.\n", 
                comchannel, lcm_get_message(lcm_com_channel, com->type));
        return 0;
    }
    return 0;
}


int lc_get_meta_int(lc_devconf_t* dconf, const char* param, int* value){
    int metanum;
    // Scan through the meta list
    // stop if end-of-list or empty entry
    for(metanum=0;
        metanum<LCONF_MAX_META && \
        dconf->meta[metanum].param[0] != '\0';
        metanum++)
        // If the parameter matches, return the value
        if(strncmp(dconf->meta[metanum].param,param,LCONF_MAX_STR)==0){
            // check that the type is correct
            if(dconf->meta[metanum].type!='i')
                print_warning("GET_META::WARNING:: param %s holds type '%c', but was queried for an integer.\n",\
                    param,dconf->meta[metanum].type);
            *value = dconf->meta[metanum].value.ivalue;
            return LCONF_NOERR;
        }
    print_error("GET_META: Failed to find meta parameter, %s\n",param);
    return LCONF_NOERR;
}


int lc_get_meta_flt(lc_devconf_t* dconf, const char* param, double* value){
    int metanum;
    // Scan through the meta list
    // stop if end-of-list or empty entry
    for(metanum=0;
        metanum<LCONF_MAX_META && \
        dconf->meta[metanum].param[0] != '\0';
        metanum++)
        // If the parameter matches, return the value
        if(strncmp(dconf->meta[metanum].param,param,LCONF_MAX_STR)==0){
            // check that the type is correct
            if(dconf->meta[metanum].type!='f')
                print_warning("GET_META::WARNING:: param %s holds type '%c', but was queried for an float.\n",\
                    param,dconf->meta[metanum].type);
            *value = dconf->meta[metanum].value.fvalue;
            return LCONF_NOERR;
        }
    print_error("GET_META: Failed to find meta parameter, %s\n",param);
    return LCONF_NOERR;
}


int lc_get_meta_str(lc_devconf_t* dconf, const char* param, char* value){
    int metanum;
    // Scan through the meta list
    // stop if end-of-list or empty entry
    for(metanum=0;
        metanum<LCONF_MAX_META && \
        dconf->meta[metanum].param[0] != '\0';
        metanum++)
        // If the parameter matches, return the value
        if(strncmp(dconf->meta[metanum].param,param,LCONF_MAX_STR)==0){
            // check that the type is correct
            if(dconf->meta[metanum].type!='s')
                print_warning("GET_META::WARNING:: param %s holds type '%c', but was queried for an string.\n",\
                    param,dconf->meta[metanum].type);
            strncpy(value, dconf->meta[metanum].value.svalue, LCONF_MAX_STR);
            return LCONF_NOERR;
        }
    print_error("GET_META: Failed to find meta parameter, %s\n",param);
    return LCONF_ERROR;
}


int lc_put_meta_int(lc_devconf_t* dconf, const char* param, int value){
    int metanum;
    for(metanum=0; metanum<LCONF_MAX_META; metanum++){
        if(dconf->meta[metanum].param[0] == '\0'){
            strncpy(dconf->meta[metanum].param, param, LCONF_MAX_STR);
            dconf->meta[metanum].value.ivalue = value;
            dconf->meta[metanum].type = 'i';
            return LCONF_NOERR;
        }else if(strncmp(dconf->meta[metanum].param, param, LCONF_MAX_STR)==0){
            dconf->meta[metanum].value.ivalue = value;
            dconf->meta[metanum].type = 'i';
            return LCONF_NOERR;
        }
    }
    print_error("PUT_META: Meta array is full. Failed to place parameter, %s\n",param);
    return LCONF_ERROR;
}


int lc_put_meta_flt(lc_devconf_t* dconf, const char* param, double value){
    int metanum;
    for(metanum=0; metanum<LCONF_MAX_META; metanum++){
        if(dconf->meta[metanum].param[0] == '\0'){
            strncpy(dconf->meta[metanum].param, param, LCONF_MAX_STR);
            dconf->meta[metanum].value.fvalue = value;
            dconf->meta[metanum].type = 'f';
            return LCONF_NOERR;
        }else if(strncmp(dconf->meta[metanum].param, param, LCONF_MAX_STR)==0){
            dconf->meta[metanum].value.fvalue = value;
            dconf->meta[metanum].type = 'f';
            return LCONF_NOERR;
        }
    }
    print_error("PUT_META: Meta array is full. Failed to place parameter, %s\n",param);
    return LCONF_ERROR;
}


int lc_put_meta_str(lc_devconf_t* dconf, const char* param, char* value){
    int metanum;
    for(metanum=0; metanum<LCONF_MAX_META; metanum++){
        if(dconf->meta[metanum].param[0] == '\0'){
            strncpy(dconf->meta[metanum].param, param, LCONF_MAX_STR);
            strncpy(dconf->meta[metanum].value.svalue, value, LCONF_MAX_STR);
            dconf->meta[metanum].type = 's';
            return LCONF_NOERR;
        }else if(strncmp(dconf->meta[metanum].param, param, LCONF_MAX_STR)==0){
            strncpy(dconf->meta[metanum].value.svalue, value, LCONF_MAX_STR);
            dconf->meta[metanum].type = 's';
            return LCONF_NOERR;
        }
    }    
    print_error("PUT_META: Meta array is full. Failed to place parameter, %s\n",param);
    return LCONF_ERROR;
}


void lc_stream_status(lc_devconf_t* dconf,
        unsigned int *samples_streamed, unsigned int *samples_read,
        unsigned int *samples_waiting){

    if(dconf->RB.buffer){
        *samples_streamed = dconf->RB.samples_streamed;
        *samples_read = dconf->RB.samples_read;
        // Case out the read and write status
        if(dconf->RB.read == dconf->RB.size_samples)
            *samples_waiting = dconf->RB.size_samples / dconf->RB.channels;
        // If read has wrapped around the end of the buffer
        else if(dconf->RB.write < dconf->RB.read)
            *samples_waiting = (dconf->RB.size_samples -\
                dconf->RB.read + dconf->RB.write)/\
                dconf->RB.channels;
        else
            *samples_waiting = (dconf->RB.write-dconf->RB.read)/\
                dconf->RB.channels;
    }
}



int lc_stream_iscomplete(lc_devconf_t* dconf){
    return (dconf->RB.samples_streamed > dconf->nsample);
}


int lc_stream_isempty(lc_devconf_t* dconf){
    return isempty_buffer(&dconf->RB);
}


int lc_stream_start(lc_devconf_t* dconf, int samples_per_read){
    int ainum,aonum,index,err=0;
    int stlist[LCONF_MAX_STCH];
    int resindex, reg_temp, dummy;
    unsigned int blocks;
    char flag;

    // If the application specifies samples, it overrides the configuration file.
    if(samples_per_read <= 0)
        samples_per_read = LCONF_SAMPLES_PER_READ;

    if(dconf->naich<1 && dconf->naoch<1 && dconf->distream==0){
        print_error("START_DATA_STREAM: No channels are configured\n");
        startfail();
    }

    // Initialize the resolution index
    resindex = dconf->aich[0].resolution;
    flag = 1;   // use the flag to remember if we have already issued a warning.
    index = 0;  // Index is a parallel count for the location in stlist.
    // Assemble the list of channels and check the resolution settings
    for(ainum=0; ainum<dconf->naich; ainum++){
        // Populate the channel list
        stlist[index++] = dconf->aich[ainum].channel * 2;
        // Do we have a conflict in resindex values?
        if(flag && resindex != dconf->aich[ainum].resolution){
            print_warning(
                    "STREAM_START::WARNING:: AICH%d has different resolution %d.\n",
                    ainum, resindex);
            flag=0;
        }
        // If this channel's resolution index is less than those previous
        if(resindex > dconf->aich[ainum].resolution)
            resindex = dconf->aich[ainum].resolution;
    }

    if(!flag)
        print_warning("STREAM_START::WARNING:: Used resolution index %d for all streaming channels.\n",resindex);

    // Write the configuration parameters unique to streaming.
    err=LJM_eWriteName(dconf->handle, 
            "STREAM_RESOLUTION_INDEX", resindex);
    // Write the settling time
    err= err ? err : LJM_eWriteName(dconf->handle, 
            "STREAM_SETTLING_US", dconf->settleus);
    if(err){
        print_error("STREAM_START: Failed to set setting and resolution settings.\n");
        startfail();
    }

    // Add digital input streaming?
    if(dconf->distream)
        LJM_NameToAddress("FIO_EIO_STATE", &stlist[index++], &dummy);

    // Add any analog output streams to the stream scanlist
    // This approach collects analog input BEFORE updating the outputs.
    // This allows one scan cycle for system settling before the new 
    // measurements are taken.  On the other hand, it also imposes a 
    // scan cycle's worth of phase between output and input.
    
    // Get the STREAM_OUT0 address
    LJM_NameToAddress("STREAM_OUT0",&reg_temp,&dummy);
    for(aonum=0;aonum<dconf->naoch;aonum++){
        stlist[index++] = reg_temp;
        // increment the index and the register
        reg_temp += 1;
    }

    // Determine the number of R/W blocks in the buffer
    blocks = dconf->trigpre > dconf->nsample ? 
                dconf->trigpre : dconf->nsample;
    blocks = (blocks/samples_per_read) + 1;

    // Configure the ring buffer
    // The number of buffer R/W blocks is calculated from the pretrigger
    // buffer size plus 1.
    if(init_buffer(&dconf->RB, 
                lc_nistream(dconf),
                samples_per_read,
                blocks)){
        print_error("STREAM_START: Failed to initialize the buffer.\n");
        return LCONF_ERROR;
    }

    // Initialize the trigger
    dconf->trigmem = 0;
    if(dconf->trigchannel >= 0)
        dconf->trigstate = LC_TRIG_PRE;

    // Start the stream.
    err=LJM_eStreamStart(dconf->handle, 
            samples_per_read,
            index,
            stlist,
            &dconf->samplehz);

    if(err){
        print_error("STREAM_START: Failed to start the stream.\n");
        startfail();
    }
    return LCONF_NOERR;
}


int lc_stream_service(lc_devconf_t* dconf){
    int dev_backlog, ljm_backlog, size, err;
    int index, this;
    double ftemp;
    double *write_data;

    // Retrieve the write buffer pointer
    write_data = get_write_buffer(&dconf->RB);
    // Perform the data transfer
    err = LJM_eStreamRead(dconf->handle, 
            write_data,
            &dev_backlog, &ljm_backlog);
    // Do some error checking
    if(dev_backlog > LCONF_BACKLOG_THRESHOLD)
        print_error("READ_STREAM: Device backlog! %d\n",dev_backlog);
    else if(ljm_backlog > LCONF_BACKLOG_THRESHOLD)
        print_error("READ_STREAM: Software backlog! %d\n",ljm_backlog);
    
    if(err){
        print_error("LCONFIG: Error reading from data stream.\n");
        LJM_ErrorToString(err, err_str);
        print_error("%s\n",err_str);
        return LCONF_ERROR;
    }
    // If there was no error, update the ringbuffer registers
    service_write_buffer(&dconf->RB);

    // Tend to the software trigger
    if(dconf->trigstate == LC_TRIG_PRE){
        if(dconf->RB.samples_streamed >= dconf->trigpre){
            dconf->trigstate = LC_TRIG_ARMED;
            if(dconf->trigchannel == LCONF_TRIG_HSC){
                err = LJM_eReadName(dconf->handle, "DIO18_EF_READ_A", &ftemp);
                if(err){
                    print_error("SERVICE DATA STREAM: Failed to test the high speed counter.\n");
                    dconf->trigmem = 0x00;
                    return LCONF_ERROR;
                }
                dconf->trigmem = (unsigned int) ftemp;
            }
        }
    }else if(dconf->trigstate == LC_TRIG_ARMED){
        // Test for a trigger event
        size = dconf->RB.blocksize_samples;
        // Case out the edge types in advance for speed
        // test for a counter event
        if(dconf->trigchannel == LCONF_TRIG_HSC){
            err = LJM_eReadName(dconf->handle, "DIO18_EF_READ_A", &ftemp);
            if(err){
                print_error("SERVICE DATA STREAM: Failed to test the high speed counter.\n");
                dconf->trigmem = 0x00;
                return LCONF_ERROR;
            }else if(ftemp > dconf->trigmem){
                dconf->trigstate = LC_TRIG_ACTIVE;
                dconf->trigmem = 0x00;
            }
        // Test for a rising edge
        }else if(dconf->trigedge == LC_EDGE_RISING){
            for(index = dconf->trigchannel;
                    index < size; index += dconf->RB.channels){
                if(write_data[index] > dconf->triglevel)
                    this = 0b01;
                else
                    this = 0b10;
                if(dconf->trigmem == this){
                    dconf->trigstate = LC_TRIG_ACTIVE;
                    dconf->trigmem = 0x00;
                    break;
                }else
                    dconf->trigmem = this>>1;
            }
        // Falling edge
        }else if(dconf->trigedge == LC_EDGE_FALLING){
            for(index = dconf->trigchannel;
                    index < size; index += dconf->RB.channels){
                if(write_data[index] <= dconf->triglevel)
                    this = 0b01;
                else
                    this = 0b10;
                if(dconf->trigmem == this){
                    dconf->trigstate = LC_TRIG_ACTIVE;
                    dconf->trigmem = 0x00;
                    break;
                }else
                    dconf->trigmem = this>>1;
            }
        // Any edge
        }else{
            for(index = dconf->trigchannel;
                    index < size; index += dconf->RB.channels){
                if(write_data[index] > dconf->triglevel)
                    this = 0b01;
                else
                    this = ~((unsigned int)0b01);
                if(dconf->trigmem == this){
                    dconf->trigstate = LC_TRIG_ACTIVE;
                    dconf->trigmem = 0x00;
                    break;
                }else
                    dconf->trigmem = ~this;
            }
        }
        // Unless a trigger was detected, throw away a sample block and reduce
        // the record of samples streamed
        if(dconf->trigstate == LC_TRIG_ARMED){
            service_read_buffer(&dconf->RB);
            dconf->RB.samples_streamed -= dconf->RB.samples_per_read;
        }
    }
    return LCONF_NOERR;
}


int lc_stream_read(lc_devconf_t* dconf,
    double **data, unsigned int *channels, unsigned int *samples_per_read){
    
    (*data) = get_read_buffer(&dconf->RB);
    service_read_buffer(&dconf->RB);
    (*channels) = dconf->RB.channels;
    (*samples_per_read) = dconf->RB.samples_per_read;
    if(*data)
        return LCONF_NOERR;
    return LCONF_ERROR;
}


int lc_stream_stop(lc_devconf_t* dconf){
    int err;
    err = LJM_eStreamStop(dconf->handle);
    
    //clean_buffer(&dconf->RB);
    
    if(err){
        print_error("LCONFIG: Error stopping a data stream.\n");
        LJM_ErrorToString(err, err_str);
        print_error("%s\n",err_str);
        return LCONF_ERROR;
    }
    return LCONF_NOERR;
}


int lc_stream_clean(lc_devconf_t* dconf){
    clean_buffer(&dconf->RB);
    return LCONF_NOERR;
}


int lc_datafile_init(lc_devconf_t* dconf, FILE* FF){
    time_t now;

    // Write the configuration header
    lc_write_config(dconf,FF);
    // Log the time
    time(&now);
    fprintf(FF, "#: %s", ctime(&now));

    return LCONF_NOERR;
}




int lc_datafile_write(lc_devconf_t* dconf, FILE* FF){
    int err,index,row,ainum;
    unsigned int channels, samples_per_read;
    double *data = NULL;

    err = lc_stream_read(dconf,&data,&channels,&samples_per_read);
    if(data){
        index = 0;
        // Write data one element at a time.
        for(row=0; row<samples_per_read; row++){
            for(ainum=0; ainum<channels-1; ainum++)
                fprintf(FF, "%.6e\t", data[ index++ ]);
            fprintf(FF, "%.6e\n", data[ index++ ]);
        }
    }
    return err;
}



