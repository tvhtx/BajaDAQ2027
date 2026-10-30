int hall_voltage;
const double DEGREES_PER_PULSE = 1;
const double HALL_EFFECT_PIN = 3;
int pulse_timestamp = millis();
double curr_rpm;

/*
Black wire -> GND
Red wire -> 5V
Black and white wire -> D3
*/

void hall_effect_handler(){
  int period = millis() - pulse_timestamp;
  double deg_per_s = DEGREES_PER_PULSE / period;

  curr_rpm = deg_per_s * (60/360);
  Serial.println("Pulse");
}

void setup() {
  Serial.begin(115200); // Set the Serial Monitor to 115200 baud rate

  attachInterrupt(digitalPinToInterrupt(HALL_EFFECT_PIN), hall_effect_handler, RISING);
}

void loop() {
  hall_voltage = digitalRead(HALL_EFFECT_PIN); // Plug in the black striped white wire into D3
  Serial.println(hall_voltage);
  //Serial.println(curr_rpm);
}
