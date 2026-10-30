// Bare minimum Heltec V3 diagnostic
// Tests: Serial output, OLED display, LoRa radio — independently
// If display doesn't work, check board selection under Tools > Board

#include "SSD1306Wire.h"

// V3 OLED pins
#define SDA_OLED 17
#define SCL_OLED 18
#define RST_OLED 21

SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64);

void setup() {
  Serial.begin(115200);
  delay(2000); // Give serial monitor time to connect

  Serial.println("=== HELTEC V3 DIAGNOSTIC ===");
  Serial.println("Step 1: Serial is working!");

  // Enable Vext power (GPIO 36) — powers the OLED on V3 boards
  Serial.println("Step 1.5: Enabling Vext power...");
  pinMode(36, OUTPUT);
  digitalWrite(36, LOW);
  delay(50);

  // Reset the OLED display
  Serial.println("Step 2: Resetting OLED...");
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
  digitalWrite(RST_OLED, LOW);
  delay(20);
  digitalWrite(RST_OLED, HIGH);
  delay(50);

  // Initialize display
  Serial.println("Step 3: Initializing display...");
  display.init();
  display.setContrast(255);
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);

  // Draw to display
  display.clear();
  display.drawString(0, 0, "HELLO BAJA!");
  display.drawString(0, 20, "Display OK");
  display.drawString(0, 40, "V3 Diag");
  display.display();

  Serial.println("Step 4: Display should now show text!");
  Serial.println("If you see this in Serial but NOT on the screen,");
  Serial.println("the display wiring or board selection is wrong.");
  Serial.println("============================");
}

void loop() {
  Serial.println("Still alive...");
  delay(2000);
}
