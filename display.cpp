#include "display.h"
#include <LiquidCrystal.h>

Display::Display(byte a, byte b, byte c, byte d, byte e, byte f) : lcd(a,b,c,d,e,f) {
  lcd.begin(16,2);
}

void Display::temps(float *temps, byte sensor_num) {
  lcd.clear();
  lcd.print(F("Temperature "));
  lcd.print(sensor_num);
  lcd.print(":");
  lcd.setCursor(0,1);
  lcd.print(temps[sensor_num],1);
}

void Display::all_temps(float *temps, byte num_sensors) {
  lcd.clear();
  byte rows[4] = {0,0,1,1};
  byte cols[4] = {0,8,0,8};
  for(byte a=0;a<num_sensors;a++){
    lcd.setCursor(cols[a],rows[a]);
    lcd.print("T");lcd.print(a+1);lcd.print("=");
    lcd.print(temps[a],1);
  }
}

void Display::meas_interval_select(int meas_interval){
  lcd.clear();
  lcd.print(F("Meas. interval"));
  lcd.setCursor(0,1);
  lcd.print(meas_interval);
}

void Display::log_selection(byte do_log){
  lcd.clear();
  lcd.print(F("Log data? "));
  if (do_log){  
    lcd.print(F("yes"));
  }else{
    lcd.print(F("no"));
  }
}

void Display::dump_log(){
  lcd.clear();
  lcd.print(F("Dump log?"));
  lcd.setCursor(0,1);
  lcd.print(F("press up or down"));
}


void Display::error(byte error_code){
   lcd.clear();
   lcd.print(F("ERROR:"));
   lcd.setCursor(0,1);
   lcd.print(error_code);
}
