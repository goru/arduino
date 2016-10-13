#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <OneWire.h>

extern "C" {
  #include "sntp.h"
}

//#define DEBUG_ADC_REG
//#define DEBUG_ADC_LIGHT

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
  {true, 0,  500, "adc_light_in",     read_adc_light_in,   NULL},
  {true, 0,  500, "adc_light_out",    read_adc_light_out,  NULL},
//  {true, 0,  100, "temp_air",         read_temp_air,       NULL},
//  {true, 0,  100, "temp_water",       read_temp_water,     NULL},
  {true, 0, 1000, "wifi_connection",  get_wifi_connection, NULL},
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
  
  result = readAdc(ADC_RED); // 0-1023
#ifdef DEBUG_ADC_REG
  Serial.print("adc_red=");
  Serial.print(result);
  Serial.println();
#endif

  return &result;
}

void* read_adc_green(struct scheduled_handler* self) {
  static unsigned int result = 0;
  
  result = readAdc(ADC_GREEN); // 0-1023
#ifdef DEBUG_ADC_REG
  Serial.print("adc_green=");
  Serial.print(result);
  Serial.println();
#endif

  return &result;
}

void* read_adc_blue(struct scheduled_handler* self) {
  static unsigned int result = 0;
  
  result = readAdc(ADC_BLUE); // 0-1023
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
  
  result = calculate_lux(readAdc(ADC_LIGHT_IN)); // adc:0-1024, lux:0-1000
#ifdef DEBUG_ADC_LIGHT
  Serial.print("adc_light_in=");
  Serial.print(result);
  Serial.println();
#endif

  return &result;
}

void* read_adc_light_out(struct scheduled_handler* self) {
  static unsigned int result = 0;

  result = calculate_lux(readAdc(ADC_LIGHT_OUT)); // adc:0-1024, lux:0-1000
#ifdef DEBUG_ADC_LIGHT
  Serial.print("adc_light_out=");
  Serial.print(result);
  Serial.println();
#endif

  return &result;
}

void* read_temp_air(struct scheduled_handler* self) {
  static unsigned int result = 0;
//  byte i;
//  byte present = 0;
//  byte data[12];
//  byte addr[8];
//
//  onewire.reset_search();
//  if(!onewire.search(addr)) {
//    Serial.print("No more addresses.\n");
//    onewire.reset_search();
//    return;
//  }
//
//  Serial.print("R=");
//  for( i = 0; i < 8; i++) {
//    Serial.print(addr[i], HEX);
//    Serial.print(" ");
//  }
//
//  if ( OneWire::crc8( addr, 7) != addr[7]) {
//      Serial.print("CRC is not valid!\n");
//      return;
//  }
//
//  if ( addr[0] == 0x10) {
//      Serial.print("Device is a DS18S20 family device.\n");
//  }
//  else if ( addr[0] == 0x28) {
//      Serial.print("Device is a DS18B20 family device.\n");
//  }
//  else {
//      Serial.print("Device family is not recognized: 0x");
//      Serial.println(addr[0],HEX);
//      return;
//  }
//
//  onewire.reset();
//  onewire.select(addr);
//  onewire.write(0x44,1);         // start conversion, with parasite power on at the end
//
//  delay(1000);     // maybe 750ms is enough, maybe not
//  // we might do a ds.depower() here, but the reset will take care of it.
//
//  present = onewire.reset();
//  onewire.select(addr);    
//  onewire.write(0xBE);         // Read Scratchpad
//
//  Serial.print("P=");
//  Serial.print(present,HEX);
//  Serial.print(" ");
//  for ( i = 0; i < 9; i++) {           // we need 9 bytes
//    data[i] = onewire.read();
//    Serial.print(data[i], HEX);
//    Serial.print(" ");
//  }
//  Serial.print(" CRC=");
//  Serial.print( OneWire::crc8( data, 8), HEX);
//  Serial.println();

  return &result;
}

void* read_temp_water(struct scheduled_handler* self) {
  static unsigned int result = 0;
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

  return &result;
}

void* print_values(struct scheduled_handler* self) {
  Serial.printf("time=%10d, ", *(uint32*)(find_scheduled_handler("ntp_server")->result));
  Serial.printf("red=%4d, ", *(unsigned int*)(find_scheduled_handler("adc_red")->result));
  Serial.printf("green=%4d, ", *(unsigned int*)(find_scheduled_handler("adc_green")->result));
  Serial.printf("blue=%4d, ", *(unsigned int*)(find_scheduled_handler("adc_blue")->result));
  Serial.printf("light_in=%4d, ", *(unsigned int*)(find_scheduled_handler("adc_light_in")->result));
  Serial.printf("light_out=%4d", *(unsigned int*)(find_scheduled_handler("adc_light_out")->result));
//  Serial.printf("temp_air=%4d, ", *(unsigned int*)(find_scheduled_handler("temp_air")->result));
//  Serial.printf("temp_water=%4d\n", *(unsigned int*)(find_scheduled_handler("temp_water")->result));
  Serial.println();
  return NULL;
}
