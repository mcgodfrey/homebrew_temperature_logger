/*
 
 * To program pro-mini
 green -> RX
 white -> TX
 hit reset button just as it starts uploading
 
 
 TODO:
 * program will hang if dumping a large file over serial. make it do it in the background?
 * lcd backlight and timeout (ie. auto turn on with button press then timeout after ~5 mins)
 * indicator LEDs, eg. sd card inserted, taking measurement
 * allocate temp_array based on number of sensors, rather than defining a MAX_SERNSOR size array at the beginning.
 * 
 */

#include <LiquidCrystal.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "RTClib.h"
#include "display.h"
#include "Switch.h"
#include "MyTimer.h"
#include "misc.h"
#include "defs.h"



//prototypes
enum State_t {DISP_TEMP, MEAS_INTERVAL_SELECT, DATETIME, LOG_SELECT, DUMP_LOG, ERR};
byte log_temps(void);
byte init_sd_card(void);
//callbacks
void measure_temps(void);
void conversion_complete(void);
void timeout_display(void);
void next_sensor_display(void);

//state variables
State_t state;
float temp_array[MAX_SENSORS];
char probe_name_array[MAX_SENSORS][PROBE_NAME_LEN];
DeviceAddress sensor_addresses[MAX_SENSORS];
byte num_sensors;
boolean do_log;
byte meas_interval;
char date_str[20];

//Object setup
//Buttons
Switch button_up(PB_U, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Switch button_down(PB_D, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Switch button_select(PB_S, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
//LCD
Display disp(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
//Timers
MyTimer measTimer(DEFAULT_MEAS_INTERVAL, measure_temps, false, false);
MyTimer conversionTimer(CONVERSION_TIME, conversion_complete, true, true);
MyTimer display_timeout(TIMEOUT_PERIOD, timeout_display, true, false);
//Devices/interfaces
RTC_DS1307 rtc;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//other variables
char filename[12] = "temp";
byte sensor_display = 0;
byte error_code = ERROR_NONE;
byte sd_present = 0;

/*
 *
 */
void setup() {
  delay(2000);
  Serial.begin(BAUD_RATE);
  Wire.begin();
  
  //Init temperature sensors
  sensors.begin();
  num_sensors = sensors.getDeviceCount();
    
  //init RTC
  rtc.begin();
  if (!rtc.isrunning()) {
    state = ERR;
    error_code = ERROR_RTC;
    return;
  }
  
  //sd init
  do_log = DEFAULT_DO_LOG;
  if(error_code = init_sd_card()){
    sd_present = 0;
  }else{
    sd_present = 1;
  }
  
  
  //Take an initial temperature reading and move to the initial state
  state = DISP_TEMP;
  meas_interval = DEFAULT_MEAS_INTERVAL;
  measure_temps();
  delay(CONVERSION_TIME);
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
  //poll the timers
  //if any of these have expired then their callback will automatically run.
  conversionTimer.run();
  measTimer.run();
  display_timeout.run();
  
  //poll the buttons
  button_up.poll();
  button_down.poll();
  button_select.poll();
  
  //reset the display timeout timer if a button was pressed
  if(button_up.pushed() || button_down.pushed() || button_select.pushed()){
    display_timeout.restart();
  }

  //****************************************
  //Main state machine for UI
  switch (state) {
    case DISP_TEMP:
      if(button_select.pushed()){
		  //move to the next state
        disp.meas_interval_select(meas_interval);
        state = MEAS_INTERVAL_SELECT;
      }
      break;
    case MEAS_INTERVAL_SELECT:
      if(button_up.pushed()){
        meas_interval = meas_interval < MAX_INTERVAL-INTERVAL_INCR ? meas_interval+INTERVAL_INCR : MAX_INTERVAL;
        disp.meas_interval_select(meas_interval);
        measTimer.setInterval(meas_interval);
      }else if(button_down.pushed()){
        meas_interval = meas_interval > 2*INTERVAL_INCR ? meas_interval-INTERVAL_INCR : INTERVAL_INCR;
        disp.meas_interval_select(meas_interval);
        measTimer.setInterval(meas_interval);
      }else if(button_select.pushed()){
        //move to the next state
        disp.disp_time(rtc);
        state=DATETIME;
       }
      break;
    case DATETIME:
      if(button_select.pushed()){
        //move to the next state
        if(sd_present){
          disp.log_selection(do_log);
          state=LOG_SELECT;
        }else{
          disp.all_temps(temp_array, probe_name_array, num_sensors);
          state = DISP_TEMP;
        }      
      }
      break;
    case LOG_SELECT:
      if(button_up.pushed() || button_down.pushed()){
        do_log = !do_log;
        disp.log_selection(do_log);
      }else if(button_select.pushed()){
        //move to the next state
        disp.dump_log();
        state = DUMP_LOG;
      }
      break;
    case DUMP_LOG:
      if(button_up.pushed() || button_down.pushed()){
        dump_log(filename);
        disp.all_temps(temp_array, probe_name_array, num_sensors);
        state=DISP_TEMP;
      }else if(button_select.pushed()){
        //move to the next state
        disp.all_temps(temp_array, probe_name_array, num_sensors);
        state = DISP_TEMP;
      }
      break;
    case ERR:
      disp.error(error_code);
      while(1);
      break;
    default:
      state = ERR;
      error_code = ERROR_UNKNOWN_STATE;
      break;
  }
}








/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/*
 * Triggers a temperature measurement for all the attached probes
 * Note that the temperature measurement is asynchronous and runs in the background
 * This is the callback function for the measurement timer
 */
void measure_temps(){
  //get the current timestamp
  DateTime t = rtc.now();
  datetime2str(t, date_str);
  
  //trigger a temperature measurement
  sensors.begin();
  sensors.setResolution(TEMPERATURE_PRECISION);
  num_sensors = sensors.getDeviceCount();
  for(byte i = 0; i < num_sensors; i++){
    sensors.getAddress(sensor_addresses[i], i);
  }
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  
  //Set up timer for temperature conversion to complete
  conversionTimer.restart();
  return;  
}


/*
 * Reads the temperature of all the attached probes
 * Prints to serial and optionally saves to SD
 * This is the callback function for the measurement timer
 */
void conversion_complete(){
  Serial.println(""); Serial.println("");
  Serial.println(date_str);
  
  //Read in the temperatures and print to serial port
  byte sensor_num = 0;
  for(int i = 0; i<num_sensors; i++){
    //Serial.print("Looking for ");printAddress(sensor_addresses[i]);Serial.println("");
    
    //check that the probe is still present
    if(sensors.isConnected(sensor_addresses[i])){
      temp_array[sensor_num] = sensors.getTempC(sensor_addresses[i]);
      get_probe_name(sensor_addresses[i], probe_name_array[sensor_num]);
      Serial.print(probe_name_array[sensor_num]); Serial.print(" - ");
      Serial.print(temp_array[sensor_num],1); Serial.println("degC");
      sensor_num++;
    }
  }
  num_sensors = sensor_num;

  //update the display
  if(state==DISP_TEMP){
    disp.all_temps(temp_array, probe_name_array, num_sensors);
  }
  //save to SD
  if(do_log && sd_present){
    log_temps(date_str);
  }
  return;  
}



/*
 * Logs current temperature data to SD card.
 * Note that the global variable "filename" must be initialised
 * File format is:
 * datestring,sensor0_name,temp0,sensor1_name,temp1,sensor2_name,temp2
 * datestring,sensor0_name,temp0,sensor1_name,temp1,sensor2_name,temp2
 */
byte log_temps(char *date_str){
  File f = SD.open(filename, FILE_WRITE);
  if(f) {
    f.print(date_str);
    for(byte a=0; a<num_sensors; a++){
      f.print(",");
      f.print(probe_name_array[a]);
      f.print(",");
      f.print(temp_array[a]);
    }
    f.println(""); 
    f.close();
  }else{
    state=ERR;
    error_code = ERROR_LOGGING;
  }
  return(ERROR_NONE);  
}



/*
 * Opens comms with the SD card
 * It generates a new filename and saves this to the gloabal variable
*/
byte init_sd_card(){
  // see if the card is present and can be initialized:
  if (!SD.begin(SD_SS)){
    return(ERROR_NO_SD);
  }

  //generate the filename. format is "tempxxx.txt" where xxx is a 
  //3 digit number which is incremented each time a new measurement  starts.
  //ie. it keeps incrementing the counter until it finds a file which doesn't exist yet
  for(byte a=0;a<255;a++){
    num2char(a,&filename[4],3);
    filename[7] = '.';
    filename[8]='t';
    filename[9]='x';
    filename[10]='t';
    filename[11]='\0';
    if(!SD.exists(filename)){
      break;
    }
  }
  Serial.print(F("log filename: "));
  Serial.print(filename);
  Serial.println("");
  return(ERROR_NONE);
}



/*
 * Callback function when the display timeout timer expires
 */
void timeout_display(){
  disp.all_temps(temp_array, probe_name_array, num_sensors);
  state = DISP_TEMP;
}




