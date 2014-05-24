/** vim: set et sw=2 sts=2 ts=2 ft=cpp : **/

#include <TinySoftPwm.h>
#include <DigiUSB.h>

#define PIN_RED   0
#define PIN_BLUE  1
#define PIN_GREEN 2

#define INDEX_RED   0
#define INDEX_GREEN 1
#define INDEX_BLUE  2

void setup()
{
   TinySoftPwm_begin(128, 0); /* 128 x TinySoftPwm_process() calls before overlap (Frequency tuning), 0 = PWM init for all declared pins */
   DigiUSB.begin();
}

void loop()
{
  static uint8_t rgb[3] = { 0, 0, 128 };

  pwmProcess();
  pwmWrite(rgb);

  readFromUSB(rgb);
}

void pwmProcess()
{
  static uint32_t m = micros();

  /***********************************************************/
  /* Call TinySoftPwm_process() with a period of 60 us       */
  /* The PWM frequency = 128 x 60 # 7.7 ms -> F # 130Hz      */
  /* 128 is the first argument passed to TinySoftPwm_begin() */
  /***********************************************************/
  if ((micros() - m) < 60) {
    return;
  }

  /* We arrive here every 60 microseconds */
  m = micros();

  TinySoftPwm_process(); /* This function shall be called periodically (like here, based on micros(), or in a timer ISR) */
}

void pwmWrite(uint8_t* rgb)
{
  static uint32_t m = millis();
  static uint8_t pwmStep = 0;
  static int8_t dir = 1;

  /*************************************************************/
  /* Increment/decrement PWM on LED Pin with a period of 10 ms */
  /*************************************************************/
  if ((millis() - m) < 10) {
    return;
  }

  /* We arrived here every 10 milliseconds */
  m = millis();

  analogWrite(
    PIN_RED,   ((float)rgb[INDEX_RED]   / 255) * pwmStep);
  TinySoftPwm_analogWrite(
    PIN_GREEN, ((float)rgb[INDEX_GREEN] / 255) * pwmStep);
  analogWrite(
    PIN_BLUE,  ((float)rgb[INDEX_BLUE]  / 255) * pwmStep);

  pwmStep += dir;

  if (pwmStep == 255) {
    dir = -1;
  } else if (pwmStep == 0) {
    dir = 1;
    DigiUSB.delay(1000);
  }
}

void readFromUSB(uint8_t* rgb)
{
  static uint8_t i = 0, j = 0;
  static char buf[] = "255";

  DigiUSB.refresh();

  if (!DigiUSB.available()) {
    return;
  }

  char c = DigiUSB.read();
  switch (c) {
    case 'r':
      i = INDEX_RED;   j = 0;
      break;
    case 'g':
      i = INDEX_GREEN; j = 0;
      break;
    case 'b':
      i = INDEX_BLUE;  j = 0;
      break;
  }

  if ((j < 3) && ('0' <= c) && (c <= '9')) {
    buf[j++] = c;
    if (j == 3) {
      rgb[i] = min(atoi(buf), 255);
//      DigiUSB.println(rgb[i], DEC);
    }
  }
}
