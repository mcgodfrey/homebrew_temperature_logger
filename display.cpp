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
void Display::all_temps(float *temp_array, char probe_name_array[MAX_SENSORS][PROBE_NAME_LEN], byte num_sensors) {
  lcd.clear();
  if (num_sensors == 0){
    lcd.setCursor(0,0);
    lcd.print("no sensors");
  }else{
    for(byte a=0;a<num_sensors;a++){
      lcd.setCursor(a*5,0);
      lcd.print(probe_name_array[a]);
      lcd.setCursor(a*5,1);
      lcd.print(temp_array[a],1);
    }
  }
}

/*
 * measurement interval select screen
 */
void Display::meas_interval_select(int meas_interval){
  lcd.clear();
  lcd.setCursor(0,0);  //not necessary? I think lcd.clear() sets the cursor position
  lcd.print("Meas. interval");
  lcd.setCursor(0,1);
  lcd.print(meas_interval);
}

void Display::disp_time(RTC_DS1307 rtc){
  lcd.clear();
  DateTime t = rtc.now();
  char str[12];
  date2str(t, str);
  lcd.setCursor(0,0);
  lcd.print(str);
  time2str(t, str);
  lcd.setCursor(0,1);
  lcd.print(str);
}

/*
 * Log selection (enable) screen
 */
void Display::log_selection(byte do_log){
  lcd.clear();
  lcd.print("Log data? ");
  if (do_log){  
    lcd.print("yes");
  }else{
    lcd.print("no");
  }
}

/*
 * Dump log (over serial) screen
 */
void Display::dump_log(){
  lcd.clear();
  lcd.print("Dump log?");
  lcd.setCursor(0,1);
  lcd.print("press up or down");
}

/*
 * error display screen
 */
void Display::error(byte error_code){
   lcd.clear();
   lcd.print("ERROR:");
   lcd.setCursor(0,1);
   lcd.print(error_code);
}
