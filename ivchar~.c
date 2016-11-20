// Current-Voltage Characteristic Acquisition

#include "ldisplay.h"       // For the display helper functions
#include "lconfig.h"        // For control of the T7
#include "lgas.h"           // For gas measurements from the U12
#include "psat.h"           // For water/steam properties in heat calculations
#include <math.h>           // for ceil()
#include <unistd.h>         


// Where should the columns be displayed on the screen?
#define COL1 25
#define COL2 60

// A text file for a user to enter manual/analog measurements
#define ANALOG_FILE "analog.dat"
#define CONFIG_FILE "ivchar.conf"
#define DATA_FILE   "ivchar.dat"
#define NAVG_MAX 1024

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



/* COOLANT_HEAT
.   How much heat went into the coolant?  Calculate from the flow rate of air
.   the flow rate of water and the temperature change.
.
.   COOLANT_HEAT assumes that the inlet air-water mixture will reach a three-way
.   equilibrium with liquid, vapor, and air.  
.
.   Tlow and Thigh are the coolant inlet and outlet temperatures in C.
*/
double coolant_heat(const double air_gps,
                    const double water_gps,
                    const double Tlow,
                    const double Thigh);


/* PLATE_HEAT
.   Given the two plate internal temperature measurements, use the plate thermal
.   model to estimate the plate's peak surface temperature, the temperature at
.   the edge of the flame, and the heat transferred to the plate.
.
.   T_low and T_high are the plate's internal temperatures in C.  T_cool is the
.   approximate coolant temperature in C.  That should probably be the average 
.   of the coolant inlet and outlet.
*/
void plate_heat(const double Tlow,
                    const double Thigh,
                    const double Tcool,
                    double* Tpeak,
                    double* heat);



/********************************
 *                              *
 *          Algorithm           *
 *                              *
 ********************************/

int main(){
    DEVCONF             dconf[2];
    char                charin;
    int                 ii, jj, repeat;
    double              ftemp, Tamb, V[4], T[4];
    static const double orifice_mm2 = 0.4948,   // 1/32" orifice area
                        sound_mps = 348.;    // speed of sound in air at 300K
    double              buffer[4*NAVG_MAX];

    printf("IVCHAR loading config file: %s\n", CONFIG_FILE);
    load_config(dconf, 2, CONFIG_FILE);

    printf("Treating configuration 0 as the temperature measurement\n");
    if(dconf[0].naich != 4){
        printf("ERROR! Need four analog input channels for the temperature measurement.\n");
        return 1;
    }

    open_config(dconf, 0);
    upload_config(dconf, 0);

    // Get the oxygen and fuel gas zero settings
    if(!get_meta_flt(dconf,0,"o2offset",&ftemp))
        LGAS_O2_OFFSET_SCFH = ftemp;
    if(!get_meta_flt(dconf,0,"fgoffset",&ftemp))
        LGAS_FG_OFFSET_SCFH = ftemp;
    printf( "Define flt:o2offset and flt:fgoffset to change the flow meter zeros\n"
            "Using values %f, %f respectively.\n", LGAS_O2_OFFSET_SCFH, LGAS_FG_OFFSET_SCFH);

    // Check to make sure the sample count will fit in the buffer
    if(dconf[0].nsample > NAVG_MAX){
        printf( "ERROR! Bad value for NSAMPLE in configuration file: %s\n"  
                "The maximum allowed by this binary is %d.\n", 
                CONFIG_FILE, NAVG_MAX);
        return 1;
    }

    // Update the analog measurements until the user is satisfied
    printf("Found values in analog file: %s\n",ANALOG_FILE);
    charin = 'n';
    while(charin!='Y'){
        get_analog(&water_gph, &air_psig, &standoff_in);
        printf( "   Water (GPH): %f\n"
                "    Air (PSIG): %f\n"
                " Standoff (IN): %f\n", water_gph, air_psig, standoff_in);
        printf("Correct? ('Y' to continue):");
        charin = getchar();
    }
    // Density of water is 1000 grams/Liter
    // There are 3.785 Liters per US gallon
    // There are 3600 seconds in an hour
    // 1.05139 == 1000 * 3.785 / 3600
    water_gps = 1.05139 * water_gph;
    // Critical flow is determined by the density of the air
    // psig + 14.7 == psia
    // m = d0 * a0 * A * / ((k+1)/2)**((k+1)/2(k-1))
    // d0 : density at the inlet
    // a0 : speed of sound at the inlet
    // A : area of the orifice
    // k : specific heat ratio
    air_gps = (air_psig + 14.7) * 0.015907485 * orifice_mm2;

    // Get gas flow rates
    get_gas(&oxygen_scfh, &fuel_scfh);
    // Update the flow and ratio calculations
    flow_scfh = oxygen_scfh + fuel_scfh;
    ratio_fto = fuel_scfh / oxygen_scfh;
    printf("Found flow rates\n  O2 (SCFH): %f\n  FG (SCFH): %f\n", oxygen_scfh, fuel_scfh);

    printf("Measuring temperatures..");
    // Collect raw thermocouple voltages
    start_data_stream(dconf,0,-1);
    read_data_stream(dconf,0,buffer);
    stop_data_stream(dconf,0);

    // Get the approximate ambient temperature
    LJM_eReadName(dconf[0].handle, "TEMPERATURE_AIR_K", &Tamb);

    // Average the tiny voltages and convert to temperature
    for(jj=0; jj<4; jj++){
        for(ii=0; ii<dconf[0].nsample; ii++)
            V[jj] += buffer[ii*4+jj];
        V[jj]/=dconf[0].nsample;
        LJM_TCVoltsToTemp(LJM_ttK, V[jj], Tamb, &T[jj]);
        T[jj] -= 273.15;
    }
    // Map the respective temperatures to their appropriate values
    plate_Thigh_C = T[0];
    plate_Tlow_C = T[1];
    cool_Thigh_C = T[2];
    cool_Tlow_C = T[3];

    // Update the calculated properties
    cool_Q_kW = coolant_heat(   air_gps, water_gps, 
                                cool_Tlow_C, cool_Thigh_C);
    plate_heat( plate_Tlow_C, plate_Thigh_C, 
                0.5*(cool_Thigh_C + cool_Tlow_C),
                &plate_Tpeak_C, &plate_Q_kW);

    printf( "done.\n"
            "Found Values:\n"
            "    Plate Thigh (C): %f\n"
            "     Plate Tlow (C): %f\n"
            "  Coolant Thigh (C): %f\n"
            "   Coolant Tlow (C): %f\n", 
            plate_Thigh_C, plate_Tlow_C, cool_Thigh_C, cool_Tlow_C);

    close_config(dconf, 0);

    // Move on to the IV measurement
    open_config(dconf, 1);
    upload_config(dconf, 1);

    // Write meta parameters for our measurements...
    put_meta_flt(dconf,1,"plate_Thigh_C",plate_Thigh_C);
    put_meta_flt(dconf,1,"plate_Tlow_C",plate_Tlow_C);
    put_meta_flt(dconf,1,"plate_Q_kW",plate_Q_kW);
    put_meta_flt(dconf,1,"plate_Tpeak_C",plate_Tpeak_C);
    put_meta_flt(dconf,1,"cool_Thigh_C",cool_Thigh_C);
    put_meta_flt(dconf,1,"cool_Tlow_C",cool_Tlow_C);
    put_meta_flt(dconf,1,"air_psig",air_psig);
    put_meta_flt(dconf,1,"water_gph",water_gph);
    put_meta_flt(dconf,1,"cool_Q_kW",cool_Q_kW);
    put_meta_flt(dconf,1,"standoff_in",standoff_in);
    put_meta_flt(dconf,1,"oxygen_scfh",oxygen_scfh);
    put_meta_flt(dconf,1,"fuel_scfh",fuel_scfh);
    put_meta_flt(dconf,1,"flow_scfh",flow_scfh);
    put_meta_flt(dconf,1,"ratio_fto",ratio_fto);
    
    // Calculate how many reads we'll need
    // Start by looking for the "periods" meta parameter
    if(get_meta_int(dconf, 1, "periods", &repeat))
        repeat = 10;    // default to 10 if it is missing
    // repeat now contains the number of periods.
    // Convert it to the number of repeats by the formula
    // repeats = period * Nperiods * samplehz / samples_per_read
    repeat *= (int) ceil(
        dconf[1].samplehz / ((double) dconf[1].nsample) /\
        dconf[1].aoch[0].frequency);

    if(repeat<=0){
        printf("WARNING: unexpected non-positive value for \"repeat\"; using 1.\n");
        repeat = 1;
    }else if(repeat > 1000){
        printf("WARNING: found huge value for \"repeat\"; using 1000.\n");
        repeat = 1000;
    }

    printf("Starting IV characteristic measurement.");
    start_file_stream(dconf,1,-1,DATA_FILE);
    for(ii=0; ii<repeat; ii++){
        printf(".");
        fflush(stdout);
        read_file_stream(dconf,1);
    }
    stop_file_stream(dconf,1);
    printf("done.\n");

    return 0;
}


//*****************************************************************************
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



//*****************************************************************************
double coolant_heat(const double air_mass,
                    const double water_mass,
                    double Tlow,
                    double Thigh){
    double  water_vap1_gps, // The water vapor flow rate at the inlet
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
    Tlow += 273.15;
    Thigh += 273.15;

    // Latent heats in J/g
    dh1 = latent(Tlow);
    dh2 = latent(Thigh);
    // d-less partial pressures
    xv1 = psat(Tlow)/ptot1;
    xv2 = psat(Thigh)/ptot2;

    // If we're below the triple point or above the critical point
    // Something is VERY VERY WRONG
    if(xv1<0 || xv2<0) return -1.;
    // If the temperature has exceeded the saturation temperature at this pressure
    // The estimate will be rough
    if(xv1 >= 1.) water_vap1_gps = water_mass;
    else water_vap1_gps = air_mass * (ww / wa) * xv1 / (1. - xv1);

    if(xv2 >= 1.) water_vap2_gps = water_mass;
    else water_vap2_gps = air_mass * (ww / wa) * xv2 / (1. - xv2);

    // Check to see if all the water is vapor.  This can happen at low water 
    // flow rates
    if(water_vap1_gps > water_mass) water_vap1_gps = water_mass;
    if(water_vap2_gps > water_gps) water_vap2_gps = water_mass;

    Q = (air_mass*cpa + water_mass*cpw)*(Thigh - Tlow) + \
        water_vap2_gps * dh2 - water_vap1_gps * dh1;

    // Convert to kJ
    return .001 * Q;
}


//*****************************************************************************
void plate_heat(const double Tlow,
                    const double Thigh,
                    const double Tcool,
                    double* Tpeak,
                    double* heat){
    double dT, n, th, tl;
    // Nominal temperature drop across the plate
    dT = 1.4556 * (Thigh - Tlow);
    // dimensionless high and low temperatures
    th = (Thigh - Tcool)/dT;
    tl = (Tlow - Tcool)/dT;
    // dimensionless plate conductivity
    n = 1. + (1.894*Tlow - 1.207*Thigh)/(1.0032*Thigh - 1.0005*Tlow);
    *Tpeak = (3.903 + n)*dT + Tcool;
    *heat = .0042 * dT;
}
