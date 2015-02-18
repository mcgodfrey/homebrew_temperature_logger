#ifndef MISC
#define MISC

#include "DallasTemperature.h"
#include "RTClib.h"
#include <SD.h>

void printAddress(DeviceAddress deviceAddress);
void calc_date(DateTime t, char *buffer);
int ipow(int base, int exponent);
void num2char(int num, char *buffer, byte n);
void dump_log(char *filename);




#endif
