#The BIG Red Button

##Introduction
This repo contains code for an arduino nano that uses the FastLED-Library to control an LED-Strip. The BIG Red Button is meant to be a light installation for parties and events. It's a big red buzzer that people can press. It's powered thorugh USB and controls an LED-Strip. It's meant to be connected to several LED-Strips that run away from the button. In the main mode a light point runs away from the buzzer to the end of the led strips and pressing the buzzer feels like unleashing some kind of shockwave.
There are other modes as well. The Buzzer has 4 dipswitches on the side and a Potentiometer:

###Potentiometer:
The Potentiometer controls the Brightness of the strip in run-mode and the settings values in setting-mode

###Dip-Switches:
The first 3 dipswitches are used to input a 3-bit number (0..7) and the last dipswitch is used to set run-mode or settin-mode. Run-Mode means that the first three dipswitches selects mode 0..7 and the buzzer does something depending on the selected mode. The Poti controls overall brightness. Settings-Mode means that you can adjust one out of 8 settings (0..7). You can select the setting with the dip-switches, and the LED-Strip will display a progress-bar with the current value. To adjust the value, press down the big buzzer (the progress-bar starts blinking), and turn the poti knob.
Tipp: You can set the brightness low in run-mode and then switch to settings to avoid lighting everythign up ;)