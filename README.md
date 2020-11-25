# Astro Timer
This project was born while searching for a timer device that could be used to control lamps based on sunrise/sunset. I was unable to find anything within a reasonable price range that would handle the needs I had:
  - Set a time in the morning when the lights should turn on.
  - Turn off lights at sunrise, with the ability to offset the time either before or after the actual sunrise.
  - Turn on lights at sunset, with the ability to offset the time either before or after the actual sunset.
  - Set a time in the evening when the lights should turn off.

To make it even more flexible, there are two relay outputs. The first output is switched on/off for all the events, while the second output does not turn on/off at the preset times, only at sunrise/sunset. This way you can have two sets of lights: The first set (pin #3 by default) turns on at sunset, turns off at a preset time (after sunset), then turns on again at the preset time (before sunrise), and off again at sunrise. The second set (pin #2) turns on at sunset and then off again at sunrise. You can of course choose to only include one relay in your setup.

The prototype is using the Arduino Uno board, it uses almost all RAM, but seems to be stable. If you want to add functionality, you might need a more powerful board.

# Hardware
The following hardware is needed:
  - I2C real time clock (RTC) based on DS1307, for example the one from Adafruit.
  - I2C OLED display based on SSD1306 or SH1106, most cheap Ebay variants works fine.
  - Four pushbuttons for Up/Down/Set/Exit.
  - Relay boards **or** NPN transistors driving solid state relays.

# Schematics
Below is the schematics on how to connect the hardware together. I have included both a relay module and a transistor/solid state relay (any standard NPN transistor will work, just beware of the pinout), but you can choose either setup for both relay outputs.

![Schematics](https://github.com/VauxhallViva/astrotimer/blob/main/images/AstroTimer_schematic.png)

# How to setup and use
## Compilation
Before you compile and upload the code to the Arduino board, make sure you go through the "defines" and comment/uncomment the options you need for your specific case:
- Latitude, longitude and offset from UTC time for your geographical location.
- I2C addresses for your chosen RTC and OLED.
- Your type of OLED (driver chip type).

## Hardware remarks
The two relay outputs (pin 2 and 3) can be used in two different ways:
- Connected directly to a 5V relay module (there are plenty of variants on Ebay), where there is a transistor on-board to drive the relay.
- Connected to a resistor plus NPN transistor, driving either a standard 5V relay or a solid state relay.

It is highly recommended to use a solid state relay if you intend to control mains current (110 to 230V AC). The relay should be mounted in an approved enclosing and installed by a professional electrician.

## How to use
When the code runs for the first time, and your RTC module is "fresh", the time will be set to the date and time when you compiled the code, and all settings will be defaulted.

The buttons have the following functions:
**SET**: Enters settings mode, and steps through the available settings.
**EXIT**: Exits settings mode.
**UP/DOWN**: In normal mode, the up button will toggle relay 1 on/off, and the down button will toggle relay 2 on/off. In settings mode, the selected setting is adjusted.

To enter the settings mode, press the SET button. You can now use the UP/DOWN buttons to choose either "Settings" or "Adj.time". Then press the SET button again to enter the selected area, and you can now adjust the settings or the time/date.

Press the SET button to move to the next parameter, and use UP/DOWN buttons to change the value for each parameter. When done, press the EXIT button to return to the time display.

*Settings*
**Sunrise:** When enabled, relay 1 will turn on at the "Sunrise On" time and both relays will be turned off at sunrise.
**Sunrise offset:** Offset the sunrise time (plus or minus) to turn off relays before or after the actual sunrise.
**Sunrise on:** The time when relay 1 will turn on, if "Sunrise" is enabled. If set to 0:00, the relay will not turn on.
**Sunset:** When enabled, both relays will be turned on at sunset and relay 1 will turn off at the "Sunset Off" time.
**Sunset offset:** Offset the sunset time (plus or minus) to turn on relays before or after the actual sunset.
**Sunset off:** The time when relay 1 will turn off, if "Sunset" is enabled. If set to 0:00, the relay will not turn off.

*Adjust time*
**Hour/Minute/Year/Month/Date:** Adjust to the desired value.
**DST (Daylight Savings Time):** Set to either On or Off. The timer will automatically change this setting if it's running on the dates when DST is changed.


