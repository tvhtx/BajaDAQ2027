#include <heltec_unofficial.h>

#define RF_FREQUENCY 915.0
#define LORA_BANDWIDTH 125.0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 5
#define LORA_PREAMBLE_LENGTH 8
#define BUFFER_SIZE 150

volatile bool rxFlag = false;
void setRxFlag() { rxFlag = true; }

void setup() {
  pinMode(36, OUTPUT);
  digitalWrite(36, LOW);
  delay(50);

  heltec_setup();

  display.init();
  display.setContrast(255);
  display.flipScreenVertically();

  display.clear();
  display.drawString(0, 0, "Radio Init...");
  display.display();

  RADIOLIB_OR_HALT(radio.begin());
  radio.setFrequency(RF_FREQUENCY);
  radio.setBandwidth(LORA_BANDWIDTH);
  radio.setSpreadingFactor(LORA_SPREADING_FACTOR);
  radio.setCodingRate(LORA_CODINGRATE);
  radio.setPreambleLength(LORA_PREAMBLE_LENGTH);
  radio.setPacketReceivedAction(setRxFlag);

  RADIOLIB_OR_HALT(radio.startReceive());

  display.clear();
  display.drawString(0, 0, "Waiting for");
  display.drawString(0, 12, "packets...");
  display.drawString(15, 48, "BAYLOR BAJA 2026");
  display.display();
  Serial.println("Entering RX mode... Waiting for telemetry from DAQ_LoRa_slave...");
}

void loop() {
  heltec_loop();

  if (!rxFlag) return;
  rxFlag = false;

  String rxdata;
  int state = radio.readData(rxdata);
  RADIOLIB_OR_HALT(radio.startReceive());

  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("RX failed: %d\n", state);
    return;
  }

  float rssi = radio.getRSSI();
  float snr = radio.getSNR();
  Serial.println("================================");
  Serial.println("Received: " + rxdata);
  Serial.printf("RSSI: %.0f | SNR: %.0f\n", rssi, snr);
  Serial.println("================================\n");

  // Parse CSV: time,millis,mph,gps,cvtRPM,wheelRPM,cvtT,diffT,gearT
  char buf[BUFFER_SIZE];
  rxdata.toCharArray(buf, sizeof(buf));

  char* timeStr  = strtok(buf,  ",");
  char* millisStr = strtok(NULL, ","); (void)millisStr;
  char* mphStr   = strtok(NULL, ",");
  char* gpsStr   = strtok(NULL, ","); (void)gpsStr;
  char* cvtStr   = strtok(NULL, ",");
  char* wheelStr = strtok(NULL, ",");
  char* tempStr  = strtok(NULL, ",");

  display.clear();
  char line[40];
  snprintf(line, sizeof(line), "%s mph", mphStr ? mphStr : "?");
  display.drawString(0, 0, line);
  snprintf(line, sizeof(line), "CVT:%s Whl:%s", cvtStr ? cvtStr : "?", wheelStr ? wheelStr : "?");
  display.drawString(0, 16, line);
  snprintf(line, sizeof(line), "Temp: %s F", tempStr ? tempStr : "?");
  display.drawString(0, 32, line);
  snprintf(line, sizeof(line), "RSSI:%.0f %s", rssi, timeStr ? timeStr : "?");
  display.drawString(0, 48, line);
  display.display();
}
