// LoRa RX Test Script for Heltec WiFi LoRa 32 V3
// Listens for packets from LoRa_TX_Test and prints them
// Flash this to one Heltec board, and flash LoRa_TX_Test to the other
//
// Board: WiFi LoRa 32 (V3) under Heltec ESP32 Series Dev-boards

#include <heltec_unofficial.h>

// LoRa parameters — MUST match the TX script exactly
#define FREQUENCY 915.0 // MHz (US ISM band)
#define BANDWIDTH 125.0 // kHz
#define SPREAD_FACTOR 7 // SF7
#define CODING_RATE 5   // 4/5
#define PREAMBLE_LEN 8  // Symbols

unsigned long rxCount = 0;

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
  display.drawString(0, 0, "LoRa RX Test");
  display.drawString(0, 12, "Initializing...");
  display.display();

  // Initialize radio
  RADIOLIB_OR_HALT(radio.begin());

  // Configure radio parameters — must match TX side
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREAD_FACTOR);
  radio.setCodingRate(CODING_RATE);
  radio.setPreambleLength(PREAMBLE_LEN);

  display.clear();
  display.drawString(0, 0, "RX Ready!");
  display.drawString(0, 12, "Listening...");
  display.display();
  Serial.println("LoRa RX Test - Listening for packets...");
  Serial.println("Format: "
                 "time,millis,wheelMPH,gpsSpeed,cvtRPM,wheelRPM,cvtTemp,"
                 "diffTemp,gearTemp");
  Serial.println("============================================================="
                 "============");
}

void loop() {
  heltec_loop();

  String rxdata;
  int state = radio.receive(rxdata);

  if (state == RADIOLIB_ERR_NONE) {
    // Packet received successfully
    rxCount++;
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();

    Serial.println("==========================================================="
                   "==============");
    Serial.printf("[RX #%lu] %s\n", rxCount, rxdata.c_str());
    Serial.printf("  RSSI: %.1f dBm | SNR: %.1f dB | Length: %d bytes\n", rssi,
                  snr, rxdata.length());
    Serial.println("==========================================================="
                   "==============\n");

    // Show on OLED
    char line1[30], line2[30], line3[30];
    sprintf(line1, "RX #%lu", rxCount);
    sprintf(line2, "RSSI:%.0f SNR:%.0f", rssi, snr);

    // Parse speed from CSV: time,millis,wheelMPH,...
    int commaCount = 0;
    int startIdx = 0;
    String mphStr = "---";
    for (int i = 0; i < (int)rxdata.length(); i++) {
      if (rxdata[i] == ',') {
        commaCount++;
        if (commaCount == 2)
          startIdx = i + 1;
        if (commaCount == 3) {
          mphStr = rxdata.substring(startIdx, i);
          break;
        }
      }
    }
    sprintf(line3, "Speed: %s mph", mphStr.c_str());

    display.clear();
    display.drawString(0, 0, line1);
    display.drawString(0, 16, line2);
    display.drawString(0, 32, line3);
    display.display();

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Normal timeout, just keep listening
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("[RX] CRC error - bad packet");
    display.clear();
    display.drawString(0, 0, "CRC Error!");
    display.display();
  } else {
    Serial.printf("[RX] Failed, code %d\n", state);
  }
}
