#include <Wire.h>

#define PICO_I2C_ADDR 0x2E
#define LED_PIN       7
#define BUTTON_PIN    8

enum STATUS {
  STATUS_INIT,
  STATUS_WAIT,
  STATUS_START,
  STATUS_COUNT,
  STATUS_STOP,
  STATUS_END
};

enum STATUS timer_stat = STATUS_INIT;

unsigned long elapsed_sec = 0;
unsigned long prev_millis = millis();

int button_prev = LOW;
int button_pressed = LOW;

void setup() {
  Wire.begin();

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  digitalWrite(LED_PIN, HIGH);
  delay(250);
  digitalWrite(LED_PIN, LOW);
  delay(250);
}

void loop() {
  isReady();

  check_button_stat();
  
  switch (timer_stat) {
    case STATUS_INIT:
      timer_stat = STATUS_WAIT;
      
      digitalWrite(LED_PIN, HIGH);
      
      init_message();
      break;
    case STATUS_WAIT:
      if (get_button_pressed() == LOW) {
        break;
      }
      
      wait_message();
      
      timer_stat = STATUS_START;
      prev_millis = millis();
      break;
    case STATUS_START:
      if (get_elapsed_time() < 5000) {
        break;
      }
      
      timer_stat = STATUS_COUNT;
      elapsed_sec = 0;
      prev_millis = millis();
      
      start_message();
      break;
    case STATUS_COUNT:
      if (get_button_pressed() == HIGH) {
        timer_stat = STATUS_STOP;
        break;
      }
      
      digitalWrite(LED_PIN, (get_elapsed_time() / 1000) % 2);
      
      if (get_elapsed_time() > 5000) {
        elapsed_sec += 5;
        prev_millis = millis();
  
        count_message();
      }
      break;
    case STATUS_STOP:
      timer_stat = STATUS_END;

      elapsed_sec += get_elapsed_time() / 1000;
      prev_millis = millis();
  
      stop_message();
    case STATUS_END:
    default:
      if (get_elapsed_time() > 3000) {
        timer_stat = STATUS_INIT;
      }
      break;
  }
}

void isReady() {
  while(1) {
    Wire.requestFrom(PICO_I2C_ADDR, 1);

    while(Wire.available() > 0){
      if(Wire.read() == '>') {
        return;
      }

      delay(100);
    }
  }
}

void check_button_stat() {
  int s = digitalRead(BUTTON_PIN);
  if (button_prev == HIGH && s == LOW) {
    button_pressed = HIGH;
  }
  button_prev = s;
}

int get_button_pressed() {
  int s = button_pressed;
  button_pressed = LOW;
  return s;
}

int get_elapsed_time() {
  return millis() - prev_millis;
}

void init_message() {
  Wire.beginTransmission(PICO_I2C_ADDR);
  Wire.write("sui'cchio/o_suto\r");
  Wire.endTransmission();
  
  isReady();
  
  Wire.beginTransmission(PICO_I2C_ADDR);
  Wire.write("ta'ima-o/kido-shima'_su.\r");
  Wire.endTransmission();
}

void wait_message() {
  Wire.beginTransmission(PICO_I2C_ADDR);
  Wire.write("<NUMK VAL=5 COUNTER=byo->goni/\r");
  Wire.endTransmission();
  
  isReady();
  
  Wire.beginTransmission(PICO_I2C_ADDR);
  Wire.write("ta'ima-o/_suta'-to+shima'_su.\r");
  Wire.endTransmission();
}

void start_message() {
  Wire.beginTransmission(PICO_I2C_ADDR);
  Wire.write("_suta'-to\r");
  Wire.endTransmission();
}

void count_message() {
  String str = "<NUMK VAL=";
  str += String(elapsed_sec, DEC);
  str += " COUNTER=byo->.\r";
  
  char msg[256];
  str.toCharArray(msg, 256);
  
  Wire.beginTransmission(PICO_I2C_ADDR);
  Wire.write(msg);
  Wire.endTransmission();
}

void stop_message() {
  String str = "<NUMK VAL=";
  str += String(elapsed_sec, DEC);
  str += " COUNTER=byo->\r";
  
  char msg[256];
  str.toCharArray(msg, 256);
  
  Wire.beginTransmission(PICO_I2C_ADDR);
  Wire.write(msg);
  Wire.endTransmission();
  
  isReady();
  
  Wire.beginTransmission(PICO_I2C_ADDR);
  Wire.write("de/shita.\r");
  Wire.endTransmission();
}
