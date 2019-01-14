#include "ldisplay.h"       // For the display helper functions
#include "lgas.h"           // For gas measurements from the U12
#include <unistd.h>




int main(void){
    char go_f = 1;

    double o2_scfh, o2_gps;
    double fg_scfh, fg_gps;
    double total_scfh, total_gps;
    double ratio_scfh, ratio_gps;

	if(zero_gas()){
		printf("Zeroing failed.\n");
		return -1;
	}

    setup_keypress();
    while(go_f){
        // Quit?
        if(keypress() && getchar()=='q')
            go_f = 0;

        // Get gas flow rates
        get_gas(&o2_scfh, &fg_scfh);
        o2_gps = convert_to_mass(o2_scfh, LGAS_O2_MW);
        fg_gps = convert_to_mass(fg_scfh, LGAS_FG_MW);
        // Update the flow and ratio calculations
        total_scfh = o2_scfh + fg_scfh;
        total_gps = o2_gps + fg_gps;

        ratio_scfh = -1;
        ratio_gps = -1;
        // Only calculate the ratio if the oxygen flow rate is nonzero
        if(o2_scfh>0.){
            ratio_scfh = fg_scfh / o2_scfh;
            ratio_gps = fg_gps / o2_gps;
        }
        
        // Update the output
        clear_terminal();

        printf("%14s: %14s %14s\n", "Gas", "Vol. (scfh)", "Mass (gps)");
        printf("%14s: %14f %14f\n", "Oxygen", o2_scfh, fg_gps);
        printf("%14s: %14f %14f\n", "Fuel Gas", o2_scfh, fg_gps);
        printf("%14s: %14f %14f\n", "Total", total_scfh, total_gps);
        printf("%14s: %14f %14f\n", "Ratio", ratio_scfh, ratio_gps);
    }

    finish_keypress();
    return 0;
}
