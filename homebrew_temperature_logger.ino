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
 * Change temperature reading to non-blocking so the UI doesn't freeze
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
enum State_t {DISP_TEMP, MEAS_INTERVAL_SELECT, LOG_SELECT, DUMP_LOG, ERR};
byte log_temps(void);
byte init_sd_card(void);
//callbacks
void measure_temps(void);
void conversion_complete(void);
void timeout_display(void);
void next_sensor_display(void);

//state variables
State_t state = DISP_TEMP;
float temp_array[MAX_SENSORS];
char probe_name_array[MAX_SENSORS][PROBE_NAME_LEN];
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
char filename[16] = "log/temp";
byte sensor_display = 0;
byte error_code = ERROR_NONE;
byte sd_present = 0;

/*
 *
 */
void setup() {
  Serial.begin(BAUD_RATE);
  Wire.begin();
  
  //Init temperature sensors
  sensors.begin();
  num_sensors = sensors.getDeviceCount();
  
  //init RTC
  #if RTC
  rtc.begin();
  if (!rtc.isrunning()) {
    state = ERR;
    error_code = ERROR_RTC;
    return;
  }
  #endif

  //sd init
  #if SD_PRESENT
  do_log = DEFAULT_DO_LOG;
  if(error_code = init_sd_card()){
    sd_present = 0;
  }else{
    sd_present = 1;
  }
  #else
  do_log = false;
  sd_present = 0;
  #endif
  
  
  //other init
  meas_interval = DEFAULT_MEAS_INTERVAL;
  
  
  //Take an initial temperature reading and move to the initial state
  measure_temps();
  disp.all_temps(temp_array, probe_name_array, num_sensors);
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
  #if RTC
  DateTime t = rtc.now();
  calc_date(t, date_str);
  #else
  for(int i = 0; i < 19; i++) {
    date_str[i]='X';
  }
  date_str[19]=0;
  #endif
  
  //trigger a temperature measurement
  sensors.begin();
  num_sensors = sensors.getDeviceCount();
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
  Serial.println("");
  Serial.println(date_str);
  DeviceAddress probeAddr;
  byte sensor_num=0;
  //Read in the temperatures and print to serial port
  for(int i = 0; i<num_sensors; i++){
    //check that the probe is still present
    if(sensors.getAddress(probeAddr, i)){
      temp_array[sensor_num] = sensors.getTempC(probeAddr);
      get_probe_name(probeAddr, probe_name_array[sensor_num]);
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
    if(error_code = log_temps(date_str)){
      state = ERR;
    }
  }
  return;  
}



/*
 * Logs current temperature data to SD card.
 Note that the global variable "filename" must be initialised
 */
byte log_temps(char *date_str){
  //if the file doesn't exist yet, then we need to write some header information
  boolean write_header = false;
  if(!SD.exists(filename)){
    write_header=true;
  }
  File f = SD.open(filename, FILE_WRITE);
  if(f) {
    //First check if we need to write header info to the file (if this is the first entry in the log)
    if(write_header){
      f.print("time");
      for(byte a=0; a<num_sensors; a++){
        f.print(",");
        DeviceAddress adr;
        if(sensors.getAddress(adr, a)){
          char name = lookup_probe_name(adr);
          if(name){
            f.print(name);
          }else{
            //If I don't know the unique adr for this sensor, then just save the adr
            for (uint8_t i = 0; i < 8; i++){
              if (adr[i] < 16){
                f.print("0"); 
              }
              f.print(adr[i], HEX);
            }
          }
        }
      }
      f.println("");
    }
    
    //Now save the actual temp data
    f.print(date_str);
    for(byte a=0; a<num_sensors; a++){
      f.print(",");
      f.print(temp_array[a]);
    }
    f.println(""); 
    f.close();
  }else{
    state=ERR;
    return(ERROR_LOGGING);
  }
  return(ERROR_NONE);  
}



/*
  Opens comms with the SD card
  
*/
byte init_sd_card(){
  // see if the card is present and can be initialized:
  if (!SD.begin(SD_SS)){
    return(ERROR_NO_SD);
  }

  if(!SD.exists("log")){
    if(!SD.mkdir("log")){
      do_log=0;
      return(ERROR_SD_MKDIR);
    }
  }

  //generate the filename. format is "log/tmpxxx.txt" where xxx is a 
  //3 digit number which is incremented each time a new measurement  starts.
  //ie. it keeps incrementing the counter until it finds a file which doesn't exist yet
  for(int a=0;a<1000;a++){
    num2char(a,&filename[8],3);
    filename[11] = '.';
    filename[12]='t';
    filename[13]='x';
    filename[14]='t';
    filename[15]='\0';
    if(!SD.exists(filename)){
      break;
    }
  }
  Serial.print(F("log filename: "));Serial.print(filename);Serial.println("");
  return(ERROR_NONE);
}



/*
 * Callback function when the display timeout timer expires
 */
void timeout_display(){
  disp.all_temps(temp_array, probe_name_array, num_sensors);
  state = DISP_TEMP;
}



