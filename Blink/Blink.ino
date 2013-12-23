int RED = 3;
int BLUE = 5;
int GREEN = 6;

// the setup routine runs once when you press reset:
void setup() {          
//   Serial.begin(9600);  
}

// the loop routine runs over and over again forever:
void loop() {
  blink(0, 100, 100);
}

void blink(int red, int blue, int green) {
  int m = max(red, max(blue, green));
  
  int d = 1000 / m;
  
  double r = (double)red / m;
  double b = (double)blue / m;
  double g = (double)green / m;
  
//  Serial.println(r);
//  Serial.println(g);
//  Serial.println(b);
//  Serial.println();
  
  for (int i = 0; i <= m; i++) {
    analogWrite(RED, r * i);
    analogWrite(BLUE, g * i);
    analogWrite(GREEN, b * i);
    delay(d);
  }
  
  for (int i = m; i >= 0; i--) {
    analogWrite(RED, r * i);
    analogWrite(BLUE, g * i);
    analogWrite(GREEN, b * i);
    delay(d);
  }
  
  delay(500);
}
