/*
  PT100.h - measure temp with PT100
  Matt Godfrey, Jan 2015
*/

#ifndef PT100_h
#define PT100_h
#include <Arduino.h>


class PT100{
  public:
    PT100(int input_pin);
    void calibrate(float a, float b);
    float get_temperature();
    //float get_temperature_averaged(int n);
    //int get_raw_ADC();
    float get_cal_a();
    float get_cal_b();


  private:
    int _input_pin;
    float _a;
    float _b;
};

#endif
