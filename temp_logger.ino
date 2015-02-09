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

#include <MyLiquidCrystal.h>
#include <EEPROM.h>
#include <SD.h>
#include <Wire.h>
#include "MyRTC.h"
#include "display.h"
#include "PT100.h"
#include "Switch.h"
#include "MyTimer.h"

#define DEBUG 0


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
void EEPROM_write_float(int adr, float val);
float EEPROM_read_float(int adr);
void init_state_from_eeprom(void);
void print_settings(void);
void init_sd_card(void);
void timeout_display(void);


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
//MyTimer display_timeout(TIMEOUT_PERIOD, timeout_display);
RTC_DS1307 rtc;


//other variables
byte meas_timer_id;
byte timeout_id;
int counter=0;
char filename[20];
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
#if DEBUG
  Serial.println("start initialisation");
#endif
  
  init_state_from_eeprom();
  print_settings();
  init_sd_card();
  
  measTimer.setInterval(meas_interval);
  rtc.begin();

#if DEBUG
  Serial.println("Initialisation complete");
#endif
  disp.temps(temp_array, num_sensors);
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
  
  button_up.poll();
  button_down.poll();
  button_select.poll();
  
  if(button_up.pushed() || button_down.pushed() || button_select.pushed()){
#if DEBUG
    Serial.println("restarting timeout timer");
#endif
    //timer.restartTimer(timeout_id);
    //timer.enable(timeout_id);
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
        //disp.sensor_cal()
        disp.meas_interval_select(meas_interval);
        state=MEAS_INTERVAL_SELECT;
      }
      break;
    case MEAS_INTERVAL_SELECT:
      if(button_up.pushed() && meas_interval < MAX_INTERVAL){
        meas_interval+=1000;
        EEPROM.write(adr_meas_interval, meas_interval>>8);
        EEPROM.write(adr_meas_interval+1, meas_interval | 0xFF);
        disp.meas_interval_select(meas_interval);
        measTimer.setInterval(meas_interval);
      }else if(button_down.pushed()){
        meas_interval = meas_interval > 2000 ? meas_interval-1000 : 1000;
        EEPROM.write(adr_meas_interval, meas_interval>>8);
        EEPROM.write(adr_meas_interval+1, meas_interval | 0xFF);
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
  #if DEBUG
  Serial.print("logging temperature to ");Serial.println(filename);
  #endif
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
#if DEBUG
    Serial.println("Found config data in EEPROM");
#endif
    //Serial.println("loading it in now");
    num_sensors = EEPROM.read(adr_num_sensors);
    do_log = EEPROM.read(adr_do_log);
    meas_interval = (EEPROM.read(adr_meas_interval))<<8  + EEPROM.read(adr_meas_interval+1);
    for(char a=0; a<MAX_SENSORS; a++){
      float sens_a = EEPROM_read_float(adr_sens_cal+(2*a)*sizeof(float));
      float sens_b = EEPROM_read_float(adr_sens_cal+(2*a+1)*sizeof(float));
      sensors[a].calibrate(sens_a, sens_b);
    }
  }else{
#if DEBUG
    Serial.println("No config data in the EEPROM");
    Serial.println("Write default parameters now");
#endif
    EEPROM.write(adr_id, EEPROM_ID);
    num_sensors = DEFAULT_NUM_SENSORS;
    EEPROM.write(adr_num_sensors,num_sensors);
    do_log = DEFAULT_DO_LOG;
    EEPROM.write(adr_do_log, do_log);
    meas_interval = DEFAULT_MEAS_INTERVAL;
    EEPROM.write(adr_meas_interval, meas_interval>>8);
    EEPROM.write(adr_meas_interval+1, meas_interval | 0xFF);
    for(char a=0; a<MAX_SENSORS; a++){
      sensors[a].calibrate(0.2,10);
      EEPROM_write_float(adr_sens_cal+(2*a)*sizeof(float),0.2);
      EEPROM_write_float(adr_sens_cal+(2*a+1)*sizeof(float),10.0);
    }
  }
}

void print_settings(){
#if DEBUG
  Serial.print("Num sensors: ");Serial.println(num_sensors);
  Serial.print("do log?: ");Serial.println(do_log);
  Serial.print("Measure Interval: ");Serial.println(meas_interval);
  for(int i=0;i<MAX_SENSORS;i++){
    Serial.print("sensor "); Serial.println(i); 
    Serial.print(" a = "); Serial.println(sensors[i].get_cal_a());
    Serial.print(" b = "); Serial.println(sensors[i].get_cal_b());
  }
#endif
}


void init_sd_card(){
  // see if the card is present and can be initialized:
  if (!SD.begin(12)) {
#if DEBUG
    Serial.println("Card failed, or not present");
#endif
    return;
  }

  if(!SD.exists("log")){
#if DEBUG
    Serial.println("Didn't find log directory. Creating it now");
#endif
    if(SD.mkdir("log")){
#if DEBUG      
      Serial.println(" Success");
#endif
    }else{
#if DEBUG
      Serial.println(" failed!");
#endif
return;
    }
  }

  String base_str = "log/temp";
  while(1){
    String tmp_filename = base_str + random(999) + ".txt";
    tmp_filename.toCharArray(filename,20);
    if(SD.exists(filename)){
#if DEBUG      
      Serial.println("Filename already exists:");
      Serial.println("  " + tmp_filename);
#endif
    }else{
      break;
    }
  }

#if DEBUG
  Serial.println("card initialised.");
  Serial.print("Filename = ");Serial.println(filename);
#endif
}


void EEPROM_write_float(int adr, float value){
   byte* p = (byte*)(void*)&value;
   for (int i = 0; i < sizeof(value); i++)
       EEPROM.write(adr++, *p++);
}

float EEPROM_read_float(int adr){
  float value = 0.0;
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(adr++);
  return value;
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
