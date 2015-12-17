#include <stdio.h>
#include <string.h>
#include "dtostrf.h"
#include "itoa.h"

char *dtostrf(float f, int width, int decimals, char *result)
{
    char widths[3];
    char decimalss[3];  
    char format[100];
    itoa(width,widths,10);
    itoa(decimals,decimalss,10);
    strcpy(format,"%");
    strcat(format,widths);
    strcat(format,".");
    strcat(format,decimalss);
    strcat(format,"f");
  
    sprintf(result,format,f);
    return result;
}
