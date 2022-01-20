#include <SoftwareSerial.h>
SoftwareSerial mySerial(2,3); 

int finger1 = A0;
int finger2 = A1;
int finger3 = A2;
int finger4 = A3;
int i1,i2,i3,i4=0;

void setup(){
  Serial.begin(9600);
  mySerial.begin(9600);
}


void loop(){
 if (Serial.available()) {    
    mySerial.write(Serial.read()); 
  }//없어도됨. 그냥 블루투스 테스트용.
  int SensorReading1 = analogRead(finger1);
  int SensorReading2 = analogRead(finger2); 
  int SensorReading3 = analogRead(finger3); 
  int SensorReading4 = analogRead(finger4);        
  Serial.print("검지:"); Serial.print(SensorReading1);
  Serial.print("  중지:"); Serial.print(SensorReading2);
  Serial.print("  약지:"); Serial.print(SensorReading3);
  Serial.print("  소지:"); Serial.println(SensorReading4);
if(SensorReading1<=600){
  toggle1();
  }
if(SensorReading2<=635){
  toggle2();
  }
if(SensorReading3<=600){
  toggle3();
  }
if(SensorReading4<=600){
  toggle4();
  }
mySerial.print(i1); mySerial.print(i2); mySerial.print(i3); mySerial.print(i4);
 Serial.print(i1);  Serial.print(i2);  Serial.print(i3);  Serial.println(i4);
delay(2000);
}

void toggle1(){
 i1^=1;
  }
void toggle2(){
 i2^=1;
  }
void toggle3(){
 i3^=1;
  }
void toggle4(){
 i4^=1;
  }
  
