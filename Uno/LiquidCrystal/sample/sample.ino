#include <LiquidCrystal.h>

LiquidCrystal lcd(4, 2, 7, 13, 8, 12);

void setup()
{
  lcd.begin(16, 2);
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("Hello World!");
  
  lcd.setCursor(5, 1);
  char str[] = { 0xd5, 0xaf, 0xb8, 0xd8, 0xbc, 0xc3, 0xb2, 0xaf, 0xc3, 0xc8, 0x21, '\0' };
  lcd.print(str);
}

void loop() {}
