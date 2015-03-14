#include "misc.h"



// function to print a device address
void printAddress(DeviceAddress deviceAddress){
  for (uint8_t i = 0; i < 8; i++)  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}



/*
 * Formats the date in a nice string format
 */
void calc_date(DateTime t, char *buffer){
  num2char(t.year(),buffer,4);
  buffer[4]='/';
  num2char(t.month(),&buffer[5],2);
  buffer[7]='/';
  num2char(t.day(),&buffer[8],2);
  buffer[10]=' ';
  num2char(t.hour(),&buffer[11],2);
  buffer[13]=':';
  num2char(t.minute(),&buffer[14],2);
  buffer[16]=':';
  num2char(t.second(),&buffer[17],2);
  buffer[19]='\0';
}

/*
 * calculate integer power
 */
int ipow(int base, int exponent){
    int result = 1;
    while(exponent){
        if (exponent & 1){
            result *= base;
        }
        exponent >>= 1;
        base *= base;
    }
    return result;
}

/*
 * converts integer to string, saves "n" LSD to buffer
 */
void num2char(int num, char *buffer, byte n){
  byte i;
  for(i=0;i<n;i++){
    int div=ipow(10,(n-1-i));
    buffer[i]=(num/div)%10+'0';
  }
  buffer[n]='\0';
}



/*
 * Dumps the contents of the current log file over serial
 */
void dump_log(char *filename){
  File f = SD.open(filename);
  if(f){
    while (f.available()) {
      Serial.write(f.read());
    }
    f.close();
  }
}


char lookup_probe_name(DeviceAddress adr){
  for(byte a=0; a<NUM_KNOWN_SENSORS;a++){
    boolean match = true;
     for(byte b=0; b<8; b++){
       if(adr[b] != known_adrs[a][b]){
         match = false;
         continue;
       }
     }
     if(match){
       return('A'+a);
     }
  }
  return 0;
}
