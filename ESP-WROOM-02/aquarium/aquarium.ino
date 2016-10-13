#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <OneWire.h>

extern "C" {
  #include "sntp.h"
}

//#define DEBUG_ADC_REG
//#define DEBUG_ADC_LIGHT
//#define DEBUG_ONEWIRE_TEMP

#define ESP_ADC_CC         15
#define ESP_PWM_RED        5
#define ESP_PWM_GREEN      4
#define ESP_PWM_BLUE       16
#define ESP_ONEWIRE        2

#define ADC_VREF           3.3
#define ADC_RESOLUTION     1024

#define ADC_RED            0
#define ADC_GREEN          1
#define ADC_BLUE           2

#define ADC_LIGHT_IN       3
#define ADC_LIGHT_OUT      4
#define ADC_LIGHT_REG      10000            // Registor 10k
#define ADC_LIGHT_PC       (0.000033 / 100) // Photocurrent 33uA:100lux

#define ONEWIRE_TEMP_AIR   { 0x28, 0xEE, 0x75, 0x4F, 0x16, 0x16, 0x01, 0x62 }
#define ONEWIRE_TEMP_WATER { 0x28, 0x45, 0x8E, 0x26, 0x00, 0x00, 0x80, 0x1C }

#include "private.h"
#ifndef ESP_WIFI_SSID
#define ESP_WIFI_SSID      "your access point name"
#endif
#ifndef ESP_WIFI_PASSWD
#define ESP_WIFI_PASSWD    "password for access point"
#endif
#ifndef ESP_NTP_SERVER
#define ESP_NTP_SERVER     "ntp.nict.jp"
#endif

ESP8266WebServer http_server(80);
OneWire onewire(ESP_ONEWIRE);

struct scheduled_handler {
  bool enable;                                       // enable flag
  unsigned long previous;                            // previously executed time in millis
  unsigned long interval;                            // executing interval
  String name;                                       // handler name
  void* (*function)(struct scheduled_handler* self); // handler function, returned value will be stored to result param
  void* result;                                      // result value of this handler
};

void* read_adc_red(struct scheduled_handler* self);
void* read_adc_green(struct scheduled_handler* self);
void* read_adc_blue(struct scheduled_handler* self);
void* set_led_brightness(struct scheduled_handler* self);

void* read_adc_light_in(struct scheduled_handler* self);
void* read_adc_light_out(struct scheduled_handler* self);

void* read_temp_air(struct scheduled_handler* self);
void* read_temp_water(struct scheduled_handler* self);

void* get_wifi_connection(struct scheduled_handler* self);
void* handle_http_request(struct scheduled_handler* self);
void* sync_ntp_server(struct scheduled_handler* self);

void* print_values(struct scheduled_handler* self);

struct scheduled_handler scheduled_handlers[] = {
  {true, 0,  100, "adc_red",          read_adc_red,        NULL},
  {true, 0,  100, "adc_green",        read_adc_green,      NULL},
  {true, 0,  100, "adc_blue",         read_adc_blue,       NULL},
  {true, 0,  100, "led_brightness",   set_led_brightness,  NULL},
  {true, 0, 1000, "adc_light_in",     read_adc_light_in,   NULL},
  {true, 0, 1000, "adc_light_out",    read_adc_light_out,  NULL},
  {true, 0, 1000, "temp_air",         read_temp_air,       NULL},
  {true, 0, 1000, "temp_water",       read_temp_water,     NULL},
  {true, 0,    0, "wifi_connection",  get_wifi_connection, NULL},
  {true, 0, 1000, "http_request",     handle_http_request, NULL},
  {true, 0,  500, "ntp_server",       sync_ntp_server,     NULL},
  {true, 0, 3000, "print_values",     print_values,        NULL},
  {true, 0,    0, "",                 NULL,                NULL} // end of table
};

void initialize_scheduled_handlers() {
  struct scheduled_handler* h = scheduled_handlers;
  while(h->function != NULL) {
    h->previous = millis();
    h++;
  }
}

void execute_scheduled_handlers() {
  struct scheduled_handler* h = scheduled_handlers;
  while(h->function != NULL) {
    if((h->enable == true) && ((millis() - h->previous) > h->interval)) {
      h->result = h->function(h);
      h->previous = millis();
    }
    h++;
  }
}

struct scheduled_handler* find_scheduled_handler(String name) {
  struct scheduled_handler* h = scheduled_handlers;
  while(h->function != NULL) {
    if(h->name.equals(name)) {
      return h;
    }
    h++;
  }
  return NULL;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  SPI.begin();

  pinMode(ESP_ADC_CC, OUTPUT);

  initialize_scheduled_handlers();
}

void loop() {
  // put your main code here, to run repeatedly:
  execute_scheduled_handlers();
}

unsigned int readAdc(byte ch) {
  digitalWrite(ESP_ADC_CC, LOW);
  SPI.transfer(0x01);
  byte highbyte = SPI.transfer(((8+ch)<<4));
  byte lowbyte = SPI.transfer(0x00);
  digitalWrite(ESP_ADC_CC, HIGH);
  
  return ((highbyte << 8) + lowbyte) & 0x03FF;
}

void* read_adc_red(struct scheduled_handler* self) {
  static unsigned int result = 0;
  
  result = readAdc(ADC_RED); // 0 - 1023
#ifdef DEBUG_ADC_REG
  Serial.print("adc_red=");
  Serial.print(result);
  Serial.println();
#endif

  return &result;
}

void* read_adc_green(struct scheduled_handler* self) {
  static unsigned int result = 0;
  
  result = readAdc(ADC_GREEN); // 0 - 1023
#ifdef DEBUG_ADC_REG
  Serial.print("adc_green=");
  Serial.print(result);
  Serial.println();
#endif

  return &result;
}

void* read_adc_blue(struct scheduled_handler* self) {
  static unsigned int result = 0;
  
  result = readAdc(ADC_BLUE); // 0 - 1023
#ifdef DEBUG_ADC_REG
  Serial.print("adc_blue=");
  Serial.print(result);
  Serial.println();
#endif

  return &result;
}

void* set_led_brightness(struct scheduled_handler* self) {
  static unsigned int result = 0;

  if(self->result == NULL) {
    pinMode(ESP_PWM_RED, OUTPUT);
    pinMode(ESP_PWM_GREEN, OUTPUT);
    pinMode(ESP_PWM_BLUE, OUTPUT);
  }

  unsigned int* r = (unsigned int*)find_scheduled_handler("adc_red")->result;
  unsigned int* g = (unsigned int*)find_scheduled_handler("adc_green")->result;
  unsigned int* b = (unsigned int*)find_scheduled_handler("adc_blue")->result;
  if(r != NULL && g != NULL && b != NULL) {
    analogWrite(ESP_PWM_RED, *r);
    analogWrite(ESP_PWM_GREEN, *g);
    analogWrite(ESP_PWM_BLUE, *b);
  }

  return &result;
}

unsigned int calculate_lux(unsigned int adc_result) {
  double lux_per_voltage = ADC_LIGHT_REG * ADC_LIGHT_PC;
  double voltage = ADC_VREF / ADC_RESOLUTION * adc_result;
  return floor(voltage / lux_per_voltage);
}

void* read_adc_light_in(struct scheduled_handler* self) {
  static unsigned int result = 0;
  
  result = calculate_lux(readAdc(ADC_LIGHT_IN)); // adc:0 - 1024, lux:0 - 1000
#ifdef DEBUG_ADC_LIGHT
  Serial.print("adc_light_in=");
  Serial.print(result);
  Serial.println();
#endif

  return &result;
}

void* read_adc_light_out(struct scheduled_handler* self) {
  static unsigned int result = 0;

  result = calculate_lux(readAdc(ADC_LIGHT_OUT)); // adc:0 - 1024, lux:0 - 1000
#ifdef DEBUG_ADC_LIGHT
  Serial.print("adc_light_out=");
  Serial.print(result);
  Serial.println();
#endif

  return &result;
}

float read_temp(byte* target_addr) {
  // -60 is failed, -55 to 125 is valid temperature.
  float result = -60;
  bool found = false;
  byte addr[8];
  byte data[9];

  // search devices
  onewire.reset_search();
  while(onewire.search(addr)) {
    if(memcmp(addr, target_addr, 8) == 0) {
      found = true;
      break;
    }
  }

  if(found == false) {
    Serial.println("Couldn't find specified device.");
    return result;
  }

#ifdef DEBUG_ONEWIRE_TEMP
  Serial.print("OneWire device found: ");
  for(int i = 0; i < 8; i++) {
    Serial.printf("0x%02X ", addr[i]);
  }
  Serial.println();
#endif

  if(OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return result;
  }

  if(addr[0] != 0x28) {
    Serial.printf("Device family is not DS18B20(0x28): 0x%02X", addr[0]);
    return result;
  }

  onewire.reset();
  onewire.select(addr);
  //onewire.write(0x44,1);         // start conversion, with parasite power on at the end
  onewire.write(0x44);

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  byte present = onewire.reset();
  if(present != 1) {
    Serial.printf("Device is not ready: %d\n", present);
    return result;
  }

  onewire.select(addr);
  onewire.write(0xBE);         // Read Scratchpad

  for (int i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = onewire.read();
  }

#ifdef DEBUG_ONEWIRE_TEMP
  Serial.print("Returned data is: ");
  for (int i = 0; i < 9; i++) {
    Serial.printf("0x%02X ", data[i]);
  }
  Serial.println();
#endif

  if(OneWire::crc8(data, 8) != data[8]) {
    Serial.println("CRC is not valid!");
    return result;
  }

  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if(cfg == 0x00) {        // 9 bit resolution, 93.75 ms
    raw = raw & ~7;
  } else if(cfg == 0x20) { // 10 bit res, 187.5 ms
    raw = raw & ~3;
  } else if(cfg == 0x40) { // 11 bit res, 375 ms
    raw = raw & ~1;
  } else {                 // default is 12 bit resolution, 750 ms conversion time
  }

  result = (float)raw / 16.0;
#ifdef DEBUG_ONEWIRE_TEMP
  Serial.print("Temperature: ");
  Serial.print(result);
  Serial.print(" C");
  Serial.println();
#endif

  return result;
}


void* read_temp_air(struct scheduled_handler* self) {
  static float result = 0;
  byte target[8] = ONEWIRE_TEMP_AIR;

  result = read_temp(target); // -55C - 125C

  return &result;
}

void* read_temp_water(struct scheduled_handler* self) {
  static float result = 0;
  byte target[8] = ONEWIRE_TEMP_WATER;

  result = read_temp(target); // -55C - 125C

  return &result;
}

void* get_wifi_connection(struct scheduled_handler* self) {
  // 0 mean connected, if begger than 0 it is failed count.
  static unsigned int result = 0;

  if(self->result == NULL) {
    // initial state, do nothing
  } else if((result == 0) && (WiFi.status() == WL_CONNECTED)) {
    return &result;
  } else {
    Serial.println("WiFi status is disconnected. Need to reconnect.");
  }

  Serial.printf("Connecting to %s.", ESP_WIFI_SSID);
  
  WiFi.begin(ESP_WIFI_SSID, ESP_WIFI_PASSWD);
 
  // waiting 10 seconds
  for(int i = 0; (i < 20) && (WiFi.status() != WL_CONNECTED); i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if(WiFi.status() == WL_CONNECTED) {
    result = 0;
    self->interval = 10 * (60 * 1000);
    Serial.printf("Connected. IP address is %s.\n", WiFi.localIP().toString().c_str());
    Serial.println("WiFi status will be checked every 10 minutes.");
  } else {
    result = (result < 60)? (result + 1): 1;
    self->interval = result * (60 * 1000);
    Serial.printf("Couldn't connect to access point. status=%d\n", WiFi.status());
    Serial.printf("Reconnect automatically after %d minutes.\n", result);
  }

  return &result;
}

// https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
void* handle_http_request(struct scheduled_handler* self) {
  static unsigned int result = 0;

  if(self->result == NULL) {
    self->interval = 0; // for handleClient()

    http_server.onNotFound([](){
      http_server.send(404, "text/plain", "not found\n");
    });

    http_server.on("/api/led/on", [](){
      Serial.println("/api/led/on");
      find_scheduled_handler("adc_red")->enable = true;
      find_scheduled_handler("adc_green")->enable = true;
      find_scheduled_handler("adc_blue")->enable = true;
      http_server.send(204, "", "\n");
    });

    http_server.on("/api/led/off", [](){
      Serial.println("/api/led/off");
      find_scheduled_handler("adc_red")->enable = false;
      find_scheduled_handler("adc_green")->enable = false;
      find_scheduled_handler("adc_blue")->enable = false;
      *(unsigned int*)find_scheduled_handler("adc_red")->result = 0;
      *(unsigned int*)find_scheduled_handler("adc_green")->result = 0;
      *(unsigned int*)find_scheduled_handler("adc_blue")->result = 0;
      http_server.send(204, "", "\n");
    });

    http_server.on("/api/led/status", [](){
      Serial.println("/api/led/status");
      char buf[38];
      sprintf(buf, "{\"red\":%d,\"green\":%d,\"blue\":%d}\n", 
        *(unsigned int*)find_scheduled_handler("adc_red")->result,
        *(unsigned int*)find_scheduled_handler("adc_green")->result,
        *(unsigned int*)find_scheduled_handler("adc_blue")->result);
      http_server.send(200, "application/json", buf);
    });

    http_server.begin();

    Serial.println("Http server was started.");
  }

  http_server.handleClient();

  return &result;
}

// https://github.com/esp8266/Arduino/blob/master/tools/sdk/include/sntp.h
// https://github.com/esp8266/Arduino/blob/master/tools/sdk/lwip/src/core/sntp.c
void* sync_ntp_server(struct scheduled_handler* self) {
  static uint32 result = 0;

  if(self->result == NULL) {
    sntp_setservername(0, ESP_NTP_SERVER);
    //sntp_set_timezone(9);
    sntp_set_timezone(0);
    sntp_init();
  }

  result = sntp_get_current_timestamp();
  if(result == 0) {
    return NULL;
  }

  return &result;
}

void* print_values(struct scheduled_handler* self) {
  uint32* time = (uint32*)(find_scheduled_handler("ntp_server")->result);
  if(time != NULL) {
    Serial.printf("time=%10d\n", *time);
  }

  unsigned int* red = (unsigned int*)(find_scheduled_handler("adc_red")->result);
  unsigned int* green = (unsigned int*)(find_scheduled_handler("adc_green")->result);
  unsigned int* blue = (unsigned int*)(find_scheduled_handler("adc_blue")->result);
  if(red != NULL && green != NULL && blue != NULL) {
    Serial.printf("red=%4d, ", *red);
    Serial.printf("green=%4d, ", *green);
    Serial.printf("blue=%4d\n", *blue);
  }

  unsigned int* in = (unsigned int*)(find_scheduled_handler("adc_light_in")->result);
  unsigned int* out = (unsigned int*)(find_scheduled_handler("adc_light_out")->result);
  if(in != NULL && out != NULL) {
    Serial.printf("light_in=%4d, ", *in);
    Serial.printf("light_out=%4d\n", *out);
  }

  float* air = (float*)(find_scheduled_handler("temp_air")->result);
  float* water = (float*)(find_scheduled_handler("temp_water")->result);
  if(air != NULL && water != NULL) {
    Serial.print("temp_air=");
    Serial.print(*(float*)(find_scheduled_handler("temp_air")->result));
    Serial.print(", ");
    Serial.print("temp_water=");
    Serial.print(*(float*)(find_scheduled_handler("temp_water")->result));
    Serial.println();
  }

  Serial.println();

  return NULL;
}
