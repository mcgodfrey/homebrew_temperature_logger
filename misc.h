#ifndef MISC_H
#define MISC_H

#include "DallasTemperature.h"
#include "RTClib.h"
#include "defs.h"
#include <SD.h>

#define NUM_KNOWN_SENSORS 4
//lists of known sensor addresses and corresponding names (4 chars long)
static DeviceAddress known_addrs[NUM_KNOWN_SENSORS] = {{0x28, 0xFF, 0x25, 0x2D, 0x56, 0x14, 0x03, 0x10}, {0x28, 0xFF, 0xA1, 0x63, 0x56, 0x14, 0x03, 0x31}, {0x28, 0xFF, 0xF4, 0x4D, 0x56, 0x14, 0x03, 0x13}, {0x28, 0xFF, 0x6C, 0xB6, 0x55, 0x14, 0x03, 0x9B}};
static char probe_names[NUM_KNOWN_SENSORS][PROBE_NAME_LEN] = {"BEER","AMBI","AAAA","BBBB"};

void printAddress(DeviceAddress deviceAddress);
void date2str(DateTime t, char *buffer);
void time2str(DateTime t, char *buffer);
void datetime2str(DateTime t, char *buffer);
int ipow(int base, int exponent);
void num2char(int num, char *buffer, byte n);
byte dump_log(char *filename);
void get_probe_name(DeviceAddress addr, char* buff);


#endif //MISC_H
