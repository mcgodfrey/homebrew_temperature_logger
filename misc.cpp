#include "misc.h"


/*
 * Formats and prints an address over serial
 */
 void printAddress(DeviceAddress deviceAddress){
  for (uint8_t i = 0; i < 8; i++)  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

/*
 * Formats the date/time in a nice string format
 */
void datetime2str(DateTime t, char *buffer){
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
 * Formats the date in a nice string format
 */
void date2str(DateTime t, char *buffer){
  num2char(t.year(),buffer,4);
  buffer[4]='/';
  num2char(t.month(),&buffer[5],2);
  buffer[7]='/';
  num2char(t.day(),&buffer[8],2);
  buffer[10]='\0';
}

/*
 * Formats the time in a nice string format
 */
void time2str(DateTime t, char *buffer){
  num2char(t.hour(),&buffer[0],2);
  buffer[2]=':';
  num2char(t.minute(),&buffer[3],2);
  buffer[5]=':';
  num2char(t.second(),&buffer[6],2);
  buffer[8]='\0';
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
 * Note that this will badly hang the UI
 */
byte dump_log(char *filename){
  Serial.print(filename);
  File f = SD.open(filename);
  if(f){
    while (f.available()) {
      Serial.write(f.read());
    }
    f.close();
  }else{
    return(ERROR_SD_MISC);
  }
  return(ERROR_NONE);
}

/*
 * Generates the 4 character string probe name
 * If it is a known probe, then use the assigned name
 * Otherwise just return the last 4 hexadeimal digits
 *   of the (unique) address
 */
void get_probe_name(DeviceAddress addr, char* buff) {
  boolean match;
  byte a;
  for(a=0; a<NUM_KNOWN_SENSORS;a++){
    match = true;
    for(byte b=0; b<8; b++){
      if(addr[b] != known_addrs[a][b]){
        match = false;
        continue;
      }
    }
    if (match){
      break;
    }
  }
  if (match){
    for(byte b=0; b<PROBE_NAME_LEN; b++){
      buff[b]=probe_names[a][b];
    }
  } else {
    //TODO:
    //This should return the last 4 hex digits in the
    //device address
    buff[0]='U';
    buff[1]='N';
    buff[2]='K';
    buff[3]='N';
    buff[4]='\0';
  }
}


