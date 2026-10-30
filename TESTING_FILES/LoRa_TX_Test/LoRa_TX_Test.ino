// LoRa TX Test Script for Heltec WiFi LoRa 32 V3
// Sends fake telemetry packets every 2 seconds
// Flash this to one Heltec board, and flash LoRa_RX_Test to the other
//
// Board: WiFi LoRa 32 (V3) under Heltec ESP32 Series Dev-boards

#include <heltec_unofficial.h>

// LoRa parameters — MUST match the RX script exactly
#define FREQUENCY 915.0 // MHz (US ISM band)
#define BANDWIDTH 125.0 // kHz
#define SPREAD_FACTOR 7 // SF7
#define CODING_RATE 5   // 4/5
#define PREAMBLE_LEN 8  // Symbols
#define TX_POWER 16     // dBm

unsigned long packetCount = 0;

void setup() {
  // Enable Vext power FIRST — display needs power before init
  pinMode(36, OUTPUT);
  digitalWrite(36, LOW);
  delay(50);

  heltec_setup();

  // Re-init display now that power is stable
  display.init();
  display.setContrast(255);
  display.flipScreenVertically();

  display.clear();
  display.drawString(0, 0, "LoRa TX Test");
  display.drawString(0, 12, "Initializing...");
  display.display();

  // Initialize radio
  RADIOLIB_OR_HALT(radio.begin());

  // Configure radio parameters
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREAD_FACTOR);
  radio.setCodingRate(CODING_RATE);
  radio.setPreambleLength(PREAMBLE_LEN);
  radio.setOutputPower(TX_POWER);

  display.clear();
  display.drawString(0, 0, "TX Ready!");
  display.display();
  Serial.println("LoRa TX Test - Sending a packet every 2 seconds...");
}

void loop() {
  heltec_loop();

  packetCount++;

  // Build a fake telemetry string that mimics real DAQ data
  // Format:
  // time,millis,wheelMPH,gpsSpeed,cvtRPM,wheelRPM,cvtTemp,diffTemp,gearTemp
  char packet[150];
  float fakeSpeed = 15.0 + random(-30, 30) / 10.0;
  float fakeGPS = fakeSpeed + random(-5, 5) / 10.0;
  long fakeCVT = 2800 + random(-200, 200);
  long fakeWheel = 1400 + random(-100, 100);
  float fakeT1 = 165.0 + random(-5, 5);
  float fakeT2 = 140.0 + random(-3, 3);
  float fakeT3 = 155.0 + random(-4, 4);

  sprintf(packet, "12:30:%02lu,%lu,%.1f,%.1f,%ld,%ld,%.1f,%.1f,%.1f",
          packetCount % 60, millis(), fakeSpeed, fakeGPS, fakeCVT, fakeWheel,
          fakeT1, fakeT2, fakeT3);

  // Transmit
  Serial.printf("[TX #%lu] %s\n", packetCount, packet);
  int state = radio.transmit(packet);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("  -> Sent OK");
    char line1[30], line2[30], line3[30];
    sprintf(line1, "TX #%lu OK", packetCount);
    sprintf(line2, "%.1f mph", fakeSpeed);
    sprintf(line3, "CVT: %ld RPM", fakeCVT);
    display.clear();
    display.drawString(0, 0, line1);
    display.drawString(0, 16, line2);
    display.drawString(0, 32, line3);
    display.display();
  } else {
    Serial.printf("  -> TX FAILED (code %d)\n", state);
    char errLine[30];
    sprintf(errLine, "TX FAIL: %d", state);
    display.clear();
    display.drawString(0, 0, errLine);
    display.display();
  }

  delay(2000); // Send every 2 seconds
}
