#ifndef DISPLAY_H
#define DISPLAY_H
#include <Arduino.h>
#include "LiquidCrystal.h"
#include "defs.h"
#include "RTClib.h"
#include "misc.h"

class Display{
  public:
    Display(byte a, byte b, byte c, byte d, byte e, byte f);
    void all_temps(float *temp_array, char probe_name_array[MAX_SENSORS][PROBE_NAME_LEN], byte num_sensors);
    void meas_interval_select(int meas_interval);
    void log_selection(byte do_log);
    void disp_time(RTC_DS1307 rtc);
    void dump_log(void);
    void error(byte error_code);

  private:
    LiquidCrystal lcd;
};


#endif //DISPLAY_H
