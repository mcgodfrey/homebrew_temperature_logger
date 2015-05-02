/*
   LCD circuit:
 * LCD RS pin to digital pin 4
 * LCD Enable pin to digital pin 5
 * LCD D4 pin to digital pin 6
 * LCD D5 pin to digital pin 7
 * LCD D6 pin to digital pin 8
 * LCD D7 pin to digital pin 9
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V

  Pushbuttons connected to gnd and with internal pullups enabled
 * Up button = D10
 * Down button = D11
 * Select button = D12
 

 I2C (internal pullups)
 * SDA = D2
 * SCL = D3
 * For RTC
 
 Temperature sensors
 * Dallas oneWire sensors on pin 13
 
 SPI
 * For SD card
 * using hardware MOSI, MISO, SCK pins.
 * SS input pin not used (I think it is needed internally by the SPI library)
 * SS output is set to pin 2, but I don't actually use this as I only have 1 device
 * 5v/3v3 level shifter on SPI bus.
 * 3v3 comes from LM1117 on SD card board
 
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
 * ERROR CODES:
 * 
 * 
 * 
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


//misc definitions
#define BAUD_RATE 9600
#define SD_SS 2
#define MAX_SENSORS 4
#define DEBOUNCE_TIME 50
#define TIMEOUT_PERIOD 5
#define INTERVAL_INCR 1
#define MIN_INTERVAL 5
#define MAX_INTERVAL 255
#define TEMPERATURE_PRECISION 9
//default state
#define DEFAULT_DO_LOG false
#define DEFAULT_MEAS_INTERVAL 10
#define SENSOR_DISPLAY_TIME 10
//pins
#define LCD_RS 4
#define LCD_E 5
#define LCD_D4 6
#define LCD_D5 7
#define LCD_D6 8 
#define LCD_D7 9
#define PB_U 10
#define PB_D 11
#define PB_S 12
#define ONE_WIRE_BUS 13

//error codes
#define ERROR_NONE 0
#define ERROR_NO_PROBE 40
#define ERROR_RTC 50
#define ERROR_NO_SD 55
#define ERROR_SD_MKDIR 56
#define ERROR_LOGGING 58
#define ERROR_UNKNOWN_STATE 60


//prototypes
enum State_t {DISP_TEMP, MEAS_INTERVAL_SELECT, LOG_SELECT, DUMP_LOG, ERROR};
byte log_temps(void);
byte init_sd_card(void);
//callbacks
void measure_temps(void);
void timeout_display(void);
void next_sensor_display(void);

//state variables
State_t state = DISP_TEMP;
float temp_array[MAX_SENSORS];
byte num_sensors;
boolean do_log;
byte meas_interval;

//Object setup
Switch button_up(PB_U, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Switch button_down(PB_D, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Switch button_select(PB_S, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Display disp(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
MyTimer measTimer(DEFAULT_MEAS_INTERVAL, measure_temps, false);
MyTimer display_timeout(TIMEOUT_PERIOD, timeout_display, true);
//MyTimer sensor_display_timer(SENSOR_DISPLAY_TIME, next_sensor_display, false);
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
  delay(0);
  Serial.begin(BAUD_RATE);
  Wire.begin();
  
  //Init temperature sensors
  sensors.begin();
  num_sensors = sensors.getDeviceCount();
  if(num_sensors == 0){
    state = ERROR;
    error_code = ERROR_NO_PROBE;
    return;
  }
  
  //init RTC
  rtc.begin();
  if (!rtc.isrunning()) {
    state = ERROR;
    error_code = ERROR_RTC;
    return;
  }
  
  //other init
  do_log = DEFAULT_DO_LOG;
  meas_interval = DEFAULT_MEAS_INTERVAL;
  if(error_code = init_sd_card()){
    sd_present = 0;
  }else{
    sd_present = 1;
  }
  

  


  //Take an initial temperature reading and move to the initial state
  measure_temps();
  disp.all_temps(temp_array, num_sensors);
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
    display_timeout.restart();
  }
      
  switch (state) {
    case DISP_TEMP:
      if(button_select.pushed()){
        disp.meas_interval_select(meas_interval);
        state = MEAS_INTERVAL_SELECT;
      }
      break;
    case MEAS_INTERVAL_SELECT:
      if(button_up.pushed() && meas_interval < MAX_INTERVAL){
        meas_interval = meas_interval < MAX_INTERVAL-INTERVAL_INCR ? meas_interval+INTERVAL_INCR : MAX_INTERVAL;
        disp.meas_interval_select(meas_interval);
        measTimer.setInterval(meas_interval);
      }else if(button_down.pushed()){
        meas_interval = meas_interval > 2*INTERVAL_INCR ? meas_interval-INTERVAL_INCR : INTERVAL_INCR;
        disp.meas_interval_select(meas_interval);
        measTimer.setInterval(meas_interval);
      }else if(button_select.pushed()){
        if(sd_present){
          disp.log_selection(do_log);
          state=LOG_SELECT;
        }else{
          disp.all_temps(temp_array, num_sensors);
          state = DISP_TEMP;
        }
      }
      break;
    case LOG_SELECT:
      if(button_up.pushed() || button_down.pushed()){
        do_log = !do_log;
        disp.log_selection(do_log);
      }else if(button_select.pushed()){
        disp.dump_log();
        state = DUMP_LOG;
      }
      break;
    case DUMP_LOG:
      if(button_up.pushed() || button_down.pushed()){
        dump_log(filename);
        disp.all_temps(temp_array, num_sensors);
        state=DISP_TEMP;
      }else if(button_select.pushed()){
        disp.all_temps(temp_array, num_sensors);
        state = DISP_TEMP;
      }
      break;
    case ERROR:
      disp.error(error_code);
      while(1);
      break;
    default:
      state = ERROR;
      error_code = ERROR_UNKNOWN_STATE;
      break;
  }
}














/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/*
 * Measures temperature of each attached sensor and optionally logs them
 * This is the callback function for the measurement timer
 */
void measure_temps(){
  //get the current timestamp
  DateTime t = rtc.now();
  char date_str[20];
  calc_date(t, date_str);
  Serial.println(date_str);
  
  //measure temperatures and save to temp_array;
  //also print temperatures to serial for debug
  //note that this part hangs because the temperature read takes ~half a second per sensor
  //I need to make this non-blocking because the UI hangs
  sensors.requestTemperatures();
  for(int i=0;i<num_sensors; i++){
    DeviceAddress adr;
    if(sensors.getAddress(adr, i)){
      temp_array[i] = sensors.getTempC(adr);
      char name=lookup_probe_name(adr);
      Serial.print("  ");
      if(name){
        Serial.print(name);
      }else{
        printAddress(adr);
      }
      Serial.print(" - ");
      Serial.println(temp_array[i]);
    }
  }
  //update the display
  if(state==DISP_TEMP){
    disp.all_temps(temp_array, num_sensors);
  }
  //save to SD
  if(do_log && sd_present){
    if(error_code = log_temps(date_str)){
      state = ERROR;
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
    state=ERROR;
    return(ERROR_LOGGING);
  }
  return(ERROR_NONE);  
}




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
  disp.all_temps(temp_array, num_sensors);
  state = DISP_TEMP;
}



