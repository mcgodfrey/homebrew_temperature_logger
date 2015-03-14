#ifndef MISC
#define MISC

#include "DallasTemperature.h"
#include "RTClib.h"
#include <SD.h>

#define NUM_KNOWN_SENSORS 2
static DeviceAddress known_adrs[NUM_KNOWN_SENSORS] = {{0x28, 0xFF, 0xF4, 0x4D, 0x56, 0x14, 0x03, 0x13}, {0x28, 0xFF, 0x6C, 0xB6, 0x55, 0x14, 0x03, 0x9B}};

void printAddress(DeviceAddress deviceAddress);
void calc_date(DateTime t, char *buffer);
int ipow(int base, int exponent);
void num2char(int num, char *buffer, byte n);
void dump_log(char *filename);
char lookup_probe_name(DeviceAddress adr);




#endif
