/* 
Very simplified version of :

Switch
Copyright (C) 2012  Albert van Dalen http://www.avdweb.nl
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License at http://www.gnu.org/licenses .
 

 
*/
 
#include "Arduino.h"
#include "Switch.h"
 
Switch::Switch(byte _pin, byte PinMode, bool polarity, byte debounceDelay):  pin(_pin), polarity(polarity), debounceDelay(debounceDelay) { 
  pinMode(pin, PinMode); 
  _switchedTime = millis();
  level = digitalRead(pin);
}
 
bool Switch::poll(){ 
  bool newlevel = digitalRead(pin);   
 
  if((newlevel != level) & (millis() - _switchedTime >= debounceDelay)) {
    _switchedTime = millis();
    level = newlevel;
    _switched = 1;

    return _switched;
  }
  return _switched = 0;
}
 
bool Switch::pushed(){ 
  return _switched && !(level^polarity); 
} 


