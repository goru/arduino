#include <Wire.h>

#define PICO_I2C_ADDRESS 0x2E

void setup() {
  Wire.begin();
}

void loop() {
  isReady();

  Wire.beginTransmission(PICO_I2C_ADDRESS);
  Wire.write("yukkurisiteittene\r");
  Wire.endTransmission();

  // send once
  while(1) {}
}

void isReady() {
  while(1) {
    Wire.requestFrom(PICO_I2C_ADDRESS, 1);

    while(Wire.available() > 0){
      if(Wire.read() == '>') {
        return;
      }

      delay(10);
    }
  }
}
