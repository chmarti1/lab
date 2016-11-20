/*
.
.   Tools for printing real time information to a terminal
.
.   (c) 2016
.   Released under GPLv3.0
.   Chris Martin
.   Assistant Professor of Mechanical Engineering
.   Penn State University, Altoona College
.
.   Tested on the Linux kernal 3.19.0
.   Linux Mint 17.3 Rosa
.
*/


#ifndef __LDISPLAY
#define __LDISPLAY


// Add some headers
#include <unistd.h>
#include <stdio.h>
#include <string.h>
//#include <time.h>
#include <termios.h>
#include <sys/select.h>
//#include <sys/time.h>
//#include <sys/types.h>


/* CHANGELOG
These change logs follow the convention below:
**LCONF_VERSION
Date
Notes

**1.0
6/30/2016
Original version.  Includes print_param(), print_str(), print_int(), and 
print_flt().
*/




/****************************
 *                          *
 *       Constants          *
 *                          *
 ****************************/

#define LDISP_VERSION 1.0

/*
.   Macros for moving the cursor around
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
#define LDISP_MAX(a,b)      (a>b?a:b)
// Return the minimum integer
#define LDISP_MIN(a,b)      (a>b?b:a)
// Clamp an integer between two extrema
// assumes a > b and returns c clamped between a and b
#define LDISP_CLAMP(a,b,c)  LDISP_MAX(LDISP_MIN(a,c), b)



/*
Help on Linux terminal control characters

$man console_codes

gets the job done.
*/




/****************************
 *                          *
 *       Prototypes         *
 *                          *
 ****************************/

/* CLEAR TERMINAL
.   This will clean up the terminal display.
.   We move the cursor to home and clear the display
*/
void clear_terminal(void);


/* PRINT TEXT AT A LOCATION
.   This prints text starting at a row,column location
*/
void print_text(const unsigned int row,
                const unsigned int column,
                const char * text);

/* PRINT HEADER TEXT AT A LOCATION
.   This prints text starting at a row,column location
*/
void print_header(const unsigned int row,
                const unsigned int column,
                const char * text);




/* PRINT PARAM and PRINT XXX
.   print_param() and print_XXX() are responsible for drawing and refreshing
.   displays in a console.  They build and refresh a display of the format
.
.       param : value
.
.   The parameter label appears to the left of a colon and its value to the 
.   right; separated by spaces.  print_param() draws the parameter string and 
.   the print_XXX() functions print over the old values with strings formatted
.   based on the data type.
.
.   The row and column indicate the location in the console where the colon 
.   should be located.  Care should be taken to provide enough space for the 
.   formatted text.  (1,1) represents the upper left-hand corner of the 
.   terminal.
*/
void print_param(const unsigned int row, 
                const unsigned int column, 
                const char * param);

void print_str(const unsigned int row,
                const unsigned int column,
                const char * value);

void print_int(const unsigned int row,
                const unsigned int column,
                const int value);

void print_flt(const unsigned int row,
                const unsigned int column,
                const double value);

/* BOLD DISPLAY
.   These are identical to their regular counterparts, but they display bold
.   characters instead of ordinary characters.
*/
void print_bparam(const unsigned int row, 
                const unsigned int column, 
                const char * param);

void print_bstr(const unsigned int row,
                const unsigned int column,
                const char * value);

void print_bint(const unsigned int row,
                const unsigned int column,
                const int value);

void print_bflt(const unsigned int row,
                const unsigned int column,
                const double value);


/* KEYPRESS
.   Detect whether there is new data waiting on the stdin stream.  This services
.   a non-blocking keyboard input.  Returns 1 if there is new input.  Returns
.   0 if not.
.
.   To work as expected, the setup_keypress() function should be called first. 
.   The finish_keypress() will return the terminal to normal operation.  See 
.   those funcitons for more information.
*/
char keypress(void);

/* SETUP_KEYPRESS
.   Configure the terminal to allow the KEYPRESS function to work properly.  In
.   normal (Canonical) operation, terminal input is witheld until a user presses
.   enter (EOL).  SETUP_KEYPRESS reconfigures the terminal to return individual
.   characters immediately.
*/
void setup_keypress(void);

/* FINISH_KEYPRESS
.   Configure the terminal to behave normally after waiting for a KEYPRESS.  
.   While waiting for a keypress, key entry at the terminal is passed to the 
.   file stream immediately without waiting for enter (EOL character).  However,
.   in normal operation, this is not ideal as it would prohibit entry editing.
*/
void finish_keypress(void);

/* PROMPT ON KEYPRESS
.   On a keypress, block a loop to prompt the user for input.  When the 
.   SETUP_KEYPRESS function is called prior to a loop, the PROMPT_ON_KEYPRESS
.   function can be used to initiate a "blocking" prompt for user input only if
.   the user has pressed a particular key.  Otherwise, the loop will continue
.   unaffected.  The function returns 1 if the prompt was triggered and 0 
.   otherwise.
.
.   look_for    If look_for < 0, then any key will trigger the prompt.
.               Otherwise, look_for is interpreted as a character.  If that 
.               characteris found on stdin, the prompt will be triggered.
.   prompt      This string will be printed to stdout to prompt the user for 
.               input.
.   input       This is the string returned by the prompt.
.   length      This is the maximum acceptable string length.
*/
int prompt_on_keypress(int look_for, const char* prompt, char* input, unsigned int length);


/****************************
 *                          *
 *       Algorithm          *
 *                          *
 ****************************/
//******************************************************************************
void clear_terminal(void){
    // Move the cursor to home and clear from the cursor to the end
    printf("\x1B[H\x1B[J");
}

//******************************************************************************
void print_text(const unsigned int row,
                const unsigned int column,
                const char * text){
    printf(LDISP_FMT_TXT,row,column,text);
}

//******************************************************************************
void print_header(const unsigned int row,
                const unsigned int column,
                const char * text){
    printf(LDISP_FMT_HDR,row,column,text);
}

//******************************************************************************
void print_param(const unsigned int row, 
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
void print_str(const unsigned int row, 
                const unsigned int column, 
                const char * value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_STR,row,column+2,value);
}

//******************************************************************************
void print_int(const unsigned int row, 
                const unsigned int column, 
                const int value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_INT,row,column+2,value);
}

//******************************************************************************
void print_flt(const unsigned int row, 
                const unsigned int column, 
                const double value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_FLT,row,column+2,value);
}

//******************************************************************************
void print_bparam(const unsigned int row, 
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
void print_bstr(const unsigned int row, 
                const unsigned int column, 
                const char * value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_BSTR,row,column+2,value);
}

//******************************************************************************
void print_bint(const unsigned int row, 
                const unsigned int column, 
                const int value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_BINT,row,column+2,value);
}

//******************************************************************************
void print_bflt(const unsigned int row, 
                const unsigned int column, 
                const double value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_BFLT,row,column+2,value);
}

//******************************************************************************
char keypress(void){
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
void setup_keypress(void){
    struct termios terminal_state;
    tcgetattr(LDISP_STDIN_FD, &terminal_state);     // Get the current state
    terminal_state.c_lflag &= ~(ICANON | ECHO);   // Clear the canonical bit
                                                  // Turn off the echo
    terminal_state.c_cc[VMIN] = 1;      // Set the minimum number of characters
    tcsetattr(LDISP_STDIN_FD, TCSANOW, &terminal_state); // Apply the changes
    // TCSANOW = Terminal Control Set Attribute Now; controls timing
}

//******************************************************************************
void finish_keypress(void){
    struct termios terminal_state;
    tcgetattr(LDISP_STDIN_FD, &terminal_state);     // Get the current state
    terminal_state.c_lflag |= (ICANON | ECHO);  // Set the canonical bit
                                                // Turn on the echo
    tcsetattr(LDISP_STDIN_FD, TCSANOW, &terminal_state); // Apply the changes
    // TCSANOW = Terminal Control Set Attribute Now; controls timing
}

//******************************************************************************
int prompt_on_keypress(int look_for, const char* prompt, char* input, const unsigned int length){
    if(     keypress() && \
            (getchar()==(char)look_for || look_for<0)){
        finish_keypress();
        fputs(prompt,stdout);
        fgets(input,length,stdin);
        setup_keypress();
        return 1;
    }
    return 0;
}

#endif
