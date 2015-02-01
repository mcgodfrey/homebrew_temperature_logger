/*
   LCD circuit:
 * LCD RS pin to digital pin 7
 * LCD Enable pin to digital pin 6
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V

  Pushbuttons connected to gnd and with internal pullups enabled
 * Up button = D8
 * Down button = D9
 * Select button = D10
 
 TODO:
 * level shifter for SD
 * SD logging
 * dump SD over serial
 * timestamp - rtc module, manually track time, etc.
 * sensor calibration procedure
 * timeout to return to temperature screen
 * lcd backlight and timeout (ie. auto turn on with button press then timeout after ~5 mins)
 * multiplexer
 * enclosure
 *
 */

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <SD.h>
#include "display.h"
#include "PT100.h"
#include "Switch.h"
#include "SimpleTimer.h"

//definitions
#define MAX_SENSORS 4
#define DEBOUNCE_TIME 50
#define EEPROM_ID 47
#define DEFAULT_DO_LOG false
#define DEFAULT_NUM_SENSORS 1
#define DEFAULT_MEAS_INTERVAL 3000
#define MAX_INTERVAL 31000
enum State_t {DISP_TEMP, SENSOR_SELECT, MEAS_INTERVAL_SELECT, SENSOR_CAL, LOG_SELECT, ERROR};
void measure_temps(float *temp_array, char num_sensors);
void log_temps(float temp_array, char num_sensors);
void EEPROM_write_float(int adr, float val);
float EEPROM_read_float(int adr);
void init_state_from_eeprom(void);
void print_settings(void);
void init_sd_card(void);


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
Display disp(7, 6, 5, 4, 3, 2);
SimpleTimer timer;


//other variables
byte meas_timer_id;
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
  Serial.begin(9600);
  Serial.println("start initialisation");

  init_state_from_eeprom();
  print_settings();
  init_sd_card();
  
  meas_timer_id = timer.setInterval(meas_interval, measure_temps);
  
  Serial.println("Initialisation complete");
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
  
  timer.run();
  
  button_up.poll();
  button_down.poll();
  button_select.poll();
  
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
        timer.deleteTimer(meas_timer_id);
        meas_timer_id = timer.setInterval(meas_interval, measure_temps);
      }else if(button_down.pushed()){
        meas_interval = meas_interval > 2000 ? meas_interval-1000 : 1000;
        EEPROM.write(adr_meas_interval, meas_interval>>8);
        EEPROM.write(adr_meas_interval+1, meas_interval | 0xFF);
        disp.meas_interval_select(meas_interval);
        timer.deleteTimer(meas_timer_id);
        meas_timer_id = timer.setInterval(meas_interval, measure_temps);
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
    Serial.println("Found config data in the EEPROM");
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
    Serial.println("No config data in the EEPROM");
    Serial.println("I'm writing the default parameters now");
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
  if (!SD.begin(12)) {
    Serial.println("Card failed, or not present");
    return;
  }

  if(!SD.exists("logger")){
    Serial.println("Didn't find logger directory. Creating it now");
    if(SD.mkdir("logger")){
      Serial.println(" Success");
    }else{
      Serial.println(" failed!");
      return;
    }
  }

  while(1){
    String tmp_filename = "logger/temp" + random(999);
    tmp_filename += ".txt";
    tmp_filename.toCharArray(filename,20);
    if(SD.exists(filename)){
      Serial.println("Filename already exists:");
      Serial.println("  " + tmp_filename);
    }else{
      break;
    }
  }

  Serial.println("card initialised.");
  Serial.print("Filename = ");Serial.println(filename);
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
