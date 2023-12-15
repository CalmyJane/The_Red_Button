# The BIG Red Button

## Introduction
This repo contains code for an arduino nano that uses the FastLED-Library to control an LED-Strip. The BIG Red Button is meant to be a light installation for parties and events. It's a big red buzzer that people can press. It's powered thorugh USB and controls an LED-Strip. It's meant to be connected to several LED-Strips that run away from the button. In the main mode a light point runs away from the buzzer to the end of the led strips and pressing the buzzer feels like unleashing some kind of shockwave.
There are other modes as well. The Buzzer has 4 dipswitches on the side and a Potentiometer:

## Potentiometer:
The Potentiometer controls the Brightness of the strip in run-mode and the settings values in setting-mode

## Dip-Switches:
The first 3 dipswitches are used to input a 3-bit number (0..7) and the last dipswitch (dip 4) is used to set run-mode or settin-mode. 

## Modes:
The buzzer features several different modes that can be selected while in run-mode (dip 4 is false)

### 0: Running Light
When the buzzer is pressed, a single light runs down the line. press multiple times to run. Uses the SPEED-Setting for running speed and COLORS for point color. If COLOR-settings are all 0 it picks random color on each buzzer press.
### 1: Light Switch
A simple light switch. uses COLORS for light color - be aware, that many LEDs lit up at once cause color fades due to voltage drops.
### 2: Pressure Bar
While the buzzer is pressed the leds start to fill in the set color (or random color if all colors are 0)
### 3: Waveform Mode
Similar to Pressure Bar, but with sine-wobbles on the color
### 4: Twinkle
Made for Cloud lamps, there are random twinkles popping up in different colors

## Settings:
When you set Dipswitch 4 to High, you enter Settings mode. You can now view settings selected through dip switches 1-3. They will be displayed as a progress bar on the LED strip. To adjust a setting, select it through dip 1-3, hold down the buzzer (progress bar starts blinking) and turn the potentiometer. Release the buzzer to log the value. Make sure to set dip 4 to false when you are done, this goes back to run-mode and saves your settings to EEPROM. If you don't return to run-mode, settings are not saved!
Tipp: You can set the brightness low in run-mode and then switch to settings to avoid lighting everythign up ;)

### 0: COLOR_RED
### 1: COLOR_GREEN
### 2: COLOR_BLUE
### 3: SPEED
### 4: NUMBER_LEDS - number of connected LEDs. This requires a restart to take effect.