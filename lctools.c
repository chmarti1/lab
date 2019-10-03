#include "lctools.h"
#include "lconfig.h"


/*
.   Macros for moving the cursor around
*/


/*
Help on Linux terminal control characters

$man console_codes

gets the job done.
*/


// Move the cursor to home (1,1)
#define LDISP_CHOME         printf("\x1B[H")
// Move the cursor to (x,y)
#define LDISP_CGO(x,y)      printf("\x1B[%d;%dH",x,y)
// The value format specifiers
// Default to 15 character width to overwrite previous long outputs
// Use 3 decimal places on floating precision numbers
#define LDISP_VALUE_LEN     "15"
#define LDISP_FLT_PREC      "3"
#define LDISP_FMT_TXT       "\x1B[%d;%dH%s"
#define LDISP_FMT_HDR       "\x1B[%d;%dH\x1B[4m%s\x1B[0m"
#define LDISP_FMT_PRM       "\x1B[%d;%dH%s :"
#define LDISP_FMT_STR       "\x1B[%d;%dH%-" LDISP_VALUE_LEN "s"
#define LDISP_FMT_INT       "\x1B[%d;%dH%-" LDISP_VALUE_LEN "d"
#define LDISP_FMT_FLT       "\x1B[%d;%dH%-" LDISP_VALUE_LEN "." LDISP_FLT_PREC "f"
#define LDISP_FMT_BPRM      "\x1B[%d;%dH\x1B[1m%s :\x1B[0m"
#define LDISP_FMT_BSTR      "\x1B[%d;%dH\x1B[1m%-" LDISP_VALUE_LEN "s\x1B[0m"
#define LDISP_FMT_BINT      "\x1B[%d;%dH\x1B[1m%-" LDISP_VALUE_LEN "d\x1B[0m"
#define LDISP_FMT_BFLT      "\x1B[%d;%dH\x1B[1m%-" LDISP_VALUE_LEN "." LDISP_FLT_PREC "f\x1B[0m"

#define LDISP_STDIN_FD      STDIN_FILENO

// Return the maximum integer
#define LDISP_MAX(a,b)      ((a)>(b)?(a):(b))
// Return the minimum integer
#define LDISP_MIN(a,b)      ((a)>(b)?(b):(a))
// Clamp an integer between two extrema
// assumes a > b and returns c clamped between a and b
#define LDISP_CLAMP(a,b,c)  LDISP_MAX(LDISP_MIN((a),(c)), (b))

/****************************
 *                          *
 *       Algorithm          *
 *                          *
 ****************************/

//******************************************************************************
void lct_clear_terminal(void){
    // Move the cursor to home and clear from the cursor to the end
    printf("\x1B[H\x1B[J");
}

//******************************************************************************
void lct_print_text(const unsigned int row,
                const unsigned int column,
                const char * text){
    printf(LDISP_FMT_TXT,row,column,text);
}

//******************************************************************************
void lct_print_header(const unsigned int row,
                const unsigned int column,
                const char * text){
    printf(LDISP_FMT_HDR,row,column,text);
}

//******************************************************************************
void lct_print_param(const unsigned int row, 
                const unsigned int column, 
                const char * param){
    int x;
    // Offset the starting location of the parameter by the length of the string
    // Include an extra character for the space
    x = column - strlen(param)-1;
    // Keep things from running off the edge
    // If there is a string overrun, the space and colon will be offset
    x = LDISP_MAX(1,x);
    printf(LDISP_FMT_PRM,row,x,param);
}

//******************************************************************************
void lct_print_str(const unsigned int row, 
                const unsigned int column, 
                const char * value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_STR,row,column+2,value);
}

//******************************************************************************
void lct_print_int(const unsigned int row, 
                const unsigned int column, 
                const int value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_INT,row,column+2,value);
}

//******************************************************************************
void lct_print_flt(const unsigned int row, 
                const unsigned int column, 
                const double value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_FLT,row,column+2,value);
}

//******************************************************************************
void lct_print_bparam(const unsigned int row, 
                const unsigned int column, 
                const char * param){
    int x;
    // Offset the starting location of the parameter by the length of the string
    // Include an extra character for the space
    x = column - strlen(param)-1;
    // Keep things from running off the edge
    // If there is a string overrun, the space and colon will be offset
    x = LDISP_MAX(1,x);
    printf(LDISP_FMT_BPRM,row,x,param);
}

//******************************************************************************
void lct_print_bstr(const unsigned int row, 
                const unsigned int column, 
                const char * value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_BSTR,row,column+2,value);
}

//******************************************************************************
void lct_print_bint(const unsigned int row, 
                const unsigned int column, 
                const int value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_BINT,row,column+2,value);
}

//******************************************************************************
void lct_print_bflt(const unsigned int row, 
                const unsigned int column, 
                const double value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_BFLT,row,column+2,value);
}

//******************************************************************************
char lct_is_keypress(void){
// Credit for this code goes to
// http://cc.byexamples.com/2007/04/08/non-blocking-user-input-in-loop-without-ncurses/
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(LDISP_STDIN_FD, &fds); //STDIN_FILENO is 0
    select(LDISP_STDIN_FD+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(LDISP_STDIN_FD, &fds);
}

//******************************************************************************
void lct_setup_keypress(void){
    struct termios terminal_state;
    tcgetattr(LDISP_STDIN_FD, &terminal_state);     // Get the current state
    terminal_state.c_lflag &= ~(ICANON | ECHO);   // Clear the canonical bit
                                                  // Turn off the echo
    terminal_state.c_cc[VMIN] = 1;      // Set the minimum number of characters
    tcsetattr(LDISP_STDIN_FD, TCSANOW, &terminal_state); // Apply the changes
    // TCSANOW = Terminal Control Set Attribute Now; controls timing
}

//******************************************************************************
void lct_finish_keypress(void){
    struct termios terminal_state;
    tcgetattr(LDISP_STDIN_FD, &terminal_state);     // Get the current state
    terminal_state.c_lflag |= (ICANON | ECHO);  // Set the canonical bit
                                                // Turn on the echo
    tcsetattr(LDISP_STDIN_FD, TCSANOW, &terminal_state); // Apply the changes
    // TCSANOW = Terminal Control Set Attribute Now; controls timing
}

//******************************************************************************
int lct_keypress_prompt(int look_for, const char* prompt, char* input, const unsigned int length){
    if(     lct_is_keypress() && \
            (getchar()==(char)look_for || look_for<0)){
        lct_finish_keypress();
        fputs(prompt,stdout);
        fgets(input,length,stdin);
        lct_setup_keypress();
        return 1;
    }
    return 0;
}




/************************************
 *                                  *
 *      Interacting with Channels   *
 *                                  *
 ************************************/
/* LCT_AI_BYLABEL
.  LCT_AO_BYLABEL
.  LCT_FIO_BYLABEL
.   These functions return the integer index of the appropriate channel with the
.   label that matches LABEL.  If no matching channel is found, the functions
.   return -1.
*/
int lct_ai_bylabel(lc_devconf_t *dconf, unsigned int devnum, char label[]){
    int ii;
    for(ii=0; ii<dconf->naich; ii++){
        if(strcmp(label, dconf->aich[ii].label)==0)
            return ii;
    }
    return -1;
}

int lct_ao_bylabel(lc_devconf_t *dconf, unsigned int devnum, char label[]){
    int ii;
    for(ii=0; ii<dconf->naoch; ii++){
        if(strcmp(label, dconf->aoch[ii].label)==0)
            return ii;
    }
    return -1;
}

int lct_ef_bylabel(lc_devconf_t *dconf, unsigned int devnum, char label[]){
    int ii;
    for(ii=0; ii<dconf->nefch; ii++){
        if(strcmp(label, dconf->efch[ii].label)==0)
            return ii;
    }
    return -1;
}

/************************************
 *                                  *
 *      Interacting with Data       *
 *                                  *
 ************************************/



int lct_diter_init(lc_devconf_t *dconf, lct_diter_t *diter,
                    double* data, unsigned int data_size, unsigned int channel){
    unsigned int ii;
    ii = lc_nistream(dconf);
    if(channel >= ii){
        diter->last = NULL;
        diter->next = NULL;
        return -1;
    }
    diter->increment = sizeof(double) * ii;
    diter->next = &data[channel];
    diter->last = &data[data_size];
    return 0;
}


double* lct_diter_next(lct_diter_t *diter){
    double *done;
    if(diter->next > diter->last){
        diter->last = NULL;
        return NULL;
    }
    done = diter->next;
    diter->next += diter->increment;
    return done;
}

/* LCT_DATA
.   A utility for indexing a data array in an application.  Presuming that an 
.   applicaiton has defined a double array and has streamed data into it using
.   the READ_DATA_STREAM function, the LCT_DATA function provides a pointer to
.   an element in the array corresponding to a given channel number and sample
.   number.  DCONF and DEVNUM are used to determine the number of channels being
.   streamed.  DATA is the data array being indexed, and DATA_SIZE is its total
.   length.  CHANNEL and SAMPLE determine which data point is to be returned.
.   
.   LTC_DATA is roughly equivalent to
.       &data[nistream_config(dconf,devnum)*sample + channel]
.
.   When CHANNEL or SAMPLE are out of range, LTC_DATA returns a NULL pointer.
*/
double * lct_data(lc_devconf_t *dconf, 
                double data[], unsigned int data_size,
                unsigned int channel, unsigned int sample){
    unsigned int ii;
    
    ii = lc_nistream(dconf);
    if(channel >= ii)
        return NULL;
    ii = sample*ii + channel;
    if(ii >= data_size)
        return NULL;
    return &data[ii];
}

/* LCT_CAL_INPLACE
.   Apply the channel calibrations in-place on the target array.  The contents
.   of the data array are presumed to be raw voltages as returned by the 
.   READ_DATA_STREAM function.  DCONF and DEVNUM are used to determine the 
.   calibration parameters, DATA is the array on which to operate, and DATA_SIZE
.   is its length.  
*/
void lct_cal_inplace(lc_devconf_t *dconf, 
                double data[], unsigned int data_size){
    lct_diter_t diter;
    double *this;
    unsigned int ii;
    // Loop through the analog channels
    for(ii = 0; ii<dconf->naich; ii++){
        // Initialize the iteration
        if(lct_diter_init(dconf,&diter,data,data_size,ii)){
            fprintf(stderr, "LCT_CAL_INPLACE: This error should never happen!  Calibration failed.\n"\
                    "    We sincerely apologize.\n");
            return;
        }
        while(this = lct_diter_next(&diter))
            *this = dconf->aich[ii].calslope * ((*this) - dconf->aich[ii].calzero);
    }
    return;
}

