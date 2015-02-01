#include "display.h"
#include <LiquidCrystal.h>

Display::Display(char a, char b, char c, char d, char e, char f) : lcd(a,b,c,d,e,f) {
  lcd.begin(16,2);
}

void Display::temps(float *temps, int no_sensors) {
   switch (no_sensors) {
     case 1:
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Temperature: ");
       lcd.setCursor(0,1);
       lcd.print(temps[0],1);
       break;
     case 2:
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Temp1: ");lcd.print(temps[0],1);
       lcd.setCursor(0,1);
       lcd.print("Temp2: ");lcd.print(temps[1],1);
       break;
     case 3:
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("T1: ");lcd.print(temps[0],1);
       lcd.setCursor(8,0);
       lcd.print("T2: ");lcd.print(temps[1],1);
       lcd.setCursor(0,1);
       lcd.print("T3: ");lcd.print(temps[2],1);
       break;
     case 4:
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("T1: ");lcd.print(temps[0],1);
       lcd.setCursor(8,0);
       lcd.print("T2: ");lcd.print(temps[1],1);
       lcd.setCursor(0,1);
       lcd.print("T3: ");lcd.print(temps[2],1);
       lcd.setCursor(8,1);
       lcd.print("T4: ");lcd.print(temps[3],1);
       break;
     default:
       lcd.clear();
       break;
   }
}


void Display::sensor_select(int no_sensors){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Num. sensors");
  lcd.setCursor(0,1);
  lcd.print(no_sensors);
}

void Display::meas_interval_select(int meas_interval){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Meas. interval");
  lcd.setCursor(0,1);
  lcd.print(meas_interval);
}


void Display::sensor_cal(int sensor_no, double a, double b){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sensor ");
  lcd.print(sensor_no);
  lcd.setCursor(0,1);
  lcd.print("a=");lcd.print(a,2);
  lcd.setCursor(8,1);
  lcd.print("b=");lcd.print(b,2);
}

void Display::log_selection(char do_log){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Log data? ");
  if (do_log){  
    lcd.print("yes");
  }else{
    lcd.print("no");
  }
}

void Display::error(char *msg){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("error");
  lcd.setCursor(0,1);
  lcd.print(msg);
}
