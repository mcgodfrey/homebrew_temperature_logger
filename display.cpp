#include "display.h"
#include <LiquidCrystal.h>

Display::Display(byte a, byte b, byte c, byte d, byte e, byte f) : lcd(a,b,c,d,e,f) {
  lcd.begin(16,2);
}

void Display::temps(float *temps, byte no_sensors) {
   switch (no_sensors) {
     case 1:
       lcd.clear();
       lcd.print("Temperature: ");
       lcd.setCursor(0,1);
       lcd.print(temps[0],1);
       break;
     case 2:
       lcd.clear();
       lcd.print("Temp1: ");lcd.print(temps[0],1);
       lcd.setCursor(0,1);
       lcd.print("Temp2: ");lcd.print(temps[1],1);
       break;
       /*
       case 3:
       lcd.clear();
       lcd.print("T1: ");lcd.print(temps[0],1);
       lcd.setCursor(8,0);
       lcd.print("T2: ");lcd.print(temps[1],1);
       lcd.setCursor(0,1);
       lcd.print("T3: ");lcd.print(temps[2],1);
       break;
     case 4:
       lcd.clear();
       lcd.print("T1: ");lcd.print(temps[0],1);
       lcd.setCursor(8,0);
       lcd.print("T2: ");lcd.print(temps[1],1);
       lcd.setCursor(0,1);
       lcd.print("T3: ");lcd.print(temps[2],1);
       lcd.setCursor(8,1);
       lcd.print("T4: ");lcd.print(temps[3],1);
       break;
       */
     default:
       lcd.clear();
       break;
   }
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
