/* PSAT.H
Chris Martin (c) 2016

Saturation pressure of water and latent heat of vaporization
*/

#ifndef __PSAT
#define __PSAT

#include <math.h>   // for POW and SQRT

// Define some handy reference constants in MPa and K
// These are the critical and triple points for water

#define PSAT_CRIT_P     22.064
#define PSAT_CRIT_T     647.096
#define PSAT_TRIP_P     .000611657
#define PSAT_TRIP_T     273.16
// Specific heat of liquid water in kJ/kg/K
#define PSAT_CP         4.1318


/* PSAT
A function for calculating the saturation (partial) pressure of water using the 
IF-97 formulation (pgs 33-34) from the IAPWS (iapws.org).

    P = psat(T);

Calculates the partial pressure in MPa given the mixture temperature in K.  If 
T is below the tripple point, PSAT will return -2.  If T is above the critical 
point, PSAT will return -1.

PSAT() has been validated against the test conditions provided in IF-97 pg 34
table 35.
*/
double psat(const double T){
    // Intermediate variables
    double A,B,C,t,P;
    // Coefficient Array
    const double n[] = {0.11670521452767e4,
                        -0.72421316703206e6,
                        -0.17073846940092e2,
                        0.12020824702470e5,
                        -0.32325550322333e7,
                        0.14915108613530e2,
                        -0.48232657361591e4,
                        0.40511340542057e6,
                        -0.23855557567849,
                        0.65017534844798e3};

    if(T>PSAT_CRIT_T) return -1.;
    else if(T<PSAT_TRIP_T) return -2.;

    // OK, time to get calculating.
    // Start with the dimensionless temperature
    // IF-97 uses reference T,P at 1K, 1MPa, so our P and T values are
    // already "non-dimensional"
    // IF-97 uses coefficient indices that start at 1, so ours will be offset
    // by 1.
    // The equations are referenced by IF-97 equation numbers

    t = T + n[8]/(T - n[9]);        // eqn. 29b
    A = n[1] + t*(n[0] + t);        // eqn. 30...
    B = n[4] + t*(n[3] + t*n[2]);
    C = n[7] + t*(n[6] + t*n[5]);
    P = 2*C / (-B + sqrt(B*B-4*A*C));
    return P*P*P*P;
}




/* LATENT
Calculates the latent enthalpy of vaporization for water given the temperature
in Kelvin.  The code is based on a quadratic fit for the properties of steam
between 300 and 500 Kelvin.  If dh is the enthalpy of vaporization and T is the
temperature, the fit is of the form

    T = D2 dh**2 + D1 dh + D0

This provides an approximation for dh better than .05% accurate in that ragnge.  
The calculation of dh is of the form

    dh = (sqrt(b**2 + 4*t) - b)/2.

where

    t = (T - D0)/D2
    b = D1/D2
*/
double latent(const double T){
    static const double b = -2498.1238326967778;
    static const double D0 = 271.82180288060158;
    static const double D2 = -0.18598945532374373e-3;
    double t;

    t = (T - D2)/D0;
    return (sqrt(b*b + 4.*t) - b)/2.;
}



#endif
