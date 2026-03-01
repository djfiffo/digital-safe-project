#include <SoftwareSerial.h>
#include <Servo.h>

Servo Lock;

SoftwareSerial mySerial(2,3);

#define servo 10
#define green_LED 12
#define red_LED 13

#define morseLED 7
#define morseBuzzer 6

int last_output;

void setup(){
  Serial.begin(4800);
  mySerial.begin(4800);
  Lock.attach(servo);
  pinMode(green_LED, OUTPUT);
  pinMode(red_LED, OUTPUT);
  pinMode(morseLED, OUTPUT);
  pinMode(morseBuzzer, OUTPUT);

  Lock.write(0);
  digitalWrite(green_LED, LOW);
  digitalWrite(red_LED, LOW);
  digitalWrite(morseLED, LOW);
  noTone(morseBuzzer);
}

void loop(){
  if(mySerial.available() > 0){
    int read = mySerial.read();

    Serial.print(read);

    if(read == 'A' || read == 'B'){
      digitalWrite(morseLED, read - 'A');
    }
    else if(read == 'C'){
      noTone(morseBuzzer);
    }
    else if(read == 'D'){
      tone(morseBuzzer, 1000);
    }
    else{
      Serial.print("\t");
      read -= '0';
      int Lock_Read = !(bitRead(read, 0));
      int Green_Read = !(bitRead(read, 1));
      int Red_Read = !(bitRead(read, 2));

      int survo_angel = 90 * Lock_Read;
      Lock.write(survo_angel);//Lock
      Serial.print("0 = ");Serial.print(Lock_Read);

      digitalWrite(green_LED, Green_Read);//Green
      Serial.print("\t1 = ");Serial.print(Green_Read);

      digitalWrite(red_LED, Red_Read);//Red
      Serial.print("\t2 = ");Serial.println(Red_Read);
    }
  }
}