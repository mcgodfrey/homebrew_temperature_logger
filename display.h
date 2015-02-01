#ifndef DISPLAY_h
#define DISPLAY_h
#include <Arduino.h>
#include <LiquidCrystal.h>

class Display{
  public:
    Display(char a, char b, char c, char d, char e, char f);
    void temps(float *temps, int no_sensors);
    void sensor_select(int no_sensors);
    void meas_interval_select(int meas_interval);
    void sensor_cal(int sensor_no, double a, double b);
    void log_selection(char do_log);
    void dump_log(void);
    void error(char *msg);

  private:
    LiquidCrystal lcd;
};


#endif
