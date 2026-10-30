//Author Daniel VanWiggeren / Taylor Van Horn
//Last Edit 05/04/26

//Moved from old radio libraries to new heltec libraries

//TX refers to the transmission of data over radio frequencies
//RX refers to the receiving of data over radio frequencies

#include <Wire.h>
#include <heltec_unofficial.h>

#define I2C_DEV_ADDR 0x69
#define FREQUENCY 915.0
#define BANDWIDTH 125.0
#define SPREAD_FACTOR 7
#define CODING_RATE 5
#define PREAMBLE_LEN 8
#define TX_POWER 16
#define BUFFER_SIZE 150

char txpacket[BUFFER_SIZE];
char pendingPacket[BUFFER_SIZE];

volatile bool newData = false;
volatile bool txDone = true;

void setTxDoneFlag() { txDone = true; }

void onReceive(int len) {
  int num = 0;
  while (Wire.available() && num < BUFFER_SIZE - 1) {
    pendingPacket[num++] = Wire.read();
  }
  pendingPacket[num] = '\0';
  newData = true;
}

void setup() {
  pinMode(36, OUTPUT);
  digitalWrite(36, LOW);
  delay(50);

  heltec_setup();

  display.init();
  display.setContrast(255);
  display.flipScreenVertically();

  Wire.begin((uint8_t)I2C_DEV_ADDR);
  Wire.onReceive(onReceive);

  display.clear();
  display.drawString(0, 0, "Radio Init...");
  display.display();

  RADIOLIB_OR_HALT(radio.begin());
  radio.setFrequency(FREQUENCY);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREAD_FACTOR);
  radio.setCodingRate(CODING_RATE);
  radio.setPreambleLength(PREAMBLE_LEN);
  radio.setOutputPower(TX_POWER);
  radio.setPacketSentAction(setTxDoneFlag);

  display.clear();
  display.drawString(0, 0, "DAQ Slave Ready");
  display.drawString(0, 12, "Waiting for I2C...");
  display.display();
  Serial.println("DAQ LoRa Slave ready. Waiting for Teensy Data...");
}

void loop() {
  heltec_loop();

  if (newData && txDone) {
    noInterrupts();
    memcpy(txpacket, pendingPacket, BUFFER_SIZE);
    newData = false;
    interrupts();

    txDone = false;
    int state = radio.startTransmit(txpacket);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.printf("TX: %s\n", txpacket);
      display.clear();
      display.drawString(0, 0, "Sent");
      display.drawString(0, 16, txpacket);
      display.display();
    } else {
      Serial.printf("TX failed: %d\n", state);
      txDone = true;
    }
  }
}
