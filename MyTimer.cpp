/*
 * MyTimer.cpp
 *
 * based on:
 * SimpleTimer - A timer library for Arduino.
 * Author: mromani@ottotecnica.com
 * Copyright (c) 2010 OTTOTECNICA Italy
 *
 */


#include "MyTimer.h"

static unsigned long elapsed(){ 
  return millis();
}

MyTimer::MyTimer(int d, timer_callback f, boolean r, boolean ms) {
  prev_millis =  elapsed();
  callback = f;
  if (ms){
    timer_delay = d;
  }else {
    timer_delay = d*1000;
  }
  run_once = r;
  enabled = false;
}

void MyTimer::run(){
  unsigned long current_millis = elapsed(); // get current time
  if(current_millis - prev_millis >= timer_delay){
    prev_millis += timer_delay;     // update time
    // check if the timer callback has to be executed
    if(enabled){
      (*callback)();
      if(run_once){
        enabled = false;
      }
    }
  }
}

void MyTimer::setCallback(timer_callback f){
  callback = f;
}

void MyTimer::setInterval(int d){
  timer_delay = d*1000;
  run_once = false;
}

void MyTimer::setTimeout(int d){
  timer_delay = d*1000;
  run_once = true;
}

void MyTimer::restart(){
  prev_millis = elapsed();
  enabled = true;
}

void MyTimer::enable(){
  enabled = true;
}

void MyTimer::disable(){
  enabled = false;
}
