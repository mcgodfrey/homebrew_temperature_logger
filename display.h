#ifndef DISPLAY_h
#define DISPLAY_h
#include <Arduino.h>
#include "LiquidCrystal.h"

class Display{
  public:
    Display(byte a, byte b, byte c, byte d, byte e, byte f);
    void temps(float *temps, byte no_sensors);
    void sensor_select(byte no_sensors);
    void meas_interval_select(int meas_interval);
    //void sensor_cal(int sensor_no, double a, double b);
    void log_selection(byte do_log);
    void dump_log(void);
    void error(char *msg);

  private:
    LiquidCrystal lcd;
};


#endif
