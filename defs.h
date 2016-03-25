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
 

 I2C (internal pullups) for RTC
 * SDA = D2
 * SCL = D3
  
 Temperature sensors
 * Dallas oneWire sensors on pin 13
 
 SPI
 * For SD card
 * using hardware MOSI, MISO, SCK pins.
 * SS input pin not used (I think it is needed internally by the SPI library)
 * SS output is set to pin 2, but I don't actually use this as I only have 1 device
 * 5v/3v3 level shifter on SPI bus.
 * 3v3 comes from LM1117 on SD card board
 * 
 */
#ifndef DEFS_H
#define DEFS_H

#define RTC 1
#define SD_PRESENT 1

//pins
#define LCD_RS 4
#define LCD_E 5
#define LCD_D4 6
#define LCD_D5 7
#define LCD_D6 8 
#define LCD_D7 9
#define PB_U 10
#define PB_D 12
#define PB_S 11
#define ONE_WIRE_BUS 13

//error codes
#define ERROR_NONE 0
#define ERROR_NO_PROBE 40
#define ERROR_RTC 50
#define ERROR_NO_SD 55
#define ERROR_SD_MISC 56
#define ERROR_LOGGING 58
#define ERROR_UNKNOWN_STATE 60

//misc definitions
#define BAUD_RATE 9600
#define SD_SS 2
#define MAX_SENSORS 4
#define PROBE_NAME_LEN 5
#define DEBOUNCE_TIME 50
#define TIMEOUT_PERIOD 10
#define INTERVAL_INCR 1
#define MIN_INTERVAL 5
#define MAX_INTERVAL 255
#define TEMPERATURE_PRECISION 12
#define CONVERSION_TIME 750 / (1 << (12 - TEMPERATURE_PRECISION))
//#define CONVERSION_TIME 1000 / (1 << (12 - TEMPERATURE_PRECISION))
//default state
#define DEFAULT_DO_LOG false
#define DEFAULT_MEAS_INTERVAL 10




#endif // DEFS_H
