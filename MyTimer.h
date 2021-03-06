/*
 * MyTimer.h
 *
 * based on:
 * SimpleTimer - A timer library for Arduino.
 * Author: mromani@ottotecnica.com
 * Copyright (c) 2010 OTTOTECNICA Italy
 *
 */


#ifndef MYTIMER_H
#define MYTIMER_H

#include <Arduino.h>

typedef void (*timer_callback)(void);

class MyTimer {
public:
    MyTimer(int d, timer_callback f, boolean r, boolean ms);
    void run(); // this function must be called inside loop()
    void setCallback(timer_callback f);
    void setInterval(int d); // call function f every d milliseconds
    void setTimeout(int d); // call function f once after d milliseconds
    void restart(); 
    void enable(); 
    void disable(); 

private:
    unsigned long prev_millis; // value returned by the millis() function in the previous run() call
    timer_callback callback;
    unsigned long timer_delay;
    boolean enabled; // which timers are enabled
    boolean run_once;
};

#endif //MYTIMER_H

