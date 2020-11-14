// Astronomical Light Timer by Leif Almgren - ver 1.0 (2020-11-14)
// Released under GNU General Public License v3.0
// ------------------------------------------------------------------------------------------
// This code runs on Arduino Uno or similar boards, and requires a DS1307 RTC module and
// an OLED display (SSD1306 and SH1106 supported). Four buttons are used for input.
// It will control a relay output (relay board, or a transistor driving a SSR),
// turning the output on at a set time before sunrise, turn off at sunrise, turn on at
// sunset and off again at a set time. Times and offsets can be adjusted.
// ------------------------------------------------------------------------------------------
// Change the LAT/LONG and UTF offset below to match your location
// Also comment/uncomment your type of OLED display

//******************* CONSTANTS AND VARIABLES *******************

#include <Wire.h>             // I2C communication
#include <EEPROM.h>           // Storing settings in EEPROM
#include <Dusk2Dawn.h>        // Sunrise/sunset calculations
#include <RTClib.h>           // RTC DS1307 module
#include <avr/pgmspace.h>     // Storing strings in program space and not memory

#define LATITUDE 58.1476192   // CHANGE THESE VALUES TO MATCH 
#define LONGITUDE 16.6136953  // YOUR OWN LOCATION
#define UTC_OFFSET 1          // AND TIME ZONE

#define DS1307_ADDRESS 0x68   // I2C address for RTC module

#define SCREEN_WIDTH 128      // OLED display width, in pixels
#define SCREEN_HEIGHT 64      // OLED display height, in pixels
#define OLED_ADDRESS 0x3C     // I2C address for OLED display

//#define OLED_SSD1306        // UNCOMMENT if you have OLED based on SSD1306 (usually the 0.96" variants)
#define OLED_SH1106           // UNCOMMENT if you have OLED based on SH1106 (usually the 1.3" variants)

#if defined OLED_SSD1306
  #define OLED_RESET    -1      // Reset pin # (or -1 if sharing Arduino reset pin)
  #include <Adafruit_SSD1306.h> // OLED driver from Arduino library manager
  #define OLED_WHITE SSD1306_WHITE
  #define OLED_BLACK SSD1306_BLACK
  #define OLED_INVERSE SSD1306_INVERSE
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

#if defined OLED_SH1106
  #define OLED_RESET    4      // Reset pin # (or -1 if sharing Arduino reset pin)
  #include <Adafruit_SH1106.h> // OLED driver from https://github.com/wonho-maker/Adafruit_SH1106
  #define OLED_WHITE WHITE
  #define OLED_BLACK BLACK
  #define OLED_INVERSE INVERSE
  Adafruit_SH1106 display(OLED_RESET);
#endif

#define BUTTON_SET 6          // I/O pin for SET button
#define BUTTON_EXIT 7         // I/O pin for EXIT button
#define BUTTON_UP 5           // I/O pin for UP button
#define BUTTON_DOWN 4         // I/O pin for DOWN button
#define RELAY 3               // I/O pin for relay output
#define RELAY_ON LOW          // Set to either HIGH or LOW (LOW for relay board, HIGH for transistor/SSR)

#define MODE_TIME 0
#define MODE_SELECT 1
#define MODE_SETTINGS 2
#define MODE_ADJUST 3

#define EEPROM_OFFSET 0

struct settingsStruct {
  bool dstFlag;
  bool onSunset;
  bool onSunrise;
  int sunsetOffset;
  int sunriseOffset;
  int sunsetTime; 
  int sunriseTime; 
};

struct timeStruct {
  byte hour;
  byte minute;
  int year;
  byte month;
  byte day;
};

bool exitBtnPressed = false;
bool setBtnPressed = false;
bool upBtnPressed = false;
bool downBtnPressed = false;

byte exitBtnState = HIGH;
byte exitBtnState_last = HIGH;
byte setBtnState = HIGH;
byte setBtnState_last = HIGH;
byte upBtnState = HIGH;
byte upBtnState_last = HIGH;
byte downBtnState = HIGH;
byte downBtnState_last = HIGH;

byte debounceDelay = 50;
unsigned long lastDebounceTime_exit = 0;
unsigned long lastDebounceTime_set = 0;
unsigned long lastDebounceTime_up = 0;
unsigned long lastDebounceTime_down = 0;

unsigned long lastKeyRead = 0;
unsigned long lastTimeRead = 0;

settingsStruct settings;

RTC_DS1307 RTC;
DateTime timeNow;
timeStruct adjustedTime;

Dusk2Dawn myLocation(LATITUDE, LONGITUDE, UTC_OFFSET);

bool aFlag = true;

byte lastHr = 0;
byte lastMin = 0;
byte lastSec = 0;
byte displayMode = MODE_TIME;
bool relayOn = false;

int sunrise = 0;
int sunset = 0;
byte sunriseHr = 0; byte sunriseMin = 0;
byte sunsetHr = 0; byte sunsetMin = 0;
byte nextSunEvent = 0;  // 0 = sunrise, 1 = sunset

int sunriseOffsetHr; byte sunriseOffsetMin;
byte sunriseOnHr; byte sunriseOnMin;
int sunsetOffsetHr; byte sunsetOffsetMin;
byte sunsetOffHr; byte sunsetOffMin;

byte curPos = 1;

const char settingHeader_0[] PROGMEM = "-SELECT-";
const char settingHeader_1[] PROGMEM = "-SETTINGS-";
const char settingHeader_2[] PROGMEM = "-ADJ.TIME-";
const char * const settingHeaders[] PROGMEM =  { settingHeader_0, settingHeader_1, settingHeader_2 };  
  
const char selectParam_0[] PROGMEM = "Settings";
const char selectParam_1[] PROGMEM = "Adj. time";
const char * const selectParams[] PROGMEM =  { selectParam_0, selectParam_1 };  

const char setParam_0[] PROGMEM = "Sunrise";
const char setParam_1[] PROGMEM = "S-rise ofs";
const char setParam_2[] PROGMEM = "Sunrise on";
const char setParam_3[] PROGMEM = "Sunset";
const char setParam_4[] PROGMEM = "S-set ofs";
const char setParam_5[] PROGMEM = "Sunset off";
const char * const setParams[] PROGMEM =  { setParam_0, setParam_1, setParam_2, setParam_3, setParam_4, setParam_5 };  

const char adjParam_0[] PROGMEM = "Hour";
const char adjParam_1[] PROGMEM = "Minute";
const char adjParam_2[] PROGMEM = "Year";
const char adjParam_3[] PROGMEM = "Month";
const char adjParam_4[] PROGMEM = "Date";
const char adjParam_5[] PROGMEM = "DST";
const char * const adjParams[] PROGMEM =  { adjParam_0, adjParam_1, adjParam_2, adjParam_3, adjParam_4, adjParam_5 };  

const char sunEvent_0[] PROGMEM = "SUNRISE";
const char sunEvent_1[] PROGMEM = "SUNSET";
const char * const sunEvents[] PROGMEM =  { sunEvent_0, sunEvent_1 };  

char buffer[10];

//******************* SETUP CODE *******************

void setup() {
  Serial.begin(9600);
  Wire.begin();
  digitalWrite(SDA, LOW);
  digitalWrite(SCL, LOW);
  RTC.begin();

  pinMode(BUTTON_EXIT, INPUT_PULLUP);
  pinMode(BUTTON_SET, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, !RELAY_ON);
    
  if (! RTC.isrunning()) {
    //Serial.println(F("RTC is NOT running, setting time."));
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
//  else
//    Serial.println(F("RTC is running."));

  #if defined OLED_SSD1306
   if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) { 
    //Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
   }
  #endif

  #if defined OLED_SH1106
   display.begin(SH1106_SWITCHCAPVCC, OLED_ADDRESS);
  #endif

  // Get settings and validate them
  EEPROM.get(EEPROM_OFFSET,settings);
  
  updateSunEventSettings();

  // Init sun events
  timeNow = RTC.now();
  updateSunEvents();

  // Display startup info
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(OLED_WHITE);
  display.setCursor(0,0);
  display.print(F("AstroTimer"));
  display.fillRect(20,30,88,2, OLED_WHITE);
  display.setTextSize(1);
  display.setCursor(30,45);
  display.print(F("Version 1.0"));
  display.display();
  delay(2000); // Show startup info for 2 sec
  display.clearDisplay();
}

//******************* MAIN LOOP *******************

void loop() {

  if ((millis() - lastTimeRead) > 100) {
    timeNow = RTC.now();
    lastTimeRead = millis();
  }

  if ((millis() - lastKeyRead) > 5) {
    checkButtons();
    lastKeyRead = millis();
  }
  
  // Process key presses
  if (exitBtnPressed) {
    exitBtnPressed = false;
    exitBtnAction();
  }
  if (setBtnPressed) {
    setBtnPressed = false;
    setBtnAction();
  }
  if (upBtnPressed) {
    upBtnPressed = false;
    upDownBtnAction(BUTTON_UP);
  }
  if (downBtnPressed) {
    downBtnPressed = false;
    upDownBtnAction(BUTTON_DOWN);
  }
  
  if (timeNow.hour() != lastHr) {
    //DST check, adjust time and DST setting twice a year
    //This logic works in Europe, but might need adjustment for other parts of the world

    if (timeNow.dayOfTheWeek() == 0 && timeNow.month() == 10 && timeNow.day() >= 25 && timeNow.hour() == 3 && settings.dstFlag == true) {
      //Set the clock to 2 am
      RTC.adjust(DateTime(timeNow.year(), timeNow.month(), timeNow.day(), 2, 0, 0));
      settings.dstFlag = false;
      EEPROM.write(EEPROM_OFFSET,settings.dstFlag);
    }
    if (timeNow.dayOfTheWeek() == 0 && timeNow.month() == 3 && timeNow.day() >= 25 && timeNow.hour() == 2 && settings.dstFlag == false) {
      //Set the clock to 3 am
      RTC.adjust(DateTime(timeNow.year(), timeNow.month(), timeNow.day(), 3, 0, 0));
      settings.dstFlag = true;
      EEPROM.write(EEPROM_OFFSET,settings.dstFlag);
    }
  }

  if (timeNow.minute() != lastMin) {
    checkActions();
    updateSunEvents();
    if (displayMode == 0)
      updateStatusDisplay();
  }

  if (timeNow.second() != lastSec and displayMode == 0) {
    updateTimeDisplay();
  }

  lastSec = timeNow.second();
  lastMin = timeNow.minute();
  lastHr = timeNow.hour();
  delay(10);
}

//-------------------------------------------------------------------

void checkActions() {
  byte sunEventHr;
  byte sunEventMin;
  int sunEvent;
  //Check if relay status should be updated
  if (settings.onSunrise) {
    // Check if relay should be turned on
    if (settings.sunriseTime != 0 and settings.sunriseTime < sunrise) {
      if (sunriseOnHr == timeNow.hour() and sunriseOnMin == timeNow.minute()) {
        relayOn = true;
        digitalWrite(RELAY, RELAY_ON);
      }
    }
  }
  if (settings.onSunset) {
    // Check if relay should be turned off
    if (settings.sunsetTime != 0) {
      if (sunsetOffHr == timeNow.hour() and sunsetOffMin == timeNow.minute()) {
        relayOn = false;
        digitalWrite(RELAY, !RELAY_ON);
      }
    }
  }

  // Check if relay should be turned off at sunrise
  if (settings.onSunrise) {
    sunEvent = ((sunriseHr * 60) + sunriseMin) + settings.sunriseOffset;
    sunEventHr = sunEvent / 60;
    sunEventMin = sunEvent - (sunEventHr * 60);
    if (sunEventHr == timeNow.hour() and sunEventMin == timeNow.minute()) {
      relayOn = false;
      digitalWrite(RELAY, !RELAY_ON);
    }
  }

  // Check if relay should be turned on at sunset
  if (settings.onSunset) {
    sunEvent = ((sunsetHr * 60) + sunsetMin) + settings.sunsetOffset;
    sunEventHr = sunEvent / 60;
    sunEventMin = sunEvent - (sunEventHr * 60);
    if (sunEventHr == timeNow.hour() and sunEventMin == timeNow.minute() and sunset < settings.sunsetTime) {
      relayOn = true;
      digitalWrite(RELAY, RELAY_ON);
    }
  }
}

//-------------------------------------------------------------------

void setBtnAction() {
  if (displayMode == MODE_TIME) {
    displayMode = MODE_SELECT;
    curPos = 1;
  }
  else if (displayMode == MODE_SELECT) {
    if (curPos == 1)
      displayMode = MODE_SETTINGS;
    else {
      displayMode = MODE_ADJUST;
      adjustedTime.hour = timeNow.hour();
      adjustedTime.minute = timeNow.minute();
      adjustedTime.year = timeNow.year();
      adjustedTime.month = timeNow.month();
      adjustedTime.day = timeNow.day();
    }
    curPos = 1;
  }
  else if (displayMode == MODE_SETTINGS) {
    if (curPos < 12)
      curPos++;
    else 
      curPos = 1;
  }
  else if (displayMode == MODE_ADJUST) {
    if (curPos < 6)
      curPos++;
    else 
      curPos = 1;
  }
  updateSetDisplay();
}

void exitBtnAction() {
  if (displayMode != MODE_TIME) {
    if (displayMode == MODE_SETTINGS)
      EEPROM.put(EEPROM_OFFSET,settings);
    else if (displayMode == MODE_ADJUST) {
      updateTime();
      EEPROM.put(EEPROM_OFFSET,settings);
    }
    displayMode = MODE_TIME;
    display.clearDisplay();
    updateTimeDisplay();
    updateStatusDisplay();
  }
}

//-------------------------------------------------------------------

void upDownBtnAction(int btn) {
  if (displayMode == MODE_TIME) {
    if (btn == BUTTON_UP and relayOn == false) {
      relayOn = true;
      digitalWrite(RELAY, RELAY_ON);
    }
    else if (btn == BUTTON_DOWN and relayOn == true) {
      relayOn = false;
      digitalWrite(RELAY, !RELAY_ON);
    }
    updateStatusDisplay();
  }
  else if (displayMode == MODE_SELECT) {
    if (btn == BUTTON_UP and curPos > 1) {
      curPos = 1;
    }
    else if (btn == BUTTON_DOWN and curPos == 1) {
      curPos = 2;
    }  
    updateSetDisplay();
  }
  else if (displayMode == MODE_SETTINGS) {
    if (curPos == 1) {
      if (btn == BUTTON_DOWN)
        settings.onSunrise = false;
      else
        settings.onSunrise = true;
    }
    else if (curPos == 2) {
      if (btn == BUTTON_DOWN)
        settings.sunriseOffset = -abs(settings.sunriseOffset);
      else
        settings.sunriseOffset = abs(settings.sunriseOffset);
    }
    else if (curPos == 3) {
      if (btn == BUTTON_DOWN) {
        if (sunriseOffsetHr > 0)
          sunriseOffsetHr--;
      }
      else {
        if (sunriseOffsetHr < 2)
          sunriseOffsetHr++;
      }
    }
    else if (curPos == 4) {
      if (btn == BUTTON_DOWN) {
        if (sunriseOffsetMin > 0)
          sunriseOffsetMin--;
        else
          sunriseOffsetMin = 59;
      }
      else {
        if (sunriseOffsetMin < 59)
          sunriseOffsetMin++;
        else
          sunriseOffsetMin = 0;
      }
    }
    else if (curPos == 5) {
      if (btn == BUTTON_DOWN) {
        if (sunriseOnHr > 0)
          sunriseOnHr--;
        else
          sunriseOnHr = 23;
      }
      else {
        if (sunriseOnHr < 23)
          sunriseOnHr++;
        else
          sunriseOnHr = 0;
      }
    }
    else if (curPos == 6) {
      if (btn == BUTTON_DOWN) {
        if (sunriseOnMin > 0)
          sunriseOnMin--;
        else
          sunriseOnMin = 59;
      }
      else {
        if (sunriseOnMin < 59)
          sunriseOnMin++;
        else
          sunriseOnMin = 0;
      }
    }
    else if (curPos == 7) {
      if (btn == BUTTON_DOWN)
        settings.onSunset = false;
      else
        settings.onSunset = true;
    }
    else if (curPos == 8) {
      if (btn == BUTTON_DOWN)
        settings.sunsetOffset = -abs(settings.sunsetOffset);
      else
        settings.sunsetOffset = abs(settings.sunsetOffset);
    }
    else if (curPos == 9) {
      if (btn == BUTTON_DOWN) {
        if (sunsetOffsetHr > 0)
          sunsetOffsetHr--;
      }
      else {
        if (sunsetOffsetHr < 2)
          sunsetOffsetHr++;
      }
    }
    else if (curPos == 10) {
      if (btn == BUTTON_DOWN) {
        if (sunsetOffsetMin > 0)
          sunsetOffsetMin--;
        else
          sunsetOffsetMin = 59;
      }
      else {
        if (sunsetOffsetMin < 59)
          sunsetOffsetMin++;
        else
          sunsetOffsetMin = 0;
      }
    }
     else if (curPos == 11) {
      if (btn == BUTTON_DOWN) {
        if (sunsetOffHr > 0)
          sunsetOffHr--;
        else
          sunsetOffHr = 23;
      }
      else {
        if (sunsetOffHr < 23)
          sunsetOffHr++;
        else
          sunsetOffHr = 0;
      }
    }
    else if (curPos == 12) {
      if (btn == BUTTON_DOWN) {
        if (sunsetOffMin > 0)
          sunsetOffMin--;
        else
          sunsetOffMin = 59;
      }
      else {
        if (sunsetOffMin < 59)
          sunsetOffMin++;
        else
          sunsetOffMin = 0;
      }
    }
    calculateSettingValues();
    updateSetDisplay();
  }
  else if (displayMode == MODE_ADJUST) {
    if (curPos == 1) {
      if (btn == BUTTON_DOWN) {
        if (adjustedTime.hour > 0)
          adjustedTime.hour--;
        else
          adjustedTime.hour = 23;
      }
      else {
        if (adjustedTime.hour < 23)
          adjustedTime.hour++;
        else
          adjustedTime.hour = 0;
      }
    }
    if (curPos == 2) {
      if (btn == BUTTON_DOWN) {
        if (adjustedTime.minute > 0)
          adjustedTime.minute--;
        else
          adjustedTime.minute = 59;
      }
      else {
        if (adjustedTime.minute < 59)
          adjustedTime.minute++;
        else
          adjustedTime.minute = 0;
      }
    }
    if (curPos == 3) {
      if (btn == BUTTON_DOWN) {
        if (adjustedTime.year > 2020)
          adjustedTime.year--;
      }
      else {
        if (adjustedTime.year < 2099)
          adjustedTime.year++;
      }
    }
    if (curPos == 4) {
      if (btn == BUTTON_DOWN) {
        if (adjustedTime.month > 1)
          adjustedTime.month--;
        else
          adjustedTime.month = 12;
      }
      else {
        if (adjustedTime.month < 12)
          adjustedTime.month++;
        else
          adjustedTime.month = 1;
      }
    }
    if (curPos == 5) {
      if (btn == BUTTON_DOWN) {
        if (adjustedTime.day > 1)
          adjustedTime.day--;
        else
          adjustedTime.day = 31;
      }
      else {
        if (adjustedTime.day < 31)
          adjustedTime.day++;
        else
          adjustedTime.day = 1;
      }
    }
    if (curPos == 6) {
      if (btn == BUTTON_DOWN)
        settings.dstFlag = false;
      else
        settings.dstFlag = true;
    }
    updateSetDisplay();
  }
}

//-------------------------------------------------------------------

void checkButtons() {
  // Exit button
  int exitBtnReading = digitalRead(BUTTON_EXIT);
  if (exitBtnReading != exitBtnState_last) {
    lastDebounceTime_exit = millis();
  }
  if ((millis() - lastDebounceTime_exit) > debounceDelay) {
    if (exitBtnReading != exitBtnState) {
      exitBtnState = exitBtnReading;
      exitBtnPressed = ( exitBtnState == LOW );
    }
  }
  exitBtnState_last = exitBtnReading;

  // Set button
  int setBtnReading = digitalRead(BUTTON_SET);
  if (setBtnReading != setBtnState_last) {
    lastDebounceTime_set = millis();
  }
  if ((millis() - lastDebounceTime_set) > debounceDelay) {
    if (setBtnReading != setBtnState) {
      setBtnState = setBtnReading;
      setBtnPressed = ( setBtnState == LOW );
    }
  }
  setBtnState_last = setBtnReading;

  // Up button
  int upBtnReading = digitalRead(BUTTON_UP);
  if (upBtnReading != upBtnState_last) {
    lastDebounceTime_up = millis();
  }
  if ((millis() - lastDebounceTime_up) > debounceDelay) {
    if (upBtnReading != upBtnState) {
      upBtnState = upBtnReading;
      upBtnPressed = ( upBtnState == LOW );
    }
  }
  upBtnState_last = upBtnReading;

  // Down button
  int downBtnReading = digitalRead(BUTTON_DOWN);
  if (downBtnReading != downBtnState_last) {
    lastDebounceTime_down = millis();
  }
  if ((millis() - lastDebounceTime_down) > debounceDelay) {
    if (downBtnReading != downBtnState) {
      downBtnState = downBtnReading;
      downBtnPressed = ( downBtnState == LOW );
    }
  }
  downBtnState_last = downBtnReading;
}

//-------------------------------------------------------------------

void updateTimeDisplay() {
  display.drawRoundRect(0, 0, display.width()-1, 25, 1, OLED_WHITE);
  display.fillRect(1, 1, display.width()-3, 22, OLED_BLACK);
  display.setTextSize(2);
  display.setTextColor(OLED_WHITE);
  display.setCursor(18,5);
  //display.print(formatDigits(timeNow.hour(),0) + ":" + formatDigits(timeNow.minute(),1) + " " + formatDigits(timeNow.second(),1));
  display.print(formatDigits(timeNow.hour(),0));
  display.print(":");
  display.print(formatDigits(timeNow.minute(),1));
  display.print(" ");
  display.print(formatDigits(timeNow.second(),1));
  display.display();
}

//-------------------------------------------------------------------

void updateStatusDisplay() {
  // Next sun event
  display.fillRect(0, 32, display.width()-1, 30, OLED_BLACK);
  display.drawRoundRect(0, 32, 70, 30, 1, OLED_WHITE);
  display.fillRect(13, 29, 46, 6, OLED_BLACK);
  display.setTextSize(1);
  display.setTextColor(OLED_WHITE);
  display.setCursor(16,29);
  strcpy_P(buffer, (char*)pgm_read_word(&(sunEvents[nextSunEvent])));
  display.print(buffer);
  display.setTextSize(2);
  display.setCursor(6,42);
  if (nextSunEvent == 0) {
    display.print(formatDigits(sunriseHr,0));
    display.print(":");
    display.print(formatDigits(sunriseMin,1));
  }
  else {
    display.print(formatDigits(sunsetHr,0));
    display.print(":");
    display.print(formatDigits(sunsetMin,1));
  }

  // Relay status
  display.drawRoundRect(74, 32, 52, 30, 1, OLED_WHITE);
  display.fillRect(82, 29, 36, 6, OLED_BLACK);
  display.setTextSize(1);
  display.setTextColor(OLED_WHITE);
  display.setCursor(83,29);
  display.print(F("OUTPUT"));
  display.setTextSize(2);
  display.setCursor(80,42);
  if (relayOn)
    display.print(F("On"));
  else
    display.print(F("Off"));

  display.display();
}

//-------------------------------------------------------------------

void updateSetDisplay() {
  byte hdrIdx;
  byte invWidth;
  byte invHeight = 18;
  byte invX;
  byte invY;
  char cstr[6];

  display.clearDisplay();
  
  if (displayMode == MODE_SELECT) {
    hdrIdx = 0; // SELECT
    display.setTextSize(2);
    display.setCursor(0,22);
    strcpy_P(buffer, (char*)pgm_read_word(&(selectParams[0])));
    display.print(buffer);
    display.setCursor(0,44);
    strcpy_P(buffer, (char*)pgm_read_word(&(selectParams[1])));
    display.print(buffer);
  }
  else if (displayMode == MODE_SETTINGS) {
    hdrIdx = 1; // SETTINGS
    display.setTextSize(2);
    display.setCursor(0,44);
    display.print(F("> "));
    display.setCursor(20,44);
    if (curPos == 1) {
      strcpy_P(buffer, (char*)pgm_read_word(&(setParams[0])));
      if (settings.onSunrise)
        display.print(F("Enabled"));
      else
        display.print(F("Disabled"));
    }
    else if (curPos == 2 or curPos == 3 or curPos == 4) {
      strcpy_P(buffer, (char*)pgm_read_word(&(setParams[1])));
      if (settings.sunriseOffset < 0)
        display.print(F("-"));
      else
        display.print(F("+"));
      sprintf(cstr, "%1d:%02d", sunriseOffsetHr, sunriseOffsetMin);
      display.print(cstr);
    }
    else if (curPos == 5 or curPos == 6) {
      strcpy_P(buffer, (char*)pgm_read_word(&(setParams[2])));
      sprintf(cstr, "%2d:%02d", sunriseOnHr, sunriseOnMin);
      display.print(cstr);
    }
    else if (curPos == 7) {
      strcpy_P(buffer, (char*)pgm_read_word(&(setParams[3])));
      if (settings.onSunset)
        display.print(F("Enabled"));
      else
        display.print(F("Disabled"));
    }
    else if (curPos == 8 or curPos == 9 or curPos == 10) {
      strcpy_P(buffer, (char*)pgm_read_word(&(setParams[4])));
      if (settings.sunsetOffset < 0)
        display.print(F("-"));
      else
        display.print(F("+"));
      sprintf(cstr, "%1d:%02d", sunsetOffsetHr, sunsetOffsetMin);
      display.print(cstr);
    }
    else if (curPos == 11 or curPos == 12) {
      strcpy_P(buffer, (char*)pgm_read_word(&(setParams[5])));
      sprintf(cstr, "%2d:%02d", sunsetOffHr, sunsetOffMin);
      display.print(cstr);
    }
    display.setCursor(0,22);
    display.print(buffer);
  }
  else if (displayMode == MODE_ADJUST) {
    hdrIdx = 2; // ADJUST TIME
    display.setTextSize(2);
    display.setCursor(0,44);
    display.print(F("> "));
    display.setCursor(20,44);
    if (curPos == 1) {
      strcpy_P(buffer, (char*)pgm_read_word(&(adjParams[0])));
      sprintf(cstr, "%2d", adjustedTime.hour);
      display.print(cstr);
    }
    else if (curPos == 2) {
      strcpy_P(buffer, (char*)pgm_read_word(&(adjParams[1])));
      sprintf(cstr, "%02d", adjustedTime.minute);
      display.print(cstr);
    }
    else if (curPos == 3) {
      strcpy_P(buffer, (char*)pgm_read_word(&(adjParams[2])));
      sprintf(cstr, "%4d", adjustedTime.year);
      display.print(cstr);
    }
    else if (curPos == 4) {
      strcpy_P(buffer, (char*)pgm_read_word(&(adjParams[3])));
      sprintf(cstr, "%02d", adjustedTime.month);
      display.print(cstr);
    }
    else if (curPos == 5) {
      strcpy_P(buffer, (char*)pgm_read_word(&(adjParams[4])));
      sprintf(cstr, "%02d", adjustedTime.day);
      display.print(cstr);
    }
    else if (curPos == 6) {
      strcpy_P(buffer, (char*)pgm_read_word(&(adjParams[5])));
      if (settings.dstFlag)
        display.print(F("Active"));
      else
        display.print(F("Inactive"));
    }
    display.setCursor(0,22);
    display.print(buffer);
  }

  display.setTextSize(2);
  display.setTextColor(OLED_WHITE);
  display.setCursor(0,0);
  strcpy_P(buffer, (char*)pgm_read_word(&(settingHeaders[hdrIdx])));
  display.print(buffer);

  // Inverse actual setting
  if (displayMode == MODE_SELECT) {
    if (curPos == 1) {
      invX = 0;
      invY = 20;
      invWidth = 100;
    }
    else {
      invX = 0;
      invY = 42;
      invWidth = 110;
    }
  }  
  else if (displayMode == MODE_SETTINGS) {
    if (curPos == 1 or curPos == 7) {
      invX = 18;
      invY = 42;
      invWidth = 100;
    }
    else if (curPos == 2 or curPos == 8) {
      invX = 18;
      invY = 42;
      invWidth = 13;
    }
    else if (curPos == 3 or curPos == 9) {
      invX = 31;
      invY = 42;
      invWidth = 13;
    }
    else if (curPos == 5 or curPos == 11) {
      invX = 18;
      invY = 42;
      invWidth = 26;
    }
    else if (curPos == 4 or curPos == 6 or curPos == 10 or curPos == 12) {
      invX = 54;
      invY = 42;
      invWidth = 26;
    }
  }  
  else if (displayMode == MODE_ADJUST) {
    if (curPos == 1 or curPos == 2 or curPos == 4 or curPos == 5) {
      invX = 18;
      invY = 42;
      invWidth = 26;
    }
    else if (curPos == 3) {
      invX = 18;
      invY = 42;
      invWidth = 52;
    }
    else if (curPos == 6) {
      invX = 18;
      invY = 42;
      invWidth = 100;
    }
  }
  
  display.fillRect(invX,invY,invWidth,invHeight,OLED_INVERSE);
  display.display();
}

//-------------------------------------------------------------------

void updateSunEvents() {
  int timeInMinutes = timeNow.hour()*60 + timeNow.minute();
  DateTime nextDay;
  sunrise  = myLocation.sunrise(timeNow.year(), timeNow.month(), timeNow.day(), settings.dstFlag);
  sunset   = myLocation.sunset(timeNow.year(), timeNow.month(), timeNow.day(), settings.dstFlag);
  if (timeInMinutes > 720) {
    nextDay = timeNow + TimeSpan(1,0,0,0);
    sunrise  = myLocation.sunrise(nextDay.year(), nextDay.month(), nextDay.day(), settings.dstFlag);
  }
  sunriseHr = sunrise / 60;
  sunriseMin = sunrise - (sunriseHr * 60) ;
  sunsetHr = sunset / 60;
  sunsetMin = sunset - (sunsetHr * 60) ;

  //Determine which is the next event
  if ( (timeNow.hour() == sunriseHr and timeNow.minute() >= sunriseMin) or (timeNow.hour() > sunriseHr and timeNow.hour() < sunsetHr) or (timeNow.hour() == sunsetHr and timeNow.minute() < sunsetMin) )
    nextSunEvent = 1;
  else
    nextSunEvent = 0;
}

//-------------------------------------------------------------------

void updateSunEventSettings() {
  char cstr[10];
  sunriseOffsetHr = abs(settings.sunriseOffset / 60);
  sunriseOffsetMin = abs(settings.sunriseOffset) - (sunriseOffsetHr * 60);
  sunriseOnHr = settings.sunriseTime / 60;
  sunriseOnMin = settings.sunriseTime - (sunriseOnHr * 60);

  sunsetOffsetHr = abs(settings.sunsetOffset / 60);
  sunsetOffsetMin = abs(settings.sunsetOffset) - (sunsetOffsetHr * 60);
  sunsetOffHr = settings.sunsetTime / 60;
  sunsetOffMin = settings.sunsetTime - (sunsetOffHr * 60);

//  sprintf(cstr, "%02d", settings.sunriseOffset);
//  Serial.println(cstr);
//  sprintf(cstr, "%02d", sunriseOffsetMin);
//  Serial.println(cstr);
}
//-------------------------------------------------------------------

void updateTime() {
  RTC.adjust(DateTime(adjustedTime.year, adjustedTime.month, adjustedTime.day, adjustedTime.hour, adjustedTime.minute, 0));
}

//-------------------------------------------------------------------

void calculateSettingValues() {
  if (settings.sunriseOffset < 0)
    settings.sunriseOffset = -(sunriseOffsetHr*60 + sunriseOffsetMin);
  else
    settings.sunriseOffset = (sunriseOffsetHr*60 + sunriseOffsetMin);
  settings.sunriseTime = (sunriseOnHr*60 + sunriseOnMin);

  if (settings.sunsetOffset < 0)
    settings.sunsetOffset = -(sunsetOffsetHr*60 + sunsetOffsetMin);
  else
    settings.sunsetOffset = (sunsetOffsetHr*60 + sunsetOffsetMin);
  settings.sunsetTime = (sunsetOffHr*60 + sunsetOffMin);
}

//-------------------------------------------------------------------

char* formatDigits(byte digits, byte mode) {
  static char str[3];
  if (mode == 0) {
    sprintf(str, "%2d", digits);
    return str;
  }
  else if (mode == 1) {
    sprintf(str, "%02d", digits);
    return str;
  }
}

//-------------------------------------------------------------------
