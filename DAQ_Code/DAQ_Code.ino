//Definitions/////////////////////////////////////////////////
/////Accelerometer
/*
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>*/
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//------------------------------------------------------------
/////Speed-temp
#include <SoftwareSerial.h>
#include <TinyGPS.h>

//------------------------------------------------------------
/////Temp
#include <OneWire.h>
#include <DallasTemperature.h>

// temp screen
#include <LiquidCrystal_I2C.h>
/*
//-----------------------------------------------------------
//SD CARD Stuff
#include <SD.h>
#include <SPI.h>

File myFile;
const int chipSelect = BUILTIN_SDCARD;
bool isSD = true;
*/

//Initialize//////////////////////////////////////////////////
/////Accelerometer
/*
Adafruit_MPU6050 mpu;
int current_accel;
byte max_accel = 0;
int init_x,init_y,init_z;
int cur_x,cur_y,cur_z;
unsigned int numIter = 0;*/

//------------------------------------------------------------
/////Speed-temp
TinyGPS gps;
SoftwareSerial ss(0, 1);
float mph_speed;

bool hasTime = false;
byte hr, m, sec;
  

//------------------------------------------------------------
/////Temp
const int SENSOR_PIN = 14; // Arduino pin connected to DS18B20 sensor's DQ pin



OneWire oneWire(SENSOR_PIN);         // setup a oneWire instance
DallasTemperature tempSensor(&oneWire); // pass oneWire to DallasTemperature library

float tempCelsius;    // temperature in Celsius
float tempFahrenheit; // temperature in Fahrenheit

bool prev=false;

float max_speed = 0;




LiquidCrystal_I2C lcd(0x27,  20, 4);




//Setup///////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  /////Accelerometer
/*
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  //Serial.println("MPU6050 Found!");
   


  // set accelerometer range to +-8G
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

  // set gyro range to +- 500 deg/s
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  // set filter bandwidth to 21 Hz
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  delay(100);
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  init_x = a.acceleration.x;
  init_y = a.acceleration.y;
  init_z = a.acceleration.z;
*/
  //------------------------------------------------------------
  /////Speed-temp
  
  ss.begin(9600);
    // Try to initialize!

  
  

  //------------------------------------------------------------
  /////Temp
  tempSensor.begin();    // initialize the sensor



  lcd.init();                      
  lcd.backlight();

  //if (!SD.begin(chipSelect)) {
  //  isSD = false;
  //}

}








//Loop////////////////////////////////////////////////////////
void loop() {
  /////Accelerometer
  /* Get new sensor events with the readings 
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Print out the values 
  cur_x = a.acceleration.x - init_x;
  cur_y = a.acceleration.y - init_y;
  cur_z = a.acceleration.z - init_z;
  current_accel = sqrt(pow(cur_x,2) + pow(cur_y,2) +pow(cur_z,2));
  Serial.print(cur_x);
  Serial.print(",");
  Serial.print(cur_y);
  Serial.print(",");
  Serial.print(cur_z);
  Serial.print(",");
  Serial.print(current_accel);
  Serial.print("\n");
  if(current_accel > max_accel){
    max_accel = current_accel;
  }*/


  //------------------------------------------------------------
  /////Speed-temp
  bool newData = false;
//  unsigned long chars;
 // unsigned short sentences, failed;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }
  if( (prev==false) && (newData==true)){
    lcd.clear();
  }
  if (newData)
  {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    mph_speed = gps.f_speed_mph();
    Serial.print(mph_speed);
    if(mph_speed > max_speed){
      max_speed = mph_speed;
    }
    lcd.setCursor(0,0);
    lcd.print(mph_speed,2);
    lcd.setCursor(5,0);
    lcd.print("mph");
    lcd.setCursor(11,0);
    lcd.print(max_speed);
    lcd.setCursor(16,0);
    lcd.print("max");

      int year;
  byte month, day, hour, minute, second, hundredths;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    if(!hasTime){
      hr = hour;
      m = minute;
      sec = second;
      hasTime = true;
    }
  if (age == TinyGPS::GPS_INVALID_AGE){
  }
  else
  {
    if(hour >= hr){
    hour-=hr;
    }
    else{
      hour += (24-hr);
    }
    if(minute>=m){
    minute-=m;
    }
    else{
      minute +=(60-m);
    }
    if(second >=sec){
      second-=sec;
    }
    else{
      second+=(60-sec);
    }
    char sz[12];
    sprintf(sz, "%02d:%02d:%02d",
        hour, minute, second);
    lcd.setCursor(0,2);
    lcd.print(sz);
    if(minute >=45 && ((hour == 0) || (hour == 3)) && minute <=52){
      lcd.setCursor(0,3);
      lcd.print("Fuel low!");
    }
    else if(((hour == 1) || (hour ==4)) && minute >=30 && minute <=37){
      lcd.setCursor(0,3);
      lcd.print("Fuel low!");
    }
    else if(((hour ==2) || (hour ==5)) && minute <=22 && minute >=15){
      lcd.setCursor(0,3);
      lcd.print("Fuel low!");
    }
    else if(hour == 3 && minute <=7){
      lcd.setCursor(0,3);
      lcd.print("Fuel low!");
    }
  }/*
     if(isSD){
        myFile = SD.open("bajaDAQ.txt", FILE_WRITE);
        myFile.print("Time:")
        myFile.print(sz);
        myFile.print 
	  // close the file:
    myFile.close();
     }*/
  }
  else{
    lcd.setCursor(0,0);
    lcd.print("invalid speed      ");
        lcd.setCursor(0, 2);
    lcd.print("Invalid time");
  }
  prev = newData;

  //------------------------------------------------------------
  /////Temp
  tempSensor.requestTemperatures();             // send the command to get temperatures
  tempCelsius = tempSensor.getTempCByIndex(0);  // read temperature in Celsius
  tempFahrenheit = tempCelsius * 9 / 5 + 32; // convert Celsius to Fahrenheit


  lcd.setCursor(0,1);
  lcd.print("temp:");
  lcd.setCursor(6,1);
  lcd.print(tempFahrenheit,2);


  //date+time
  


 


  delay(100);
  
  

}






//Function////////////////////////////////////////////////////


//------------------------------------------------------------
/////Speed-temp
  
//------------------------------------------------------------
/////Temp
