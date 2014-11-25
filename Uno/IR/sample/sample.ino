#define SW_BTN_PIN  2
#define IR_RECV_PIN 3
#define IR_LED_PIN  4
#define PULSE_DELAY 8

unsigned int irData[] = {1031,511,66,61,67,59,69,188,66,61,70,61,66,62,66,61,67,188,70,186,66,62,66,62,66,62,70,187,67,62,66,187,67,61,70,62,66,188,66,62,67,187,70,187,66,62,66,62,66,188,70,187,66,62,66,188,66,62,70,62,66,187,67,187,66,62,67,0};

void setup () {
  Serial.begin(57600);
  
  pinMode(SW_BTN_PIN,  INPUT);
  pinMode(IR_RECV_PIN, INPUT);
  pinMode(IR_LED_PIN,  OUTPUT);
}

void irRecv () {
  Serial.print(".");
  
  unsigned int prevIR = HIGH;
  unsigned long prevTime = micros();
  
  unsigned int curIR;
  unsigned long curTime;
  
  while ((curTime = micros()) - prevTime < 1000000) {
    if ((curIR = digitalRead(IR_RECV_PIN)) == prevIR) {
      continue;
    }
    
    Serial.print((curTime - prevTime) / 10);
    Serial.print(',');
    
    prevIR = curIR;
    prevTime = curTime;
  }
  
  Serial.println();
}

void pulseOn (unsigned long length) {
  unsigned long startTime = micros();
  unsigned int flag = LOW;
  
  while ((micros() - startTime) < length) {
    digitalWrite(IR_LED_PIN, flag = !flag);
    delayMicroseconds(PULSE_DELAY);
  }
}

void pulseOff (unsigned long length) {
  digitalWrite(IR_LED_PIN, LOW);
  delayMicroseconds(length);
}

void irSend () {
  Serial.println("Send");
  
  unsigned int irDataSize = sizeof(irData) / sizeof(irData[0]);
  for (unsigned int i = 0; i < irDataSize; i++) {
    unsigned long length = irData[i] * 10;
    if (i % 2 == 0) {
      pulseOn(length);
    } else {
      pulseOff(length);
    }
  }

  delay(500);
}

void loop () {
  if (digitalRead(SW_BTN_PIN) == LOW) {
    irRecv();
  } else {
    irSend();
  }
}
