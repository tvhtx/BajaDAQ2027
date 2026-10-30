//BAJA Steering Wheel Testing Environment 25-26
// Taylor Van Horn - Alex Rocque
//Last Edit: 10/25/2025

/*NOTE: IF YOU ARE WORKING ON YOUR LAPTOP AS SOMEONE NEW TO THIS IDE OR SKETCH
 YOU WILL NEED TO NAVIGATE TO THE LIBRARY MANAGER WITHIN ARDUINO IDE ON THE LEFT HAND NAVIGATION COLUMN ITS THE ONE WITH BOOKS
 AND SEARCH FOR EACH INDIVIDUAL LIBRARY LISTED HERE BEFORE YOU CAN RUN THIS PROGRAM AND TEST ANYTHING*/

#include "Wire.h"
#include "U8g2lib.h"
#include "LiquidCrystal_I2C.h"
#include "PCF8575.h"
//include new screen library
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

// I2C Addresses
#define PCF8575_ADDR 0x27
#define OLED_ADDR    0x3C
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_NO_ACK | U8G_I2C_OPT_FAST);

// Button debouncing helpers
uint8_t but1s = 1, but2s = 1, but3s = 1, but4s = 1;
uint8_t but1p = 1, but2p = 1, but3p = 1, but4p = 1;

// Setup PCF and OLED
PCF8575 PCF(PCF8575_ADDR);
SSD1306AsciiWire oled;

// Display state
typedef enum {
  UI_SPEED,
  UI_LAP_TIMER,
  UI_TEMP,
  UI_BROKE,
} UI_MODE;

UI_MODE mode = UI_SPEED;

// Debounce logic
uint8_t isFalling(uint8_t prev, uint8_t current) {
  return (prev == 1 && current == 0) ? 1 : 0;
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  // Start devices
  PCF.begin();
  oled.begin(&Adafruit128x64, OLED_ADDR);
  oled.setFont(Adafruit5x7);
  oled.clear();
  oled.println("Init complete!");

  delay(1000);
  oled.clear();
}

void loop() {
  // Save previous states
  but1p = but1s;
  but2p = but2s;
  but3p = but3s;
  but4p = but4s;

  // Read new states from PCF8575 pins
  but1s = PCF.readButton(1);  // Adjust pin numbers if different
  but2s = PCF.readButton(2);
  but3s = PCF.readButton(3);
  but4s = PCF.readButton(4);

  // Detect button press (falling edge)
  bool but1 = isFalling(but1p, but1s);
  bool but2 = isFalling(but2p, but2s);
  bool but3 = isFalling(but3p, but3s);
  bool but4 = isFalling(but4p, but4s);

  // Cycle display modes using buttons
  if (but1) {
    mode = UI_SPEED;
  } else if (but2) {
    mode = UI_LAP_TIMER;
  } else if (but3) {
    mode = UI_TEMP;
  } else if (but4) {
    mode = UI_BROKE;
  }

  // Display UI
  oled.clear();
  oled.set2X();

  switch (mode) {
    case UI_SPEED:
      oled.println("Speed Mode");
      break;
    case UI_LAP_TIMER:
      oled.println("Lap Timer");
      break;
    case UI_TEMP:
      oled.println("Temp Mon");
      break;
    case UI_BROKE:
      oled.println("Broken!");
      break;
  }

  oled.set1X();
  oled.setCursor(0, 6);
  oled.print("B1:");
  oled.print(but1s == 0 ? "Pressed" : "Up");
  oled.print(" B2:");
  oled.print(but2s == 0 ? "Pressed" : "Up");
  oled.print(" B3:");
  oled.print(but3s == 0 ? "Pressed" : "Up");
  oled.print(" B4:");
  oled.print(but4s == 0 ? "Pressed" : "Up");

  delay(10);  // debounce buffer
}