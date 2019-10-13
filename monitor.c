#include "ldisplay.h"       // For the display helper functions
#include "lgas.h"           // For gas measurements from the U12
#include "psat.h"           // For water/steam properties in heat calculations
#include "lconfig.h"
#include <unistd.h>         


// Where should the columns be displayed on the screen?
#define COL1 25
#define COL2 60

#define CONFIG_FILE "monitor.conf"
#define NAVG_MAX 1024
#define INPUT_LEN   128

/********************************
 *                              *
 *      Global Variables        *
 *                              *
 ********************************/
// Descriptions beginning with a * are calculated instead of measured
// Plate condition
double  plate_Thigh_C,      // Upper thermocouple temperature (C)
        plate_Tlow_C,       // Lower thermocouple temperature (C)
        plate_Q_kW,         // *Plate heating (kW)
        plate_Tpeak_C;      // *Plate peak temperature (C)
// Gas flow rates
double  oxygen_scfh,        // Oxygen flow rate (scfh)
        fuel_scfh,          // Fuel flow rate (scfh)
        flow_scfh,          // *Total flow rate (scfh)
        ratio_fto;          // *Fuel-to-oxygen volumetric ratio
// Coolant condition
double  water_gph,          // Water flow in gallons per hour
        water_gps,          // *Water mass flow in grams per second
        air_psig,           // Air pressure at the orifice
        air_gps,            // *Air mass flow in grams per second
        cool_Thigh_C,       // Coolant mixture high temperature (C)
        cool_Tlow_C,        // Coolant mixture inlet temperature (C)
        cool_Q_kW;          // *Coolant heat in kW
// Torch condition
double  standoff_in;        // Standoff distance in inches

// Prompt for UI
const int escape = 'p';
const char prompt[] = "Enter a command\n"\
"w2.7   Changes water flow rate to 2.7gph\n"\
"a79.5  Changes air pressure to 79.5psig\n"\
"s.275  Changes standoff height to .275in\n"\
"q or quit or e or exit will quit monitor.bin\n"\
":";

/********************************
 *                              *
 *          Prototypes          *
 *                              *
 ********************************/

/* GET_ANALOG
.   Get the user input for the analog/mechanical measurements.  These include
.   coolant flow rates and the torch standoff.
.
.   Returns 1 on failure; 0 on success.
*/
int get_analog(double * read_water, double * read_air, double * read_standoff);


/* GET_TC
.   Get thermocouple measurements.  Writes results to global variables 
.   plate_Thigh_C, plate_Tlow_C, cool_Thigh_C, and cool_Tlow_C.
*/
int get_tc(DEVCONF* localdconf, const int devnum);


/* COOLANT_HEAT
.   How much heat went into the coolant.  Uses global variables air_gps, 
.   water_gps, cool_Thigh_C, cool_Tlow_C.  Writes result to cool_Q_kW.
* /
int coolant_heat(void);
*/

/* PLATE_HEAT
.   Calculates the heat conducted through the plate in kW and the peak plate
.   temperature in degrees C.  PLATE_HEAT uses global variables, plate_Thigh_C, 
.   plate_Tlow_C, cool_Thigh_C, cool_Tlow_C. It writes results to global 
.   varaibles plate_Q_kW and plate_Tpeak_C.
* /
int plate_heat(void);
*/

/* INIT_DISPLAY
.   This prints the parameter text and headers to the screen.  The 
.   UPDATE_DISPLAY function will print the values that go with them.
*/
void init_display(void);


/* UPDATE_DISPLAY
.   Use the global variables to update the display values.  This function 
.   overwrites the old displayed data with new data.
*/
void update_display(void);



/********************************
 *                              *
 *          Algorithm           *
 *                              *
 ********************************/

int main(){
    int ii, jj;
    double ftemp;
    DEVCONF dconf[1];
    static const double orifice_mm2 = 0.4948;   // 1/32" orifice area
    char input[INPUT_LEN];
    char go_f = 1;

	load_config(dconf, 1, CONFIG_FILE);
    open_config(dconf,0);
    upload_config(dconf,0);
    
    // Get the oxygen and fuel gas zero settings
    if(!get_meta_flt(dconf,0,"o2offset",&ftemp))
        LGAS_O2_OFFSET_SCFH = ftemp;
    if(!get_meta_flt(dconf,0,"fgoffset",&ftemp))
        LGAS_FG_OFFSET_SCFH = ftemp;
    

    
    init_display();
    setup_keypress();
    while(go_f){
        // Get gas flow rates
        get_gas(&oxygen_scfh, &fuel_scfh);
        // Update the flow and ratio calculations
        flow_scfh = oxygen_scfh + fuel_scfh;
        ratio_fto = fuel_scfh / oxygen_scfh;

		// Get thermocouples
		get_tc(dconf, 0);

        // User input?
        if(prompt_on_keypress(escape,prompt,input,INPUT_LEN)){
            switch(input[0]){
                case 'w':
                    if(sscanf(&input[1],"%lf",&water_gph)==1)
                        water_gps = 1.05139 * water_gph;
                break;
                case 'a':
                    if(sscanf(&input[1],"%lf",&air_psig)==1)
                        air_gps = (air_psig + 14.7) * 0.015907485 * orifice_mm2;
                break;
                case 's':
                    sscanf(&input[1],"%lf",&standoff_in);
                break;
                case 'q':
                case 'e':
                    go_f = 0;
                break;
            }
            // Redraw the display
            init_display();
        }

        // Finally, update the output values
        update_display();
    }

    finish_keypress();
    close_config(dconf, 0);
    return 0;
}

/*
//******************************************************************************
int get_analog(double * read_water, double * read_air, double * read_standoff){
    FILE* ff;
    unsigned int count;
    float a,b,c;

    // First, get the coolant information from the coolant file.
    ff = fopen(ANALOG_FILE,"r");
    count = fscanf(ff, "%f\n%f\n%f", &a, &b, &c);
    fclose(ff);

    // Only update the data if no error was encountered
    if(count == 3){
        *read_water = a;
        *read_air = b;
        *read_standoff = c;
    }else
        return 1;
    return 0;
}
*/


//******************************************************************************
int get_tc(DEVCONF* localdconf, const int devnum){
    double *data=NULL;
    double Tamb, V[4], T[4];
    unsigned int ii, jj, channels, samples_per_read;

    // Collect raw thermocouple voltages
    // This is a blocking operation!
    start_data_stream(localdconf,devnum,-1);
    while(data==NULL){
		service_data_stream(localdconf,devnum);
		read_data_stream(localdconf,devnum, &data, &channels, &samples_per_read);
	}
    stop_data_stream(localdconf,devnum);

    // Get the approximate ambient temperature
    LJM_eReadName(localdconf[devnum].handle, "TEMPERATURE_AIR_K", &Tamb);

    // Average the tiny voltages and convert to temperature
    for(jj=0; jj<4; jj++){
        for(ii=0; ii<localdconf[devnum].nsample; ii++)
            V[jj] += data[ii*4+jj];
        V[jj]/=localdconf[devnum].nsample;
        LJM_TCVoltsToTemp(LJM_ttK, V[jj], Tamb, &T[jj]);
        T[jj] -= 273.15;    // convert to C
    }
    // Map the respective temperatures to their appropriate values
    plate_Thigh_C = T[0];
    plate_Tlow_C = T[1];
    cool_Thigh_C = T[2];
    cool_Tlow_C = T[3];
}


//*****************************************************************************
void init_display(void){
    clear_terminal();

    // Column 1: Temperature Measurements
    //  Plate temperature group
    print_header(2,1,"Plate Temperature Measurements");
    print_bparam(3,COL1,"Surface Max (C)");
    print_param(4,COL1,"Plate High T (C)");
    print_param(5,COL1,"Plate Low T (C)");
    print_bparam(6,COL1,"Heat (kW)");
    //  Coolant Temperature group
    print_header(8,1,"Coolant Measurements");
    print_bparam(9,COL1,"Coolant High T (C)");
    print_param(10,COL1,"Coolant Low T (C)");
    print_param(11,COL1,"Water (gps)");
    print_param(12,COL1,"Air (gps)");
    print_bparam(13,COL1,"Heat (kW)");

    // Column 2: Torch Measurements
    //  Gas flow rates
    print_header(2,40,"Gas Flow Rates");
    print_bparam(3,COL2,"Total Flow (scfh)");
    print_bparam(4,COL2,"Ratio (F/O)");
    print_param(5,COL2,"Fuel Gas (scfh)");
    print_param(6,COL2,"Oxygen (scfh)");

    print_header(8,40,"Configured by user");
    print_param(9,COL2,"Water (GPH)");
    print_param(10,COL2,"Air (PSIG)");
    print_param(11,COL2,"Standoff (in)");
}

//*****************************************************************************
void update_display(void){

    // Column 1: Temperature Measurements
    //  Plate temperature group
    print_bint(3,COL1,plate_Tpeak_C);
    print_int(4,COL1,plate_Thigh_C);
    print_int(5,COL1,plate_Tlow_C);
    print_bflt(6,COL1,plate_Q_kW);
    //  Coolant Temperature group
    print_bint(9,COL1,cool_Thigh_C);
    print_int(10,COL1,cool_Tlow_C);
    print_flt(11,COL1,water_gps);
    print_flt(12,COL1,air_gps);
    print_bflt(13,COL1,cool_Q_kW);

    // Column 2: Torch Measurements
    //  Gas flow rates
    print_bflt(3,COL2,flow_scfh);
    print_bflt(4,COL2,ratio_fto);
    print_flt(5,COL2,fuel_scfh);
    print_flt(6,COL2,oxygen_scfh);

    print_flt(9,COL2,water_gph);
    print_flt(10,COL2,air_psig);
    print_flt(11,COL2,standoff_in);

    LDISP_CGO(15,1);
    fflush(stdout);
}

/*
//*****************************************************************************
int coolant_heat(void){
    double  Thigh_K,        // High temperature in K
            Tlow_K,         // Low temperature in K
            water_vap1_gps, // The water vapor flow rate at the inlet
            water_vap2_gps, // The water vapor flow rate at the outlet
            dh1,            // Latent enthalpy at the inlet
            dh2,            // Latent enthalpy at the outlet
            xv1,            // Vapor mole fraction at the inlet
            xv2,            // Vapor mole fraction at the outlet
            Q;              // The result; heat
    // estimates for inlet and outlet total pressures in MPa
    static const double ptot1 = 0.13586, ptot2 = 0.101325;
    // molar weights for water and air
    static const double ww = 18.015, wa = 28.97;
    // specific heat of liquid water and air in J/g/K
    static const double cpw = 4.182, cpa = 1.005;
    
    // Convert the units

    // Go from C to K
    Tlow_K = cool_Tlow_C + 273.15;
    Thigh_K = cool_Thigh_C + 273.15;

    // Latent heats in J/g
    dh1 = latent(Tlow_K);
    dh2 = latent(Thigh_K);
    // d-less partial pressures
    xv1 = psat(Tlow_K)/ptot1;
    xv2 = psat(Thigh_K)/ptot2;

    // If we're below the triple point or above the critical point
    // Something is VERY VERY WRONG
    if(xv1<0 || xv2<0) return -1.;
    // If the temperature has exceeded the saturation temperature at this pressure
    // The estimate will be rough
    if(xv1 >= 1.) water_vap1_gps = water_gps;
    else water_vap1_gps = air_gps * (ww / wa) * xv1 / (1. - xv1);

    if(xv2 >= 1.) water_vap2_gps = water_gps;
    else water_vap2_gps = air_gps * (ww / wa) * xv2 / (1. - xv2);

    // Check to see if all the water is vapor.  This can happen at low water 
    // flow rates
    if(water_vap1_gps > water_gps) water_vap1_gps = water_gps;
    if(water_vap2_gps > water_gps) water_vap2_gps = water_gps;

    Q = (air_gps*cpa + water_gps*cpw)*(Thigh_K - Tlow_K) + \
        water_vap2_gps * dh2 - water_vap1_gps * dh1;

    // Convert to kJ
    cool_Q_kW = .001 * Q;
    return 0;
}
*/

/*
//*****************************************************************************
int plate_heat(void){
    double dT, n, th, tl, Tc;
    // Nominal temperature drop across the plate
    dT = 1.4556 * (plate_Thigh_C - plate_Tlow_C);
    Tc = 0.5*(cool_Tlow_C + cool_Thigh_C);
    // dimensionless high and low temperatures
    th = (plate_Thigh_C - Tc)/dT;
    tl = (plate_Tlow_C - Tc)/dT;
    // dimensionless plate conductivity
    n = 1. + (1.894*plate_Tlow_C - 1.207*plate_Thigh_C)/\
            (1.0032*plate_Thigh_C - 1.0005*plate_Tlow_C);
    plate_Tpeak_C = (3.903 + n)*dT + Tc;
    plate_Q_kW = .0042 * dT;
    return 0;
}
*/
