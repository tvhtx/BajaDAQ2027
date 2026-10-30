// BAJA Daq Teensy master code 2025 Season
//////////////////////////////////////////
// Author: Daniel VanWiggeren
//  Last edit 02/25/2026

// Edit 1: File created, pin definitions, libraries, setup
// Edit 2: Oops forgot to log the last couple. Got all data and state machine
// done.
//               Still need: accelerometer, screen, UI stuff, timer stuff
// Edit 3: Finished screen UI timer
//              Still need: UI improvements (test & add more than just text),
//              accelerometer

#include "PCF8575.h"
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <SD.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Wire.h>
// include new screen library
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
// include accelerometer
//

#define TIME_ZONE_OFFSET 5 // 7 for arizona
// optional features. un-comment if needed
#define useSD
char sd_str[500];
// #define secondaryDisplays
// #define teensySlave            //put address here
#define esp32Slave 0x27 // put address here
#define OLED_ADDRESS 0x3C
// Tire diameter 21" -> circumference (pi*21)/12 ft = 5.4978 ft
// 1 rpm = 5.4978 ft/min * 60 / 5280 = 0.06248 mph
const float RPM_TO_MPH = 0.06248f;
const uint8_t CVT_PULSES_PER_REV = 8;
const uint8_t WHEEL_PULSES_PER_REV = 4;

PCF8575 PCF(0x27);
uint8_t but1 = 1;
uint8_t but2 = 1;
uint8_t but3 = 1;
uint8_t but4 = 1;
uint8_t but5 = 1;
uint8_t but6 = 1;

uint8_t but1p = 1;
uint8_t but2p = 1;
uint8_t but3p = 1;
uint8_t but4p = 1;
uint8_t but5p = 1;

uint8_t but1s = 1;
uint8_t but2s = 1;
uint8_t but3s = 1;
uint8_t but4s = 1;
uint8_t but5s = 1;

uint8_t isFalling(uint8_t prv, uint8_t cur) {
  if ((prv == 1) && (cur == 0)) {
    return 0;
  } else {
    return 1;
  }
}
// create a state machine based on buttons
typedef enum {
  SPEEDOMETER = 1,
  LAP_TIMER = 2,
  TEMP_MONITOR = 3,
  BROKE_DOWN = 4,
  TEMP_EMERGENCY = 5,
  CHECK_GAUGES = 6,
  FUEL_TIMER = 7
} DISPLAY_TYPE;
typedef enum {
  INITIAL = 1,
  POWERTRAIN = 2,
  SUSP = 3,
  FUEL = 4,
  OTHER = 5
} BROKEN_TYPE;

BROKEN_TYPE john = INITIAL;
DISPLAY_TYPE mode = SPEEDOMETER;
DISPLAY_TYPE back = SPEEDOMETER;
DISPLAY_TYPE prev = SPEEDOMETER;

typedef enum { PINITIAL = 1, ENGINE = 2, GEAR = 3, DIFF = 4, CV = 5 } PWR_TYPE;
PWR_TYPE pwr = PINITIAL;

typedef enum { SINITIAL = 1, SHOCK = 2, HUB = 3, FCA = 4, RCA = 5 } SUSP_TYPE;

SUSP_TYPE sus = SINITIAL;

// Temperature sensor stuff
// ---------------------------------------------------------
const int temp_sensor_1 = 16; // gearbox temp
const int temp_sensor_2 = 15; // cvt temp ???
const int temp_sensor_3 = 14; // spare ???
// Set up cvt temp sensor

bool heatReset = false;

OneWire temp1(temp_sensor_2);
DallasTemperature cvtTemp(&temp1);
float cvtTempF;
const int cvtWarningTemp = 180;
// Set up engine temp sensor
OneWire temp2(temp_sensor_3);
DallasTemperature frontDiffTemp(&temp2);
float frontDiffTempF;
const int frontDiffWarningTemp = 180;
// Spare temp sensor
OneWire temp3(temp_sensor_1);
DallasTemperature gearboxTemp(&temp3);
float gearboxTempF;
const int gearboxWarningTemp = 180;

int hot = 0;

// GPS stuff
// ----------------------------------------------------------------------
TinyGPS gps;
SoftwareSerial ss(0, 1);
float gpsSpeed;
float gpsMaxSpeed = 0;
bool newData = false;
int year;
byte month, day, hour = 0, minute = 0, second = 0, hundredths;
byte prevSec = 0;
// Possibly do time stuff here. If not figure out a damn better way cause ugh.

// Hall effect tach stuff
// ----------------------------------------------------------
const int hall_effect_cvt = 41;   // CVT
const int hall_effect_wheel = 40; // Left Rear Wheel

volatile byte cvt_tick_count;
unsigned long rpm_cvt = 0;
unsigned long timeold_cvt = 0;

volatile byte wheel_tick_count;
unsigned long rpm_wheel = 0;
unsigned long timeold_wheel = 0;

void cvt_tick() { cvt_tick_count++; }

void wheel_tick() { wheel_tick_count++; }

void restart() { setup(); }
float mphTach;
float mphTachMax = 0;
unsigned long rpm = 0; // For display compatibility

// remember to set mode as input in setup

// secondary displays
// stuff-------------------------------------------------------
#ifdef secondaryDisplays
LiquidCrystal_I2C lcd1(0X27, 20, 4);
LiquidCrystal_I2C lcd2(0x28, 20, 4); /// verify i2c address
// LiquidCrystal_I2C lcd3(0x???, 20, 4);
#endif

// New OLED
// display---------------------------------------------------------------
SSD1306AsciiWire oled;

// Accelerometer
// -----------------------------------------------------------------

// SD Card
// -----------------------------------------------------------------------
const int chipSelect = BUILTIN_SDCARD;
int fileNumber = 0;
File dataFile;
char fname[50];

// Slaves
// ------------------------------------------------------------------------
char stats[150];
char screen[150];
char telemetry[150];

// Timers:

unsigned long int tStartMillis = 0;
unsigned long int tMillis;
unsigned long int tBest = 4294967295;
unsigned long int tLap, tTemp;
unsigned long int fStartMillis = 0;
unsigned long int ftimeMillis = 3600000;

int fhour = 1, fminute = 0, fsecond = 0;
int thour, tminute, tsecond;
int thourbest = 0;
int tminutebest = 0;
int tsecondbest = 0;
unsigned long int timeLeft;
unsigned long brokenStart = 0;

void setup() {
  Wire.begin();
  Wire.setClock(400000L);
  // Temp sensors
  cvtTemp.begin();
  frontDiffTemp.begin();
  gearboxTemp.begin(); // remmeber to change if spare temp changes

  Serial.begin(115200);
  // GPS
  ss.begin(9600);

  mode = SPEEDOMETER;
  // Hall effect sensors -- bare 3-wire hall ICs need pullups
  pinMode(hall_effect_cvt, INPUT_PULLUP);
  pinMode(hall_effect_wheel, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), restart, FALLING);

  attachInterrupt(digitalPinToInterrupt(hall_effect_cvt), cvt_tick, FALLING);
  cvt_tick_count = 0;
  rpm_cvt = 0;
  timeold_cvt = 0;

  attachInterrupt(digitalPinToInterrupt(hall_effect_wheel), wheel_tick, FALLING);
  wheel_tick_count = 0;
  rpm_wheel = 0;
  timeold_wheel = 0;
// Secondary Displays -//unused
#ifdef secondaryDisplays
  lcd1.init();
  lcd1.backlight();
  lcd2.init();
  lcd2.backlight();
#endif

  // OLED
  oled.begin(&Adafruit128x64, OLED_ADDRESS);
  oled.setFont(Adafruit5x7);

  // Buttons-
  PCF.begin();
  PCF.write16(0xFFFF);   // set every pin HIGH so they can be read as inputs

  // Accelerometer------------------------------------------------------
  // dostuff

  // SD Card
  SD.begin(chipSelect);
  sprintf(fname, "tach_test_%04d.txt", fileNumber);
  while (SD.exists(fname)) {
    fileNumber++;
    sprintf(fname, "tach_test_%04d.txt", fileNumber);
  }

  dataFile = SD.open(fname, FILE_WRITE);
  dataFile.println(
      "hr:min:sec,millis,mphTach,maxMphTach,rpmCvt,rpmWheel,gpsSpeed,"
      "maxGpsSpeed,cvtTempF,frontDiffTempF,gearboxTempF");
  dataFile.close();

  hour = 0;
  minute = 0;
  second = 0;

  // print something to screens to say initializing  to make sure everything is
  // set up
  oled.clear();
  oled.println("Initializing");
  delay(1000);
}

void loop() {
  but1p = but1s;
  but2p = but2s;
  but3p = but3s;
  but4p = but4s;
  but5p = but5s;
  // Buttons -----------1 = not pressed, 0=pressed---------------------
  but1s = PCF.readButton(1);
  but2s = PCF.readButton(0);
  but3s = PCF.readButton(2);
  but4s = PCF.readButton(3);
  but5s = PCF.readButton(4);
  // but6=PCF.readButton(5);

  Serial.printf("b1s=%d b2s=%d b3s=%d b4s=%d b5s=%d  mode=%d\n",
                but1s, but2s, but3s, but4s, but5s, mode);

  but1 = isFalling(but1p, but1s);
  but2 = isFalling(but2p, but2s);
  but3 = isFalling(but3p, but3s);
  but4 = isFalling(but4p, but4s);
  but5 = isFalling(but5p, but5s);

  if (mode != BROKE_DOWN) {

    // Temp sensors ---------------------------------------------------------
    cvtTemp.requestTemperatures();
    cvtTempF = cvtTemp.getTempFByIndex(0);

    frontDiffTemp.requestTemperatures();
    frontDiffTempF = frontDiffTemp.getTempFByIndex(0);

    gearboxTemp.requestTemperatures();
    gearboxTempF = gearboxTemp.getTempFByIndex(0);

    // if temp > warning to temp show proper warning and then set the state to
    // temperatures
    if ((cvtTempF > cvtWarningTemp) ||
        (frontDiffTempF > frontDiffWarningTemp) ||
        (gearboxTempF > gearboxWarningTemp)) {
      hot = 0;
      back = mode;
      if (!heatReset) {
        mode = TEMP_EMERGENCY;
      }
      if ((cvtTempF > cvtWarningTemp)) {
        if ((frontDiffTempF > frontDiffWarningTemp) ||
            (gearboxTempF > gearboxWarningTemp)) {
          hot = -1;
        } else {
          hot = 1;
        }
      } else if ((frontDiffTempF > frontDiffWarningTemp)) {
        if ((gearboxTempF > gearboxWarningTemp)) {
          hot = -1;
        } else {
          hot = 2;
        }
      } else {
        hot = 3;
      }
    }

    // GPS -----------------------------------------------------------------
    newData = false;
    // For one second we parse GPS data and report some key values (NAY WANT TO
    // DO LESS THAN ONE SECOND)
    for (unsigned long start = millis(); millis() - start < 300;) {
      while (ss.available()) {
        char c = ss.read();
        // Serial.write(c); // uncomment this line if you want to see the GPS
        // data flowing
        if (gps.encode(c)) // Did a new valid sentence come in?
          newData = true;
      }
    }

    if (newData) {
      float flat, flon;
      unsigned long age;
      gps.f_get_position(&flat, &flon, &age);
      gpsSpeed = gps.f_speed_mph();
      if (gpsSpeed > gpsMaxSpeed) {
        gpsMaxSpeed = gpsSpeed;
      }
    }
    char sz[12];
    unsigned long age;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second,
                       &hundredths, &age);
    hour = hour - TIME_ZONE_OFFSET;
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }

    if (age != TinyGPS::GPS_INVALID_AGE) {

      sprintf(sz, "%02d:%02d:%02d", hour, minute, second);

    } else {
      sprintf(sz, "-1");
    }
    Serial.println(sz);

    // tachometer -- pulses are divided by features-per-rev to get true RPM
    detachInterrupt(digitalPinToInterrupt(hall_effect_cvt));
    unsigned long now = millis();
    unsigned long diff = now - timeold_cvt;
    if (diff > 0) {
      rpm_cvt = (unsigned long)((60000.0 * cvt_tick_count) /
                                (diff * CVT_PULSES_PER_REV));
    }
    rpm = rpm_cvt; // Alias for display consistency
    cvt_tick_count = 0;
    timeold_cvt = now;
    attachInterrupt(digitalPinToInterrupt(hall_effect_cvt), cvt_tick, FALLING);

    detachInterrupt(digitalPinToInterrupt(hall_effect_wheel));
    now = millis();
    diff = now - timeold_wheel;
    if (diff > 0) {
      rpm_wheel = (unsigned long)((60000.0 * wheel_tick_count) /
                                  (diff * WHEEL_PULSES_PER_REV));
    }
    wheel_tick_count = 0;
    timeold_wheel = now;
    attachInterrupt(digitalPinToInterrupt(hall_effect_wheel), wheel_tick, FALLING);

    mphTach = rpm_wheel * RPM_TO_MPH;
    if (mphTach > mphTachMax) {
      mphTachMax = mphTach;
    }
    // accelerometer

    // FUEL
    if (ftimeMillis <= (millis() - fStartMillis) + 999) {
      fStartMillis = 69;
      mode = FUEL_TIMER;
    }

    // EMERGENCY
    if (but1s == 0 && but5s == 0) {
      but1 = 1;
      but5 = 1;
      if (brokenStart == 0) {
        brokenStart = millis();
      } else {
        if (millis() >= brokenStart + 3000) {
          if (mode != BROKE_DOWN)
            back = mode;
          mode = BROKE_DOWN;
        }
      }
    }
  }
  oled.clear();

  switch (mode) {
  case SPEEDOMETER: // SPEED
                    // ----------------------------------------------------------------------------------------------------------------
    sprintf(stats,
            "%02d:%02d:%02d SPEED\n %.2f mph,  %.2f gps\n%.2f max, %.2f gps",
            hour, minute, second, mphTach, gpsSpeed, mphTachMax, gpsMaxSpeed);
    oled.set2X();
    sprintf(screen, "%.2fmph\n%ld rpm", mphTach, rpm);
    oled.println(screen);
    oled.set1X();
    sprintf(screen, "max: %.2f mph\n\nGPS: %.2fmph\n      %02d:%02d:%02d",
            mphTachMax, gpsSpeed, hour, minute, second);

    oled.println(screen);

    if (but1 == 0) {
      mode = LAP_TIMER;
    } else if (but5 == 0) {
      mode = FUEL_TIMER;
    }
    break;

  case LAP_TIMER: // TIME
                  // -----------------------------------------------------------------------------------------------------------------------
    sprintf(stats, "%02d:%02d:%02d TIMER\n", hour, minute, second);
    if (tStartMillis == 0) {
      sprintf(screen, "Timer start?\n press3 (4=lap)");
      if (but3 == 0) {
        tStartMillis = millis();
        tLap = tStartMillis;
      }
    } else {
      tMillis = millis() - tStartMillis;

      thour = tMillis / 3600000;
      tminute = (tMillis / 60000) % 60;
      tsecond = (tMillis / 1000) % 60;
      if (but4 == 0) {
        tTemp = tMillis - tLap;
        if (tTemp < tBest) {
          thourbest = thour - thourbest;
          tminutebest = (tminute - tminutebest) % 60;
          tsecondbest = (tsecond - tsecondbest) % 60;
          tBest = tTemp;
        }
        tLap = millis();
      }
      sprintf(screen, "%02d:%02d:%02dcurrent\n\n%02d:%02d:%02d best", thour,
              tminute, tsecond, thourbest, tminutebest, tsecondbest);
    }
    oled.println(screen);
    sprintf(screen, "\n%.2f mph %ld rpm\n%02d:%02d:%02d", mphTach, rpm, hour,
            minute, second);
    oled.println(screen);
    if (but1 == 0) {
      mode = TEMP_MONITOR;
    } else if (but5 == 0) {
      mode = SPEEDOMETER;
    }
    break;

  case TEMP_MONITOR: // TEMP
                     // ------------------------------------------------------------------------------------------------------------------
    sprintf(stats, "%02d:%02d:%02d TEMPS\n%.2fCVT %.2fgear \n%.2f mph", hour,
            minute, second, cvtTempF, gearboxTempF, mphTach);
    sprintf(screen,
            "TEMPS\n%.2fCVT \n%.2fgear \n%.2f mph %ld rpm\nGPS: %.2fmph ",
            cvtTempF, gearboxTempF, mphTach, rpm, gpsSpeed);

    oled.println(screen);
    if (but1 == 0) {
      mode = FUEL_TIMER;
    } else if (but5 == 0) {
      mode = LAP_TIMER;
    }
    break;

  case BROKE_DOWN: // O NO
                   // -------------------------------------------------------------------------------------------------------

    switch (john) {
    case INITIAL:
      sprintf(stats, "%02d:%02d:%02d BROKEN\n", hour, minute, second);
      sprintf(screen, "Broken :(\n1power 2susp 3gas\n4other 5nevermind");
      if (but1 == 0) {
        john = POWERTRAIN;
      }
      if (but2 == 0) {
        john = SUSP;
      }
      if (but3 == 0) {
        john = FUEL;
      }
      if (but4 == 0) {
        john = OTHER;
      }
      break;
    case POWERTRAIN:
      switch (pwr) {
      case PINITIAL:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nPOWERTRAIN", hour, minute,
                second);
        sprintf(screen,
                "POWERTRAIN broke \n1engine 2gearbox\n3diff 4cv/prop\n5:back");
        if (but1 == 0) {
          pwr = ENGINE;
        } else if (but2 == 0) {
          pwr = GEAR;
        } else if (but3 == 0) {
          pwr = DIFF;
        } else if (but4 == 0) {
          pwr = CV;
        }
        if (but5 == 0) {
          john = INITIAL;
        }
        break;
      case ENGINE:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nPOWERTRAIN\nENGINE BROKE", hour,
                minute, second);
        sprintf(screen, "Engine Broke \n Sorry man \5 back");
        if (but5 == 0) {
          john = INITIAL;
          pwr = PINITIAL;
        }
        break;
      case GEAR:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nPOWERTRAIN\nGEARBOX BROKE", hour,
                minute, second);
        sprintf(screen, "Gearbox Broke \n Sorry man \5 back");
        if (but5 == 0) {
          john = INITIAL;
          pwr = PINITIAL;
        }
        break;
      case DIFF:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nPOWERTRAIN\nDIFF BROKE", hour,
                minute, second);
        sprintf(screen, "Diff Broke \n Sorry man \5 back");
        if (but5 == 0) {
          john = INITIAL;
          pwr = PINITIAL;
        }
        break;
      case CV:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nPOWERTRAIN\nCV/PROP BROKE", hour,
                minute, second);
        sprintf(screen, "CV or Prop Broke \n Sorry man \5 back");
        if (but5 == 0) {
          john = INITIAL;
          pwr = PINITIAL;
        }
        break;
      }
      break;
    case SUSP:
      switch (sus) {
      case SINITIAL:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nSUSP", hour, minute, second);
        sprintf(screen,
                "SUSP Broke\n1shock 2hub/wheel\n3frontCA 4rearCA\n 5 back");
        if (but1 == 0) {
          sus = SHOCK;
        } else if (but2 == 0) {
          sus = HUB;
        } else if (but3 == 0) {
          sus = FCA;
        } else if (but4 == 0) {
          sus = RCA;
        }
        if (but5 == 0) {
          john = INITIAL;
        }
        break;
      case SHOCK:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nSUSP \nSHOCK BROKE", hour,
                minute, second);
        sprintf(screen, "Shock Broke \n Sit tight \n 5 back");
        if (but5 == 0) {
          john = INITIAL;
          sus = SINITIAL;
        }
        break;
      case HUB:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nSUSP \nHUB BROKE", hour, minute,
                second);
        sprintf(screen, "Hubs/Wheel Broke \n Sit tight \n 5 back");
        if (but5 == 0) {
          john = INITIAL;
          sus = SINITIAL;
        }
        break;
      case FCA:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nSUSP \nFront Control Arm", hour,
                minute, second);
        sprintf(screen, "Front Control Arm \n Sit tight \n 5 back");
        if (but5 == 0) {
          john = INITIAL;
          sus = SINITIAL;
        }
        break;
      case RCA:
        sprintf(stats, "%02d:%02d:%02d BROKEN\nSUSP \nRear Control Arm", hour,
                minute, second);
        sprintf(screen, "Rear Control Arm \n Sit tight \n 5 back");
        if (but5 == 0) {
          john = INITIAL;
          sus = SINITIAL;
        }
        break;
      }

      break;
    case FUEL:
      sprintf(stats, "%02d:%02d:%02d BROKEN\nOutta Gas ", hour, minute, second);
      sprintf(screen, "Whoopsies\nOutta Gas\nNot again\n5 back");
      if (but5 == 0) {
        john = INITIAL;
      }
      break;
    case OTHER:
      sprintf(stats, "%02d:%02d:%02d BROKEN\nOTHER", hour, minute, second);
      sprintf(screen, "Something Else broke\nRadio plz?\n 5 back");
      if (but5 == 0) {
        john = INITIAL;
      }
      break;
    }
    oled.println(screen);
    delay(300);
    if (but5 == 0 && john == INITIAL) {
      mode = back;
      brokenStart = 0;
    }
    break;

  case TEMP_EMERGENCY: // TEMP BAD
                       // -----------------------------------------------------------------------------------------------

    if (hot == -1) {
      sprintf(stats,
              "%02d:%02d:%02d CAR HOT!\nSpeed: %.2f\n Multiple: cgd\n%.2f, "
              "%.2f, %.2f",
              hour, minute, second, mphTach, cvtTempF, gearboxTempF,
              frontDiffTempF);
      sprintf(screen, "Multiple systems hot\n\nb1 or b5 to reset");
    } else if (hot == 1) {
      /// cvt
      sprintf(stats, "%02d:%02d:%02d CAR HOT!\nSpeed: %.2f\n CVT HOT: \n%.2f",
              hour, minute, second, mphTach, cvtTempF);
      sprintf(screen, "CVT hot: \n %.2f F", cvtTempF);
    } else if (hot == 2) {
      // front diff
      sprintf(stats, "%02d:%02d:%02d CAR HOT!\nSpeed: %.2f\n Diff HOT: \n%.2f",
              hour, minute, second, mphTach, frontDiffTempF);
      sprintf(screen, "Front Diff hot: \n %.2f F", frontDiffTempF);
    } else if (hot == 3) {
      // gearbox
      sprintf(stats,
              "%02d:%02d:%02d CAR HOT!\nSpeed: %.2f\n Gearbox HOT: \n%.2f",
              hour, minute, second, mphTach, gearboxTempF);
      sprintf(screen, "Gearbox hot: \n %.2f F", gearboxTempF);
    } else {
      sprintf(stats,
              "%02d:%02d:%02d CAR HOT!\nSpeed: %.2f\n UNKNOWN ISSUE: "
              "cgd\n%.2f, %.2f, %.2f",
              hour, minute, second, mphTach, cvtTempF, gearboxTempF,
              frontDiffTempF);
      sprintf(screen, "Something is hot\n\nb1 or b5 to reset");
    }
    oled.println(screen);
    if ((but1 == 0) || (but5 == 0)) {
      mode = back;
      heatReset = true;
    }
    break;
  case FUEL_TIMER: // GAS
                   // --------------------------------------------------------------------------------------------------------
    if (fStartMillis == 0) {
      sprintf(stats, "%02d:%02d:%02d SETUP GAS\n Setup fuel", hour, minute,
              second);
      sprintf(screen,
              "Start fuel timer?\n%02d:%02d:%02d +b3 -b4\nstart&back b5", fhour,
              fminute, fsecond);

      oled.println(screen);
      if (but3 == 0) {
        fminute = fminute + 5;
        if (fminute >= 60) {
          fhour++;
          fminute = 0;
        }
      } else if (but4 == 0) {
        fminute = fminute - 5;
        if (fminute < 0) {
          fhour--;
          fminute = 55;
        }
      }
      if (fhour < 0) {
        fhour = 0;
      }
      if (but5 == 0) {
        fStartMillis = millis();
        ftimeMillis = (fhour * 3600000) + (fminute * 60000) + (fsecond * 100);
      }
    } else if (fStartMillis == 69) {
      sprintf(stats, "%02d:%02d:%02d \n OUT OF GAS!", hour, minute, second);
      sprintf(screen, "Pit soon buckaroo \npress 3 to reset");
      oled.set2X();
      oled.println(screen);
      oled.set1X();

      if (but3 == 0) {
        fStartMillis = 0;
        fhour = 1;
        fminute = 0;
        fsecond = 0;
      }
    } else {
      timeLeft = ftimeMillis - (millis() - fStartMillis);
      fhour = (timeLeft / 3600000);
      fminute = (timeLeft / 60000) % 60;
      fsecond = (timeLeft / 1000) % 60;
      sprintf(stats, "%02d:%02d:%02d\n Checking Gas:\n%02d:%02d:%02d", hour,
              minute, second, fhour, fminute, fsecond);
      oled.set2X();
      sprintf(screen, "%02d:%02d:%02d\nRemaining", fhour, fminute, fsecond);
      oled.println(screen);
      oled.set1X();
      sprintf(screen, "%.2f mph, %ld rpm", mphTach, rpm);
      oled.println(screen);
    }

    if (but1 == 0) {
      mode = SPEEDOMETER;
    } else if (but5 == 0) {
      mode = TEMP_MONITOR;
    }
    break;

  default: // hmmmmmmmmmmmmmmmm----------------------------------------------------------------------------------------------------
    mode = SPEEDOMETER;
    break;

    prev = mode;
  }

  // sprintf(stats,"%02d:%02d:%02d\n%.2f  %.2f\n%.2fcvt\n%.2f",hour, minute,
  // second,gpsSpeed,gpsMaxSpeed,cvtTemp,gearboxTemp);

#ifdef useSD
  dataFile = SD.open(fname, FILE_WRITE);
  //"hr:min:sec, millis, tach speed, max tach speed, engine rpm, gps speed, max
  //gps speed, cvtTemp, engineTemp, gearboxTemp"
  if (mode != BROKE_DOWN) {

    sprintf(sd_str,
            "%02d:%02d:%02d,%lu,%.2f,%.2f,%lu,%lu,%.2f,%.2f,%.3f,%.3f,%.3f",
            hour, minute, second, millis(), mphTach, mphTachMax, rpm_cvt,
            rpm_wheel, gpsSpeed, gpsMaxSpeed, cvtTempF, frontDiffTempF,
            gearboxTempF);

  } else {
    sprintf(sd_str, "Car broke down....see outside documentation");
  }
  dataFile.println(sd_str);
  dataFile.close();
#endif

  sprintf(telemetry, "%02d:%02d:%02d,%lu,%.1f,%.1f,%lu,%lu,%.1f,%.1f,%.1f", hour, minute, second, millis(), mphTach, gpsSpeed, rpm_cvt,rpm_wheel, cvtTempF, frontDiffTempF, gearboxTempF);

  Wire.beginTransmission(0x69);
  Wire.write(telemetry);
  Wire.endTransmission();
}
