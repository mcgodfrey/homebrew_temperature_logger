# Homebrew temperature logger

Arduino controlled temperature monitor and logger with multiple temperature probe inputs. 
This was designed to log temperatures in my homebrew fermentation fridge, requiring separate probes for the beer temperature, the fridge temperature and the ambient temperature. 

More details can be found on my blog - [https://www.dangerfromdeer.com] 
- [This project's post](https://www.dangerfromdeer.com) 
- [3D printing the enclosure](http://dangerfromdeer.com/2016/03/20/designing-a-3d-printed-electronics-enclosure/)

### Overview
- Up to 3 OneWire temperature probe inputs (can be extended arbitrarily)
- Temperatures displayed on 16x2 LCD and logged to an SD card
- Menu is operated via 3 pushbuttons (up, down, set)
- RTC module for accurate timestamps
- USB serial interface to dump the temperature log file and debugging
- Temperature probes each have unique address and are recognised by the code and given meaningful names
- Probes can be plugged and unplugged during logging and the display and log file will reflect which ones are currently plugged in

