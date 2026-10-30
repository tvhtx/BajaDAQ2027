#include <Wire.h>
#include "BluetoothSerial.h"

#define ACCEL_ADDR 0x18  // given SA0 is grounded
#define OUT_X_L_REG 0x28
#define OUT_X_H_REG 0x29
#define OUT_Y_L_REG 0x2A
#define OUT_Y_H_REG 0x2B

#define CTRL_REG1 0x20
#define CTRL_REG4 0x23
#define CTRL_REG2 0x21
#define HP_FILTER_RESET_REG 0x25

#define CTRL_REG1_WRITE 0b00100011
//#define CTRL_REG2_WRITE 0b00010000
#define FS 0b11 // Controls g sensitivity (See section 8.5 in datasheet)
#define CTRL_REG4_WRITE FS << 4

#define SENSITIVITY_400G 0.195
//#define SENSITIVITY_100G 0.049
#define ZEROG_400G -21 // forgor what this does
#define OFFSET_400G 38.45

BluetoothSerial SerialBT;

void I2C_write_reg(uint32_t subaddr, uint32_t payload) {
  Wire.beginTransmission(ACCEL_ADDR);
  Wire.write(subaddr);
  Wire.write(payload);
  Wire.endTransmission();
}

uint32_t I2C_read_reg(uint32_t subaddr) {
  Wire.beginTransmission(ACCEL_ADDR);
  Wire.write(subaddr);
  Wire.endTransmission();

  Wire.requestFrom(ACCEL_ADDR, 1);

  return Wire.read();
}

float accel_read_x() {
  uint8_t x_msb = I2C_read_reg(OUT_X_H_REG);
  uint8_t x_lsb = I2C_read_reg(OUT_X_L_REG);
  int16_t raw16 = (int16_t)((x_msb << 8) | x_lsb) >> 4;

  // Maybe need offset? See if this is good enough
  return (raw16 * SENSITIVITY_400G * 9.81) + OFFSET_400G;
}

float accel_read_y() {
  uint8_t y_msb = I2C_read_reg(OUT_Y_H_REG);
  uint8_t y_lsb = I2C_read_reg(OUT_Y_L_REG);
  int16_t raw16 = (int16_t)((y_msb << 8) | y_lsb) >> 4;

  // Maybe need offset? See if this is good enough
  return (raw16 * SENSITIVITY_400G * 9.81) + OFFSET_400G;
}

float read_accel_x_median(){
  float acc1 = accel_read_x();
  delay(10);
  float acc2 = accel_read_x();
  delay(10);
  float acc3 = accel_read_x();
  delay(10);

  return median_filter(acc1, acc2, acc3);
}

float read_accel_y_median(){
  float acc1 = accel_read_y();
  delay(10);
  float acc2 = accel_read_y();
  delay(10);
  float acc3 = accel_read_y();
  delay(10);

  return median_filter(acc1, acc2, acc3);
}


float median_filter(float a, float b, float c) {
    if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
    if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
    return c;
}

float wheel_angle(float accel_x, float accel_y){
  float angle = atan(accel_y/accel_x) * 180/PI;

  if(accel_x < 0){
    angle += 180;
  }
  else if(accel_y < 0){
    angle += 360;
  }

  return angle;
}

void setup() {

  Serial.begin(115200);
  delay(1000);
  SerialBT.begin("ESP32-Dyno");
  Serial.println("Bluetooth started. Pair with 'ESP32-Dyno'.");

  while(!SerialBT.hasClient()) {
    delay(1000);
    Serial.println("Waiting to pair...");
  }

  Serial.println("Device paired!");

  Wire.begin();

  I2C_write_reg(CTRL_REG1, CTRL_REG1_WRITE);  // Writes to CTRL_REG1 to power on device and enable x and y axis
  // See section 8.2 on the H3LIS331DL datasheet for more info
  I2C_write_reg(CTRL_REG4, CTRL_REG4_WRITE);  // Writes to CTRL_REG4 to make sensitivity up to 400gs
  // See section 8.5 on the H3LIS331DL datasheet for more info
  //I2C_write_reg(CTRL_REG2, CTRL_REG2_WRITE);  // Enables built in high pass filter

  //I2C_read_reg(HP_FILTER_RESET_REG);
  delay(1000);
}

// TODO: SAMPLE FASTER!! WRITE TO REGISTER FOR DATA RATE

float x_accel;
float y_accel;
float angle;

void loop() {

  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }


  //x_accel = accel_read_x();
  //y_accel = accel_read_y();

  //Serial.println(x_accel);

  x_accel = read_accel_x_median();
  y_accel = read_accel_y_median();

  //Serial.println(x_accel);

  angle = wheel_angle(x_accel, y_accel);


  SerialBT.println(angle);
  
  delay(25);
}