/* LCMAP
The LCONFIG MAP header defines global constants mapping configuration text 
strings to their corresponding enumerated values and to well formatted text
that can be printed to a human interface to describe the configuration state.
*/

#ifndef __LC_MESSAGE
#define __LC_MESSAGE

#include <string.h>
#include "lconfig.h"

#define LCM_MAX_VALUE 255


typedef struct {
    int value;
    char *message;
    char *config;
} lcm_map_t;

//
// Default unsupported messages
//
static const char * lcm_errors[] = {"?", "CORRUPT LCM MESSAGE MAP!"};

//
// Connection Status
//
static const lcm_map_t lcm_connection_status[] = {
    {.value=0, .message="Closed"},
    {.value=1, .message="Open"},
    {.value=-1}
};

//
// Connection Type
//
static const lcm_map_t lcm_connection[] = {
    {.value=LC_CON_USB, .message="USB", .config="usb"},
    {.value=LC_CON_ANY, .message="Any", .config="any"},
    {.value=LC_CON_ANY_TCP, .message="Any TCP/IP"},
    {.value=LC_CON_ETH, .message="Ethernet", .config="ethernet"},
    {.value=LC_CON_ETH, .message="Ethernet", .config="eth"},
    {.value=LC_CON_ETH_TCP, .message="Ethernet TCP/IP"},
    {.value=LC_CON_ETH_UDP, .message="Ethernet UDP"},
    {.value=LC_CON_WIFI, .message="WiFi", .config="wifi"},
    {.value=LC_CON_WIFI_TCP, .message="WiFi TCP/IP"},
    {.value=LC_CON_WIFI_UDP, .message="WiFi UDP"},
    {.value=-1}
};

//
// Device type
//
static const lcm_map_t lcm_device[] = {
    {.value=LC_DEV_ANY, .message="Any", .config="any"},
    {.value=LC_DEV_T4, .message="T4", .config="t4"},
    {.value=LC_DEV_T7, .message="T7", .config="t7"},
    {.value=LC_DEV_DIGIT, .message="Digit"},
    {.value=-1}
};

//
// Analog Output Signal
//
static const lcm_map_t lcm_aosignal[] = {
    {.value=LC_AO_CONSTANT, .message="Constant", .config="constant"},
    {.value=LC_AO_SINE, .message="Sine Wave", .config="sine"},
    {.value=LC_AO_SQUARE, .message="Square Wave", .config="square"},
    {.value=LC_AO_TRIANGLE, .message="Triangle Wave", .config="triangle"},
    {.value=LC_AO_NOISE, .message="White Noise", .config="noise"},
    {.value=-1}
};

//
// Edges
//
static const lcm_map_t lcm_edge[] = {
    {.value=LC_EDGE_ANY, .message="Rising or Falling", .config="any"},
    {.value=LC_EDGE_ANY, .message="Rising or Falling", .config="all"},
    {.value=LC_EDGE_RISING, .message="Rising", .config="rising"},
    {.value=LC_EDGE_FALLING, .message="Falling", .config="falling"},
    {.value=-1}
};

//
// Extended Feature Signal
//
static const lcm_map_t lcm_ef_signal[] = {
    {.value=LC_EF_PWM, .message="Pulse Width Modulation", .config="pwm"},
    {.value=LC_EF_COUNT, .message="Counter", .config="counter"},
    {.value=LC_EF_FREQUENCY, .message="Pulse Frequency", .config="frequency"},
    {.value=LC_EF_PHASE, .message="Relative Phase", .config="phase"},
    {.value=LC_EF_QUADRATURE, .message="Encoder Quadrature", .config="quadrature"},
    {.value=-1}
};

//
// Extended Feature Direction
//
static const lcm_map_t lcm_ef_direction[] = {
    {.value=LC_EF_INPUT, .message="Input", .config="input"},
    {.value=LC_EF_OUTPUT, .message="Output", .config="output"},
    {.value=-1}
};

//
// Extended Feature Debounce
//
static const lcm_map_t lcm_ef_debounce[] = {
    {.value=LC_EF_DEBOUNCE_NONE, .message="None", .config="none"},
    {.value=LC_EF_DEBOUNCE_FIXED, .message="Fixed Timer", .config="fixed"},
    {.value=LC_EF_DEBOUNCE_RESET, .message="Resetting Timer", .config="reset"},
    {.value=LC_EF_DEBOUNCE_MINIMUM, .message="Minimum Pulse Width", .config="minimum"},
    {.value=-1}
};

//
// COM Channel
//
static const lcm_map_t lcm_com_channel[] = {
    {.value=LC_COM_NONE, .message="None", .config="none"},
    {.value=LC_COM_UART, .message="Asynchronous Rx/Tx", .config="uart"},
    {.value=LC_COM_SPI, .message="Serial Peripheral Interface", .config="spi"},
    {.value=LC_COM_I2C, .message="Inter-Integrated Circuit", .config="i2c"},
    {.value=LC_COM_1WIRE, .message="One-Wire", .config="1wire"},
    {.value=LC_COM_SBUS, .message="S-Bus", .config="sbus"},
    {.value=-1}
};

//
// COM Parity
//
static const lcm_map_t lcm_com_parity[] = {
    {.value=LC_PARITY_NONE, .message="None", .config="N"},
    {.value=LC_PARITY_EVEN, .message="Even", .config="E"},
    {.value=LC_PARITY_ODD, .message="Odd", .config="O"},
    {.value=LC_PARITY_NONE, .message="None", .config="n"},
    {.value=LC_PARITY_EVEN, .message="Even", .config="e"},
    {.value=LC_PARITY_ODD, .message="Odd", .config="o"},
    {.value=-1}
};

/* LCM_GET_MESSAGE
The MAP is an array of lcm_map_t structs mapping an integer/enum VALUE
to a human-readable string describing the state of the configuration.
*/
const char * lcm_get_message(const lcm_map_t map[], int value){
    int ii;
    // Loop through the map values; examine no more than 256 values!
    for(ii=0; ii<=LCM_MAX_VALUE; ii++){
        // If this is the end of the map, 
        if(map[ii].value<0)
            return lcm_errors[0];
        else if(map[ii].value == value){
            if(map[ii].message)
                return map[ii].message;
            else
                return lcm_errors[0];
        }
    }
    return lcm_errors[1];
}

/* LCM_GET_CONFIG
The MAP is an array of lcm_map_t structs mapping an integer/enum VALUE
to a configuration string value
*/
const char * lcm_get_config(const lcm_map_t map[], int value){
    int ii;
    // Loop through the map values; examine no more than 256 values!
    for(ii=0; ii<=LCM_MAX_VALUE; ii++){
        // If this is the end of the map, 
        if(map[ii].value<0)
            return lcm_errors[0];
        else if(map[ii].value == value){
            if(map[ii].config)
                return map[ii].config;
            else
                return lcm_errors[0];
        }
    }
    return lcm_errors[1];
}

/* LCM_GET_VALUE
Given a configuration string, return the enumerated value from the map.  If the
configuration string is not found, return -1.  On success, the enumerated map 
value is returned in VALUE
*/
int lcm_get_value(const lcm_map_t map[], char config[], int *value){
    int ii;
    // Loop through the map values; examine no more than 256 values!
    for(ii=0; ii<=LCM_MAX_VALUE; ii++){
        // If this is the end of the map, 
        if(map[ii].value<0)
            return -1;
        else if(map[ii].config && strcmp(map[ii].config,config) == 0){
            *value = map[ii].value;
            return 0;
        }
    }
    return -1;
}

#endif
