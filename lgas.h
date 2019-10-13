/* LGAS.H
.   Tools for measuring gas flow from the LabJack U12.
.   These functions measure gas flows assuming the U12 is connected to a pair of
.   Teledyne-Hastings thermal mass flow meters measuring the flow of oxygen and
.   methane.
*/

#ifndef __LGAS
#define __LGAS

#include <ljacklm.h>
#include <stdio.h>
#include <unistd.h>

#define LGAS_VERSION 1.0



/********************************
 *                              *
 *      Global Configuration    *
 *                              *
 ********************************/
// These variables are intended to be changed by host software as necessary
// to configure the behavior of the functions
long LGAS_U12_ID = -1;
long LGAS_O2_CHANNEL = 9;
long LGAS_FG_CHANNEL = 8;
// Calibration parameters
double LGAS_O2_SLOPE_SCFH = 10.;    // scfh per volt
double LGAS_O2_OFFSET_SCFH = 0;    // scfh
double LGAS_FG_SLOPE_SCFH = 6.;     // scfh per volt
double LGAS_FG_OFFSET_SCFH = 0;    // scfh
// Properties
// These are used to convert between volume and mass flows
// Changing these will effectively change the gas being used
double LGAS_O2_MW = 31.988;
double LGAS_FG_MW = 16.04246;
double LGAS_TREF_K = 273.15;
double LGAS_PREF_PA = 100000.;



/********************************
 *                              *
 *          Prototypes          *
 *                              *
 ********************************/

/* GET_GAS
.   Obtain differential the voltages from the U12 and apply the calibration 
.   constants.
.
.   Returns 0 on success and 1 on an error.
*/
int get_gas(double * o2_scfh, double * fg_scfh);


/* ZERO_GAS
.	Collect zero flow rate measurements to determine the calibration offsets.
.	If the measurements do not appear to be zero, the operation is aborted and
.   a non-zero error code is returned.
*/
int zero_gas(void);

/* CONVERT_TO_MASS
.   Convert from scfh to a mass flow; requires the molecular weight of the gas.
.   Returns mass flow in grams per second.
*/
double convert_to_mass(double scfh, double mw);


/* CONVERT_TO_MOLES
.   Convert from scfh to molar flow.  Returns molar flow in moles per second
*/
double convert_to_moles(double scfh);


/********************************
 *                              *
 *          Algorithm           *
 *                              *
 ********************************/


//******************************************************************************
int get_gas(double * o2_scfh, double * fg_scfh){
    long err, over;
    char error_string[50];
    const long gain = 3;    // +/- 5V range
    float volts;

    // Read in the oxygen flow
    err = EAnalogIn( &LGAS_U12_ID, 0, LGAS_O2_CHANNEL, gain, &over, &volts);
    if(err){
        GetErrorString(err,error_string);
        printf( "GET_GAS: Error durring oxygen measurement.\n"
                "Received error: %s\n", error_string);
        return 1;
    }
    if(over)
        printf( "GET_GAS: Oxygen voltage exceeded measurement range.\n");
    // Apply the calibration    
    *o2_scfh = LGAS_O2_SLOPE_SCFH * volts + LGAS_O2_OFFSET_SCFH;


    // Read in the FG flow
    err = EAnalogIn( &LGAS_U12_ID, 0, LGAS_FG_CHANNEL, gain, &over, &volts);
    if(err){
        GetErrorString(err,error_string);
        printf( "GET_GAS: Error durring fuel gas measurement.\n"
                "Received error: %s\n", error_string);
        return 1;
    }
    if(over)
        printf( "GET_GAS: Fuel gas voltage exceeded measurement range.\n");
    // Apply the calibration
    *fg_scfh = LGAS_FG_SLOPE_SCFH * volts + LGAS_FG_OFFSET_SCFH;

    return 0;
}


int zero_gas(void){
    long err, over;
    char error_string[50];
    const long gain = 3;    // +/- 5V range
	const float small = 1.;
    float volts;

    // Read in the oxygen flow
    err = EAnalogIn( &LGAS_U12_ID, 0, LGAS_O2_CHANNEL, gain, &over, &volts);
    if(err){
        GetErrorString(err,error_string);
        printf( "ZERO_GAS: Error durring oxygen measurement.\n"
                "Received error: %s\n", error_string);
        return 1;
    }
    if(over){
        printf( "ZERO_GAS: Oxygen voltage exceeded measurement range.\n");
	}

	// If the voltage isn't small!
	if(volts*volts > small*small){
		printf("ZERO_GAS: Oxygen voltage exceeded %f V\n", volts);
		return 1;
	}else{
		// Apply the calibration
		LGAS_O2_OFFSET_SCFH = - volts * LGAS_O2_SLOPE_SCFH;
	}

    // Read in the FG flow
    err = EAnalogIn( &LGAS_U12_ID, 0, LGAS_FG_CHANNEL, gain, &over, &volts);
    if(err){
        GetErrorString(err,error_string);
        printf( "ZERO_GAS: Error durring fuel gas measurement.\n"
                "Received error: %s\n", error_string);
        return 1;
    }
    if(over){
        printf( "ZERO_GAS: Fuel gas voltage exceeded measurement range.\n");
	}

	// If the voltage isn't small!
	if(volts*volts > small*small){
		printf("ZERO_GAS: Fuel gas voltage exceeded %f V\n", volts);
		return 1;
	}else{
		// Apply the calibration
		LGAS_FG_OFFSET_SCFH = - volts * LGAS_FG_SLOPE_SCFH;
	}

    return 0;
}




double convert_to_mass(double scfh, double mw){
    return mw * convert_to_moles(scfh);
}


double convert_to_moles(double scfh){
    static const double R = 8.314;
    double ncms;
    // convert to normal cubic meters per second
    ncms = 7.865e-6 * scfh;
    return LGAS_PREF_PA * ncms / R / LGAS_TREF_K;
}


#endif
