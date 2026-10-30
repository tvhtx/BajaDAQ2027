#include <TinyGPS.h>

/*
Using teensy 4.1
GPS TX -> pin 0
GPS RX -> pin 1
VCC -> 5V
GND -> GND
*/

TinyGPS gps;

unsigned long lastPrint = 0;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
}

  while (Serial1.available()) {
    gps.encode(Serial1.read());
  }


  long lat, lon;
  unsigned long age;

  gps.get_position(&lat, &lon, &age);

  if (age != TinyGPS::GPS_INVALID_AGE) {
    Serial.print("Lat: ");
    Serial.print(lat / 1000000.0, 6);
    Serial.print("  Lon: ");
    Serial.println(lon / 1000000.0, 6);
  } else {
    Serial.println("Waiting for GPS fix...");
  }
}
