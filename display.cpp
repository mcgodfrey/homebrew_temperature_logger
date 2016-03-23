#include "display.h"
#include <LiquidCrystal.h>

//constructor
//This syntax initialises lcd object before the function starts or something
//I need to look up exactlly why I needed to do it this way.
Display::Display(byte a, byte b, byte c, byte d, byte e, byte f) : lcd(a,b,c,d,e,f) {
  lcd.begin(16,2);
}

/*
 * displays the probe names and tempeartures
 */
void Display::all_temps(float *temps, char probe_names[MAX_SENSORS][PROBE_NAME_LEN], byte num_sensors) {
  lcd.clear();
  byte rows[4] = {0,0,1,1};
  byte cols[4] = {0,8,0,8};
  for(byte a=0;a<num_sensors;a++){
    lcd.setCursor(a*5,0);
    lcd.print(probe_names[a]);
    lcd.setCursor(a*5,1);
    lcd.print(temps[a],1);
  }
}

/*
 * measurement interval select screen
 */
void Display::meas_interval_select(int meas_interval){
  lcd.clear();
  lcd.setCursor(0,0);  //not necessary? I think lcd.clear() sets the cursor position
  lcd.print(F("Meas. interval"));
  lcd.setCursor(0,1);
  lcd.print(meas_interval);
}

/*
 * Log selection (enable) screen
 */
void Display::log_selection(byte do_log){
  lcd.clear();
  lcd.print(F("Log data? "));
  if (do_log){  
    lcd.print(F("yes"));
  }else{
    lcd.print(F("no"));
  }
}

/*
 * Dump log (over serial) screen
 */
void Display::dump_log(){
  lcd.clear();
  lcd.print(F("Dump log?"));
  lcd.setCursor(0,1);
  lcd.print(F("press up or down"));
}

/*
 * error display screen
 */
void Display::error(byte error_code){
   lcd.clear();
   lcd.print(F("ERROR:"));
   lcd.setCursor(0,1);
   lcd.print(error_code);
}
