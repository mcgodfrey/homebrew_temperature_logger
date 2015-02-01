/*
 * SimpleTimer.h
 *
 * SimpleTimer - A timer library for Arduino.
 * Author: mromani@ottotecnica.com
 * Copyright (c) 2010 OTTOTECNICA Italy
 *
 * This library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser
 * General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser
 * General Public License along with this library; if not,
 * write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */


#ifndef SIMPLETIMER_H
#define SIMPLETIMER_H

#include <Arduino.h>

typedef void (*timer_callback)(void);

class SimpleTimer {

public:
    const static int MAX_TIMERS = 3;
    const static int RUN_FOREVER = 0;
    const static int RUN_ONCE = 1;

    SimpleTimer();
    void run(); // this function must be called inside loop()
    int setInterval(long d, timer_callback f); // call function f every d milliseconds
    int setTimeout(long d, timer_callback f); // call function f once after d milliseconds
    int setTimer(long d, timer_callback f, int n); // call function f every d milliseconds for n times
    void deleteTimer(int numTimer); 
    void restartTimer(int numTimer); 
    boolean isEnabled(int numTimer); 
    void enable(int numTimer); 
    void disable(int numTimer); 
    void toggle(int numTimer); // enables the specified timer if it's currently disabled, and vice-versa
    int getNumTimers();
    int getNumAvailableTimers() { return MAX_TIMERS - numTimers; };

private:
    // deferred call constants
    const static int DEFCALL_DONTRUN = 0;       // don't call the callback function
    const static int DEFCALL_RUNONLY = 1;       // call the callback function but don't delete the timer
    const static int DEFCALL_RUNANDDEL = 2;      // call the callback function and delete the timer

    int findFirstFreeSlot();
    unsigned long prev_millis[MAX_TIMERS]; // value returned by the millis() function in the previous run() call
    timer_callback callbacks[MAX_TIMERS];
    long delays[MAX_TIMERS];
    int maxNumRuns[MAX_TIMERS]; // number of runs to be executed for each timer
    int numRuns[MAX_TIMERS]; // number of executed runs for each timer
    boolean enabled[MAX_TIMERS]; // which timers are enabled
    int toBeCalled[MAX_TIMERS]; // deferred function call (sort of) - N.B.: this array is only used in run()
    int numTimers; // actual number of timers in use
};

#endif
