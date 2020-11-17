# Astro Timer
This project was born while searching for a timer device that could be used to control lamps based on sunrise/sunset. I was unable to find anything within a reasonable price range that would handle the needs I had:
  - Set a time in the morning when the lights should turn on.
  - Turn off lights at sunrise, with the ability to offset the time either before or after the actual sunrise.
  - Turn on lights at sunset, with the ability to offset the time either before or after the actual sunset.
  - Set a time in the evening when the lights should turn off.

The prototype is using the Arduino Uno board, it uses almost all RAM, but seems to be stable. If you want to add functionality, you might need a more powerful board.

# Hardware
The following hardware is needed:
  - I2C real time clock (RTC) based on DS1307, for example the one from Adafruit.
  - I2C OLED display based on SSD1306 or SH1106, most cheap Ebay variants works fine.
  - Four pushbuttons for Up/Down/Set/Exit.
  - Relay board **or** a NPN transistor driving a solid state relay.

# Schematics
![Schematics](https://github.com/VauxhallViva/astrotimer/blob/main/images/AstroTimer_schematic.png)
