# Astro Timer
This project was born while searching for a timer device that could be used to control lamps based on sunrise/sunset. I was unable to find anything within a reasonable price range that would handle the needs I had:
  - Set a time in the morning when the lights should turn on.
  - Turn off lights at sunrise, with the ability to offset the time either before or after the actual sunrise.
  - Turn on lights at sunset, with the ability to offset the time either before or after the actual sunset.
  - Set a time in the evening when the lights should turn off.

To make it even more flexible, there are two relay outputs. The first output is switched on/off for all the events, while the second output does not turn on/off at the preset times, only at sunrise/sunset. This way you can one set of lights that turns on at sunset, turns off at a preset time (after sunset), then turns on again at the preset time (before sunrise), while another set of lights are turned on at sunset and then off again at sunrise. You can of course choose to only include one relay in your setup.

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
