/*
  PT100.h - measure temp with PT100
  Matt Godfrey, Jan 2015
*/

#include <Arduino.h>
#include "PT100.h"

PT100::PT100(int input_pin){
  _input_pin = input_pin;
  pinMode(_input_pin, INPUT);  
}


void PT100::calibrate(float a,float b){
  _a = a;
  _b = b;
}


float PT100::get_temperature(){
  float temperature = _a*analogRead(_input_pin) + _b;
  return temperature;
}

/*
float PT100::get_temperature_averaged(int n){
  int tmp=0;
  for(int a=0;a<(1<<n);a++){
    tmp+=get_raw_ADC();
  }
  float temperature=_a*(tmp>>n) + _b;
  return temperature;
}
*/

/*
int PT100::get_raw_ADC(){
  return analogRead(_input_pin);
}
*/

float PT100::get_cal_a(){
  return _a;
}

float PT100::get_cal_b(){
  return _b;
}

