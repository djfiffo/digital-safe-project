#include <SoftwareSerial.h>

SoftwareSerial mySerial(2,3);

#define button_submit 7 //pin ปุ่มตกลง/ส่งรหัสผ่าน
#define button_reset 6 //pin ปุ่มรีเซตรหัสผ่าน

//ส่วนที่เกี่ยวกับการรับค่า morseCode
#define startButtonPin 12 //pin ปุ่มเริ่ม/หยุดรับรหัสมอส
#define inputButtonPin 11 //pin ปุ่มส่งรหัสมอส
String morseCode = "";

//Stage of FSM (มี 4 สเตจ)
#define pass_wait 0 //รอรับรหัส
#define pass_valid 1 //รหัสผ่านถูกต้อง
#define pass_invalid 2 //รหัสผ่านไม่ถูกต้อง
#define pass_reset 3 //เปลี่ยนรหัสผ่าน

//ไนซ์ พาร์ตรับรหัสจาก Serial morniter จะอยู่ใน serialEvent() (ดูด้านล่าง)
int passChecking(); //พาร์ตประกอบรหัสเข้าด้วยกัน + เช็กรหัส (ดูด้านล่าง)
String system_password = ""; //Global variable เก็บรหัสผ่านของตู้เซฟ
String input_password;  //Global variable เก็บรหัสผ่านที่ป้อนเข้ามา

String serial_part;     //Global variable เก็บพาร์ตที่อ่านจาก Serial Mornitor หรือ serialEvent();

String potentioPart(); //พาร์ตตัวต้านทานปรับค่าได้ (ดูด้านล่าง)
int A[4] = {A0,A1,A2,A3};
String Potentiometer(int);
String lastPotentioValue = "";

void morsePart(); //พาร์ตรหัสมอส (ดูด้านล่าง)

void resetPassword();

struct State {
  unsigned long ST_Out;
  unsigned long Time;
  unsigned long NextST[8];
};

typedef const struct State SType;
SType FSM[4] = {
  //Bxyz output ; x = Red LED, y = Green LED, z = Door
  //status ; 0 for lock/off 1 for unlock/on
  {B00000000,0,{pass_wait, pass_wait, pass_wait, pass_wait, pass_invalid, pass_valid, pass_invalid, pass_valid}}, //pass_wait
  {B00000011,0,{pass_valid, pass_valid, pass_reset, pass_reset, pass_wait, pass_wait, pass_reset, pass_reset}}, //pass_valid
  {B00000100,3000,{pass_wait, pass_wait, pass_wait, pass_wait, pass_wait, pass_wait, pass_wait, pass_wait}}, //pass_invalid
  {B00000111,0,{pass_reset, pass_reset, pass_reset, pass_reset, pass_wait, pass_wait, pass_wait, pass_wait}}  //pass_reset
};

void setup(){
  Serial.begin(4800);
  mySerial.begin(4800);

  pinMode(button_submit, INPUT_PULLUP);
  pinMode(button_reset, INPUT_PULLUP);

  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(inputButtonPin, INPUT_PULLUP);

  for(int i = 0;i < 4;i++){
    pinMode(A[i], INPUT);
  }
  
}

unsigned long ST = 3; //เริ่มที่ Stage เปลี่ยนรหัสผ่าน
int input, input1, input2, input3;

bool isSubmitPressed = 0; //เงื่อนไขที่ปุ่ม Submit ถูกกด จะเท่ากับ 1
bool isResetPressed = 0;  //เงื่อนไขที่ปุ่ม Reset ถูกกด จะเท่ากับ 1
bool isStartPressed = 0;  //เงื่อนไขที่ปุ่ม Start/Stop ของพาร์ตมอสโค้ด ถูกกด จะเท่ากับ 1

//ส่วนของ debounce ปุ่ม
int lastButtonState_submit = LOW;
int lastButtonState_reset = LOW;
int lastButtonState_start = LOW;
int buttonStage_submit = HIGH;
int buttonStage_reset = HIGH;
int buttonStage_start = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

int lastMySerialPrint;//ตรวจสอบการเปลี่ยนแปลงของ output ที่ส่งไปยังอีกบอร์ด

void loop(){

  Serial.print("State >> ");
  Serial.println(ST);

  if(ST == 2){
    Serial.println("Your password is invalid.");
    Serial.print("System Password : ");
    Serial.println(system_password);
    Serial.print("Input Password :  ");
    Serial.println(input_password);
  }

  //output
  if(lastMySerialPrint != FSM[ST].ST_Out){
    lastMySerialPrint = FSM[ST].ST_Out;
    mySerial.print(FSM[ST].ST_Out); //Output ค่าไปยังบอร์ดที่ 2
  }
  
  delay(FSM[ST].Time);

  //debounce button
  int reading_submit = digitalRead(button_submit);
  int reading_reset = digitalRead(button_reset);

  if (reading_submit != lastButtonState_submit || reading_reset != lastButtonState_reset /*|| reading_start != lastButtonState_start*/) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if(buttonStage_submit != reading_submit){
      buttonStage_submit = reading_submit;
      if(!reading_submit){isSubmitPressed = !reading_submit;}
    }
    if(buttonStage_reset != reading_reset){
      buttonStage_reset = reading_reset;
      if(!reading_reset){isResetPressed = !reading_reset;}
    }
  }
  lastButtonState_submit = reading_submit;
  lastButtonState_reset = reading_reset;

  int reading_start = digitalRead(startButtonPin);
  if(!reading_start) //ถ้าปุ่ม Start/Stop ของพาร์ตมอสถูกกด จะเรียกใช้ฟังก์ชัน morsePart() เพื่อเริ่มบันทึกรหัสมอส
  {
    isStartPressed = true;
    morsePart();
    while(!reading_start){reading_start = digitalRead(startButtonPin);}
    Serial.println("End of the morsePart");
  }

  //ตรวจสอบ input เพื่อเปลี่ยน state
  input1 = isSubmitPressed;
  input2 = isResetPressed;
  input3 = passChecking();
  input = input1*4 + input2*2 + input3;

  //แสดงข้อความการเปลี่ยนสถานะ
  if(ST == 1 && FSM[ST].NextST[input] == 0){
    Serial.println("<Locked Mode>");
    morseCode = "";
  	serial_part = "";
  }
  if(ST == 1 && FSM[ST].NextST[input] == 3){
    Serial.println("<Reset Password Mode>");
  }
  if(ST == 3 && FSM[ST].NextST[input] == 0){
    Serial.println("System password have been changed.");
    resetPassword();
  }
  if(ST == 0 && FSM[ST].NextST[input] == 1){
    Serial.println("Your password is valid.");
    Serial.print("System Password : ");
    Serial.println(system_password);
    Serial.print("Input Password :  ");
    Serial.println(input_password);
  }
  

  ST = FSM[ST].NextST[input];

  isSubmitPressed = 0;
  isResetPressed = 0;
  isStartPressed = 0;
}

void serialEvent() //พาร์ตรับรหัสจาก Serial morniter *หมายเหตุ พาร์ตนี้รันหลังจาก loop() จบแล้วตลอด เลยไม่ได้เรียกใช้ในโค้ด เพราะมันเรียกใช้เองอยู่แล้ว
{
  //สิ่งที่โค้ดทำ : อ่านค่าที่เข้ามาใน Serial.available แล้วนำไปบันทึกเป็นสตริงทับของเดิมในตัวแปร serial_part ที่เป็นตัวแปร Global
  //ไม่มี return เพราะค่าเก็บในตัวแปร global ข้างต้นแล้ว
  if(Serial.available()){
    serial_part = Serial.readStringUntil('\n');
    serial_part.trim();
    Serial.println(serial_part);
  }
}

int passChecking() //พาร์ตประกอบรหัส+เช็กรหัส
{ 
  //สิ่งที่โค้ดทำ : กำหนดให้ตัวแปรGlobalนี้ -> input_password = serial_part + potentioPart() + morse_part
  //return ค่าที่เกิดจากการเทียบ input_password กับ system_password
  input_password ="";
  input_password.concat(serial_part);
  input_password.concat(":");
  lastPotentioValue = potentioPart();
  input_password.concat(lastPotentioValue);
  input_password.concat(":");
  input_password.concat(morseCode);
  if(input_password == system_password){
    return 1;
  }
  else{
    return 0;
  }
}

String potentioPart() //พาร์ตตัวต้านทานปรับค่าได้
{
  int n = 10;
  String Pass[n];
  for(int k = 0;k < n;k++){
    Pass[k] = "";
    for (int i = 0;i < 4;i++){
      Pass[k].concat(Potentiometer(i));
    }
  }
  for(int i = 0;i < n - 1;i++){
    if(Pass[i] != Pass[i + 1]){
      Serial.println("Potentiometer Changed!");
      return lastPotentioValue;
    }
  }
  return Pass[0];
}

String Potentiometer(int i){
  int range[4][10] = {
    {0,14,52,204,375,526,708,874,980,1015},
    {0,102,205,307,409,512,614,716,818,921},
    {0,102,205,307,409,512,614,716,818,921},
    {0,14,52,204,375,526,708,874,980,1015}
  };
  if ( analogRead(A[i])>=range[i][0] && analogRead(A[i])<range[i][1])
    {
    	return "0";
    }
    else if (analogRead(A[i])>=range[i][1] && analogRead(A[i])<range[i][2])
    {
    	return "1";
    }
    else if (analogRead(A[i])>=range[i][2] && analogRead(A[i])<range[i][3])
    {
    	return "2";
    }
    else if (analogRead(A[i])>=range[i][3] && analogRead(A[i])<range[i][4])
    {
    	return "3";
    }
    else if (analogRead(A[i])>=range[i][4] && analogRead(A[i])<range[i][5])
    {
    	return "4";
    }
    else if (analogRead(A[i])>=range[i][5] && analogRead(A[i])<range[i][6])
    {
    	return "5";
    }
    else if (analogRead(A[i])>=range[i][6] && analogRead(A[i])<range[i][7])
    {
    	return "6";
    }
    else if (analogRead(A[i])>=range[i][7] && analogRead(A[i])<range[i][8])
    {
    	return "7";
    }
    else if (analogRead(A[i])>=range[i][8] && analogRead(A[i])<range[i][9])
    {
    	return "8";
    }
    else //range[i][9] - 1023
    {
    	return "9";
    }
}

void morsePart() // ก้อง พาร์ตรหัสมอส
{
  // ตัวแปรควบคุมสถานะ
  bool isReceiving = false;
  bool isPressing = false;
  unsigned long pressStartTime = 0;

  bool firstIn = true;

  // สำหรับ debounce
  int startButtonState = HIGH;
  int inputButtonState;
  int lastStartButtonState = HIGH;
  int lastInputButtonState = HIGH;
  unsigned long lastDebounceTime = HIGH;
  const unsigned long debounceDelay = 50;

  while(isStartPressed){
    unsigned long currentTime = millis();

    int startReading = digitalRead(startButtonPin);
    int inputReading = digitalRead(inputButtonPin);

    if (startReading != lastStartButtonState || inputReading != lastInputButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      // ตรวจสอบการกดปุ่มเริ่ม/หยุด พร้อม debounce
      if(firstIn){
        startReading = LOW;
        firstIn = false;
      }
      if (startReading != startButtonState) {
        startButtonState = startReading;

        if (startButtonState == LOW) {
          isReceiving = !isReceiving;

          if (isReceiving) {
            mySerial.print("B");
            Serial.println("Ready");
            morseCode = "";
          } else {
            mySerial.print("A");
            Serial.print("Receive ");
            Serial.println(morseCode);
            isStartPressed = false;
          }
        }
      }
      
      if (inputReading != inputButtonState) {
        inputButtonState = inputReading;
        // ถ้าอยู่ในโหมดรับค่า
        if (isReceiving) {
          // ตรวจสอบการกดปุ่ม input พร้อม debounce
          if (!isPressing && digitalRead(inputButtonPin) == LOW) {
            isPressing = true;
            pressStartTime = currentTime;
            mySerial.print("D");
          } else {
            if (isPressing && digitalRead(inputButtonPin) == HIGH) {
              isPressing = false;
              mySerial.print("C");
              unsigned long pressDuration = currentTime - pressStartTime;

              if (pressDuration < 300) {
                morseCode += "0";
                Serial.println("get 0");
              } else {
                morseCode += "1";
                Serial.println("get 1");
              }
            }
          }
        }
      }
    }
    lastStartButtonState = startReading;
    lastInputButtonState = inputReading;
  }
}

void resetPassword(){
  system_password = input_password;
  morseCode = "";
  serial_part = "";
}