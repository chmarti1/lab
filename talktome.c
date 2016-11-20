#include "lgas.h"
#include <stdio.h>
#include <unistd.h>


int main(void){
    double o2, fg;
    //printf("%d\n",init_u12());
    //sleep(5);
    printf("%d\n",get_gas(&o2, &fg));
    printf("In SCFH: %f, %f\n",o2,fg);
    printf("In MOLES/SEC: %f, %f\n",convert_to_moles(o2), convert_to_moles(fg));
    printf("In GRAMS/SEC%f, %f\n",convert_to_mass(o2,LGAS_O2_MW), convert_to_mass(fg,LGAS_FG_MW));
    return 0;
}
