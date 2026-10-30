#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/TomThumb.h>
//#include <TimerOne.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define MAX_RPM 3800

#define TACHOMETER_HEIGHT 5

#define LAP_TIMER_X 30
#define LAP_TIMER_Y 25

#define SPEEDOMETER_X 43
#define SPEEDOMETER_Y 14

GFXfont* default_font = &TomThumb;
uint8_t default_text_size = 1;

typedef enum{
  MAIN_MENU
}display_state;

uint64_t lap_timer_begin_timestamp;
volatile uint64_t lap_timer_time_elapsed;
volatile uint32_t minutes_elapsed;
volatile uint32_t seconds_elapsed;
volatile uint32_t ms_elapsed;


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
display_state curr_display_state;


int16_t rpm_x_ticks_arr[MAX_RPM/1000+1];

void calc_rpm_x_ticks(){
  int num_tach_tick_marks = MAX_RPM / 1000;
  int cursor_position = 0;

  for(int i = 0; i < num_tach_tick_marks+1; i++){
    rpm_x_ticks_arr[i] = cursor_position;
    cursor_position += SCREEN_WIDTH/num_tach_tick_marks-1;
  }
}

void setup_tachometer(){
  display.setTextSize(1);
  display.setFont(default_font);

  display.drawRect(0, 0, SCREEN_WIDTH, TACHOMETER_HEIGHT, WHITE);
  
  for(int i = 0; i <  MAX_RPM/1000+1; i++){
    // draw rpm number from tick mark array
    display.setCursor(rpm_x_ticks_arr[i], TACHOMETER_HEIGHT+7);
    display.print(i);
    if(i != 0 && i != MAX_RPM/1000){
      // only put a little tick mark next to rpm number if its not the lowest or highest rpm
      display.drawPixel(rpm_x_ticks_arr[i]+1, TACHOMETER_HEIGHT, WHITE);
    }
  }
}

double rpm_rect_width;
void update_tachometer(uint32_t rpm){
  display.fillRect(1, 1, SCREEN_WIDTH-2, TACHOMETER_HEIGHT-2, BLACK);
  rpm_rect_width = ((double)rpm/MAX_RPM)*SCREEN_WIDTH-2;
  display.fillRect(1, 1, (int16_t)rpm_rect_width, TACHOMETER_HEIGHT-2, WHITE);
}

void setup_speedometer(int16_t x=SPEEDOMETER_X, int16_t y=SPEEDOMETER_Y){
  display.drawRoundRect(x, y, 40, 50, 1, WHITE);
  display.setCursor(x+15, y+45);
  display.setFont(&TomThumb);
  display.setTextSize(1);
  display.print("mph");
  
  display.setFont(default_font);
  display.setTextSize(default_text_size);
}

void update_speed(uint32_t speed, int16_t x=SPEEDOMETER_X, int16_t y=SPEEDOMETER_Y){ // What about reverse? Negative or not? Watch out for the uint32_5 if you want negatives
  display.fillRect(x+5, y+6, 22, 16, BLACK);
  display.setTextSize(3);
  if(speed < 10){
    x += 10;
  }
  else{
    x += 5;
  }
  display.setCursor(x, 55);
  display.print(speed);

  display.setFont(default_font);
  display.setTextSize(default_text_size);
}

void timer_counter_handler(){
  lap_timer_time_elapsed = millis() - lap_timer_begin_timestamp;
  minutes_elapsed = lap_timer_time_elapsed / (60000);
  seconds_elapsed = ((lap_timer_time_elapsed - minutes_elapsed*60000) / 1000) % 60;
  ms_elapsed = lap_timer_time_elapsed % 1000;

  char time_str[10];
  sprintf(time_str, "%02lu:%02lu.%03lu", minutes_elapsed, seconds_elapsed, ms_elapsed);
  display.setCursor(LAP_TIMER_X, LAP_TIMER_Y);
  display.setTextSize(1);
  display.setFont(NULL);
  display.fillRect(LAP_TIMER_X, LAP_TIMER_Y-7, 54, 9, BLACK);
  display.print(time_str);
  display.display();

  display.setFont(default_font);
  display.setTextSize(default_text_size);
}

void setup_engine_temp(){
  display.drawRoundRect(64, 32, 20, 20, 1, WHITE);
}

void display_engine_temp(uint32_t engine_temp){
  
}

struct element_struct{
  uint32_t border_x;
  uint32_t border_y;
  uint32_t border_w;
  uint32_t border_h;
  GFXfont* font;

  uint32_t content_x;
  uint32_t content_y;
  uint32_t content_w;
  uint32_t content_h;

  uint32_t content_clear_x;
  uint32_t content_clear_y;
  uint32_t content_clear_w;
  uint32_t content_clear_h;

}

void setup() {
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  //Timer1.initialize(1000);
  //Timer1.attachInterrupt(timer_counter_handler);

  calc_rpm_x_ticks();
  //delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);

  curr_display_state = MAIN_MENU;

  //setup_tachometer();
  //setup_speedometer();

  display.drawBitmap(64, 32, )

  display.display();

  lap_timer_begin_timestamp = millis();
  lap_timer_time_elapsed = 0;

}

void loop() {
  
}
