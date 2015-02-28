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
 
 
 TODO:
 * program will hang if dumping a large file over serial. make it do it in the background?
 * lcd backlight and timeout (ie. auto turn on with button press then timeout after ~5 mins)
 * file format for SD card
 * indicator LEDs, eg. sd card inserted, taking measurement
 * allocate temp_array based on number of sensors, rather than defining a MAX_SERNSOR size array at the beginning.
 * add header line to log file with temp sensor unique adr.
 * Maybe save some temp sensor unique adrs. in EEPROM with a human readable name/number for each one.
 * Change temperature reading to non-blocking so the UI doesn't freeze
 * 
 */

#include <LiquidCrystal.h>
#include <SD.h>
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
#define ONE_WIRE_BUS 13
#define TEMPERATURE_PRECISION 9
//default state
#define DEFAULT_DO_LOG false
#define DEFAULT_MEAS_INTERVAL 10
#define SENSOR_DISPLAY_TIME 10


//prototypes
enum State_t {DISP_TEMP, MEAS_INTERVAL_SELECT, LOG_SELECT, DUMP_LOG, ERROR};
void log_temps(void);
void init_sd_card(void);
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
Switch button_up(8, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Switch button_down(10, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Switch button_select(9, INPUT_PULLUP, LOW, DEBOUNCE_TIME);
Display disp(7, 6, 5, 4, 12, 11);
MyTimer measTimer(DEFAULT_MEAS_INTERVAL, measure_temps, false);
MyTimer display_timeout(TIMEOUT_PERIOD, timeout_display, true);
MyTimer sensor_display_timer(SENSOR_DISPLAY_TIME, next_sensor_display, false);
RTC_DS1307 rtc;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//other variables
char filename[16] = "log/temp";
byte sensor_display = 0;



/*
 *
 */
void setup() {
  delay(5000);
  Serial.begin(BAUD_RATE);
  Wire.begin();
  
  //Init temperature sensors
  sensors.begin();
  num_sensors = sensors.getDeviceCount();

  //init RTC
  rtc.begin();
  if (!rtc.isrunning()) {
    //Serial.println("RTC not running");
  }
  
  //other init
  do_log = DEFAULT_DO_LOG;
  meas_interval = DEFAULT_MEAS_INTERVAL;
  init_sd_card();
  if(num_sensors == 1){
    //if there is only 1 sensor, then we don't need to loop through them to display them
    sensor_display_timer.disable();
  }

  //Take an initial temperature reading and move to the initial state
  measure_temps();
  disp.temps(temp_array, sensor_display);
  measTimer.restart();
  sensor_display_timer.restart();
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
  sensor_display_timer.run();
  
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
      }else if(button_up.pushed()){
        next_sensor_display();
        sensor_display_timer.restart();
      }else if(button_down.pushed()){
        sensor_display = sensor_display==0 ? num_sensors-1 : sensor_display-1;
        disp.temps(temp_array, sensor_display);
        sensor_display_timer.restart();
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
        disp.log_selection(do_log);
        state=LOG_SELECT;
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
        disp.temps(temp_array, sensor_display);
        state=DISP_TEMP;
      }else if(button_select.pushed()){
        disp.temps(temp_array, sensor_display);
        state = DISP_TEMP;
      }
      break;
    case ERROR:
      break;
    default:
      state = ERROR;
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
  sensors.requestTemperatures();
  for(int i=0;i<num_sensors; i++){
    DeviceAddress adr;
    if(sensors.getAddress(adr, i)){
        temp_array[i] = sensors.getTempC(adr);
        Serial.print("  ");printAddress(adr);Serial.print(" - ");Serial.println(temp_array[i]);
    }
  }
  //update the display
  if(state==DISP_TEMP){
    disp.temps(temp_array, sensor_display);
  }
  //save to SD
  if(do_log){
    log_temps(date_str);
  }
  return;  
}

/*
 * Logs current temperature data to SD card.
 Note that the global variable "filename" must be initialised
 */
void log_temps(char *date_str){
  File f = SD.open(filename, FILE_WRITE);
  if(f) {
    f.print(date_str);
    for(byte a=0; a<num_sensors; a++){
      f.print(",");
      f.print(temp_array[a]);
    }
    f.println(""); 
    f.close();
  }
  return;  
}

void init_sd_card(){
  // see if the card is present and can be initialized:
  if (!SD.begin(SD_SS)){
    return;
  }

  if(!SD.exists("log")){
    if(!SD.mkdir("log")){
      do_log=0;
      return;
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
  //Save header information to file.
}

/*
 * Callback function when the display timeout timer expires
 */
void timeout_display(){
  disp.temps(temp_array, sensor_display);
  state = DISP_TEMP;
}

void next_sensor_display(){
  if(state==DISP_TEMP){
    sensor_display = (sensor_display+1)%num_sensors;
    disp.temps(temp_array, sensor_display);
  }
}
