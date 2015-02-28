#include "display.h"
#include <LiquidCrystal.h>

Display::Display(byte a, byte b, byte c, byte d, byte e, byte f) : lcd(a,b,c,d,e,f) {
  lcd.begin(16,2);
}

void Display::temps(float *temps, byte sensor_num) {
  lcd.clear();
  lcd.print("Temperature ");
  lcd.print(sensor_num);
  lcd.print(":");
  lcd.setCursor(0,1);
  lcd.print(temps[sensor_num],1);
}

void Display::meas_interval_select(int meas_interval){
  lcd.clear();
  lcd.print("Meas. interval");
  lcd.setCursor(0,1);
  lcd.print(meas_interval);
}

void Display::log_selection(byte do_log){
  lcd.clear();
  lcd.print("Log data? ");
  if (do_log){  
    lcd.print("yes");
  }else{
    lcd.print("no");
  }
}

void Display::dump_log(){
  lcd.clear();
  lcd.print("Dump log?");
  lcd.setCursor(0,1);
  lcd.print("press up or down");
}
