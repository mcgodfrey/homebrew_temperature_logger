/*
   LCD circuit:
 * LCD RS pin to digital pin 7
 * LCD Enable pin to digital pin 6
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 12
 * LCD D7 pin to digital pin 11
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V

  Pushbuttons connected to gnd and with internal pullups enabled
 * Up button = D8
 * Down button = D9
 * Select button = D10
 
 * 5v/3v3 level shifter on SPI bus.
 * 50mA max on arduino 3v3 output which I don't think is enough
 * currently using discrete LM1117 3v3 regulator.
 *
 
 I2C (internal pullups)
 * SDA = D2
 * SCL = D3
 
 TODO:
 * fix error in meas_interval read and/or write in eeprom
 * program will hang if dumping a large file over serial. make it do it in the background?
 * timestamp - rtc module, manually track time, etc.
 * sensor calibration procedure
 * timeout to return to temperature screen - not working properly (at all...)
 * lcd backlight and timeout (ie. auto turn on with button press then timeout after ~5 mins)
 * multiplexer
 * enclosure
 * file format for SD card
 * Switch to new smaller sd library
 * remove eeprom library and save config on sd card instead (save program space)
 * Use 3v3 regulator on sd breakout board
 * trim difference amp
 * indicator LEDs, eg. sd card inserted, taking measurement
 * 
 * 
 */

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include "display.h"
#include "PT100.h"
#include "Switch.h"
#include "MyTimer.h"
#include "EEPROM_writeAnything.h"


//definitions
#define BAUD_RATE 9600
#define MAX_SENSORS 4
#define DEBOUNCE_TIME 50
#define EEPROM_ID 47
#define DEFAULT_DO_LOG false
#define DEFAULT_NUM_SENSORS 1
#define DEFAULT_MEAS_INTERVAL 3000
#define MAX_INTERVAL 31000
#define TIMEOUT_PERIOD 5000
enum State_t {DISP_TEMP, SENSOR_SELECT, MEAS_INTERVAL_SELECT, SENSOR_CAL, LOG_SELECT, DUMP_LOG, ERROR};
void measure_temps(void);
void log_temps(void);
void init_state_from_eeprom(void);
void print_settings(void);
void init_sd_card(void);
void timeout_display(void);
void calc_date(DateTime t, char *buffer);


//state variables
State_t state = DISP_TEMP;
float temp_array[MAX_SENSORS];
byte num_sensors;
boolean do_log;
int meas_interval;


//Object setup
Switch button_up(8, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Switch button_down(10, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Switch button_select(9, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
PT100 sensors[MAX_SENSORS] = {PT100(0), PT100(0), PT100(0), PT100(0)};
Display disp(7, 6, 5, 4, 12, 11);
MyTimer measTimer(10000, measure_temps, false);
MyTimer display_timeout(TIMEOUT_PERIOD, timeout_display, true);
RTC_DS1307 rtc;


//other variables
byte meas_timer_id;
byte timeout_id;
int counter=0;
char filename[16] = "log/temp";
//EEPROM addresses
#define adr_id 0
#define adr_num_sensors 1
#define adr_do_log 2
#define adr_meas_interval 3
#define adr_sens_cal 5



/*
 *
 */
void setup() {
  delay(5000);
  Serial.begin(BAUD_RATE);
  Serial.println("start initialisation");
  Wire.begin();
  
  init_state_from_eeprom();
  print_settings();
  init_sd_card();

  measTimer.setInterval(meas_interval);
  
  rtc.begin();
  if (!rtc.isrunning()) {
    Serial.println("RTC is not running");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }else{
    Serial.println("RTC running properly");
  }
  DateTime current_datetime = rtc.now();
  char datestr[20];
  calc_date(current_datetime,datestr);
  Serial.println(datestr);
    
  Serial.println("Initialisation complete");
  disp.temps(temp_array, num_sensors);
  measTimer.restart();
}




/*
 * State machine design for the display and user interface
 * The main screen is the temperature display which changes depending
 *  on the number of sensors which are connected
 * Then there are a number of other screens for changing the settings 
 *  which you toggle between and change with 3 pushbuttons.
 *
 * The temperature logging is controlled by a separate timer which triggers
 *  every meas_interval ms. This runs in the background and shouldn't affect
 *  or be affected by what is displayed on the screen
 * Changes in settings take place immediately for the measurement
 * 
 */
void loop() {
  
  measTimer.run();
  display_timeout.run();
  
  button_up.poll();
  button_down.poll();
  button_select.poll();
  
  if(button_up.pushed() || button_down.pushed() || button_select.pushed()){
    Serial.println("restarting timeout timer");
    display_timeout.restart();
  }
  
    
  switch (state) {
    case DISP_TEMP:
      if(button_select.pushed()){
        disp.sensor_select(num_sensors);
        state = SENSOR_SELECT;
      }
      break;
    case SENSOR_SELECT:
      if(button_up.pushed()){
        if(num_sensors < MAX_SENSORS){
          num_sensors++;
          EEPROM.write(adr_num_sensors, num_sensors);
          disp.sensor_select(num_sensors);
        }
      }else if(button_down.pushed()){
        if(num_sensors > 1){
          num_sensors--;
          EEPROM.write(adr_num_sensors, num_sensors);
          disp.sensor_select(num_sensors);
        }
      }else if(button_select.pushed()){
        disp.meas_interval_select(meas_interval);
        state=MEAS_INTERVAL_SELECT;
      }
      break;
    case MEAS_INTERVAL_SELECT:
      if(button_up.pushed() && meas_interval < MAX_INTERVAL){
        meas_interval+=1000;
        EEPROM_writeAnything(adr_meas_interval, meas_interval);
        disp.meas_interval_select(meas_interval);
        measTimer.setInterval(meas_interval);
      }else if(button_down.pushed()){
        meas_interval = meas_interval > 2000 ? meas_interval-1000 : 1000;
        EEPROM_writeAnything(adr_meas_interval, meas_interval);
        disp.meas_interval_select(meas_interval);
        measTimer.setInterval(meas_interval);
      }else if(button_select.pushed()){
        disp.log_selection(do_log);
        state=LOG_SELECT;
      }
      break;
    case LOG_SELECT:
      if(button_up.pushed() || button_down.pushed()){
        do_log = !do_log;
        EEPROM.write(adr_do_log, do_log);
        disp.log_selection(do_log);
      }else if(button_select.pushed()){
        disp.dump_log();
        state = DUMP_LOG;
      }
      break;
    case DUMP_LOG:
      if(button_up.pushed() || button_down.pushed()){
        dump_log();
        disp.temps(temp_array, num_sensors);
        state=DISP_TEMP;
      }else if(button_select.pushed()){
        disp.temps(temp_array, num_sensors);
        state = DISP_TEMP;
      }
      break;
    case ERROR:
      break;
    default:
      disp.error("bad_state");
      state = ERROR;
      break;
  }

}














/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
void measure_temps(){
  Serial.println("measuring temperature");
  DateTime t = rtc.now();
  char date_str[20];
  calc_date(t, date_str);
  Serial.println(date_str);
  for(int a=0; a<num_sensors; a++){
    temp_array[a] = sensors[a].get_temperature();
  }
  if(state==DISP_TEMP){
    disp.temps(temp_array, num_sensors);
  }
  if(do_log){
    log_temps();
  }
  return;  
}

void log_temps(){
  Serial.print("logging temperature to ");Serial.println(filename);
  File f = SD.open(filename, FILE_WRITE);
  if(f) {
    f.print(counter++);
    for(char a=0; a<num_sensors; a++){
      f.print(",");
      f.print(temp_array[a]);
    }
    f.println("");
  }else{
    Serial.println(" error opening file");
    return;  
  }
  f.close();
  return;  
}




void init_state_from_eeprom(){
  if(EEPROM.read(adr_id) == EEPROM_ID){
    Serial.println("Found config data in EEPROM");
    //Serial.println("loading it in now");
    num_sensors = EEPROM.read(adr_num_sensors);
    do_log = EEPROM.read(adr_do_log);
    EEPROM_readAnything(adr_meas_interval, meas_interval);
    for(char a=0; a<MAX_SENSORS; a++){
      float sens_a;
      EEPROM_readAnything(adr_sens_cal+(2*a)*sizeof(float), sens_a);
      float sens_b;
      EEPROM_readAnything(adr_sens_cal+(2*a+1)*sizeof(float), sens_b);
      sensors[a].calibrate(sens_a, sens_b);
    }
  }else{
    Serial.println("No config data in the EEPROM");
    Serial.println("Write default parameters now");
    EEPROM.write(adr_id, EEPROM_ID);
    num_sensors = DEFAULT_NUM_SENSORS;
    EEPROM.write(adr_num_sensors,num_sensors);
    do_log = DEFAULT_DO_LOG;
    EEPROM.write(adr_do_log, do_log);
    meas_interval = DEFAULT_MEAS_INTERVAL;
    EEPROM_writeAnything(adr_meas_interval, meas_interval);
    for(char a=0; a<MAX_SENSORS; a++){
      sensors[a].calibrate(0.2,10);
      EEPROM_writeAnything(adr_sens_cal+(2*a)*sizeof(float),0.2);
      EEPROM_writeAnything(adr_sens_cal+(2*a+1)*sizeof(float),10.0);
    }
  }
}


void print_settings(){
  Serial.print("Num sensors: ");Serial.println(num_sensors);
  Serial.print("do log?: ");Serial.println(do_log);
  Serial.print("Measure Interval: ");Serial.println(meas_interval);
  for(int i=0;i<MAX_SENSORS;i++){
    Serial.print("sensor "); Serial.println(i); 
    Serial.print(" a = "); Serial.println(sensors[i].get_cal_a());
    Serial.print(" b = "); Serial.println(sensors[i].get_cal_b());
  }
}


void init_sd_card(){
  // see if the card is present and can be initialized:
  if (!SD.begin(13)) {
    Serial.println("Card failed, or not present");
    return;
  }

  if(!SD.exists("log")){
    Serial.println("Didn't find log directory. Creating it now");
    if(SD.mkdir("log")){
      Serial.println(" Success");
    }else{
      Serial.println(" failed!");
      return;
    }
  }

  int counter = 0;
  while(1){
    char tmp[4];
    num2char(counter++,&filename[8],3);
    filename[11] = '.';
    filename[12]='t';
    filename[13]='x';
    filename[14]='t';
    filename[15]='\0';
    if(SD.exists(filename)){
      Serial.println("Filename already exists:");
      Serial.println("  "); Serial.println(filename);
    }else{
      break;
    }
  }
  Serial.println("card initialised.");
  Serial.print("Filename = ");Serial.println(filename);
}


void num2char(int num, char *buffer, byte n){
  byte i;
  for(i=0;i<n;i++){
    int div=ipow(10,(n-1-i));
    buffer[i]=(num/div)%10+'0';
  }
  buffer[n]='\0';
}

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

void dump_log(){
  File f = SD.open(filename);
  if(f){
    while (f.available()) {
      Serial.write(f.read());
    }
    f.close();
  }
}


void timeout_display(){
  disp.temps(temp_array, num_sensors);
  state = DISP_TEMP;
}
