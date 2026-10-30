#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial); // Wait for Serial monitor
  Serial.println("I2C Scanner Starting...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0)
    Serial.println("No I2C devices found.\n");

  delay(2000);
}
