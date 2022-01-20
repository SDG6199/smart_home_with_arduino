#include <SoftwareSerial.h>   //블루투스
SoftwareSerial mySerial(0, 1);

#define SS_PIN 10
#define RST_PIN 9
#define tactbutton 7
#include <SPI.h>         // RFID SPI통신
#include <MFRC522.h>    //MIFARE 기술(RFID)
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; //MFRC522라는 class에서 구조체 MIFARE_Key를 쓰기 위해 범위지정연산자::사용.
char nuidPICC[4]; //새로운 NUID를 저장할 어레이. 

#include <DHT.h>      //dht11
int DHTPIN = 4;
#define DHTTYPE DHT11    
DHT dht(DHTPIN, DHTTYPE);

//---lcd
#include <LiquidCrystal_I2C.h> //i2c: A4 = SCL, A5 = SDA 

//-------변수선언-----
int shiftregister = 0;
char finger[] = { 0,0,0,0 };
int count = 0;
int breakin1, breakoff1 = 0; //디폴트상태 제어
int breakin2, breakoff2 = 0;
int breakin3, breakoff3 = 0;
int past = 0;
int flag = 0;
int stepPin[] = { 14, 15, 16, 17 };
int dir, del, i, j = 0;
//------조명
int light=6;
//------커튼

//-----문잠금
int valve = 0;
int event_valve = 0;
int now_valve = 0;
int past_valve = 0;

int event_unlocklight = 0;
int event_locklight = 0;
int now_locklight = 0;
int past_locklight = 0;
int now_unlocklight = 0;
int past_unlocklight = 0;
//-----tv 
int echoPin = 8;
//----lcd
int lcdAddress = 0x27;  //0x27 or 0x3F.
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(lcdAddress, lcdColumns, lcdRows);
int tvdistanceMm;
String mm;
//-------shift register
int latch = 2;
int clock = 3;
int data = 5;

void setup() {
    Serial.begin(9600);
    mySerial.begin(9600);
    dht.begin();                          //dht11 셋업
    pinMode(tactbutton, INPUT_PULLUP);  //문닫힘확인버튼 셋업
    SPI.begin(); // Init SPI bus
    rfid.PCD_Init(); // Init MFRC522 
    pinMode(light, OUTPUT);
//-----초음파센서
    pinMode(echoPin, INPUT);
    //-----lcd
    lcd.begin();
    lcd.backlight();   //사실없어도 default가 backlight on이다.
    lcd.clear();    // lcd에 print가 (0,0)부터 되도록 clear.
//----shift register
    pinMode(latch, OUTPUT);
    pinMode(clock, OUTPUT);
    pinMode(data, OUTPUT);
    //----motor
    pinMode(stepPin[0], OUTPUT);
    pinMode(stepPin[1], OUTPUT);
    pinMode(stepPin[2], OUTPUT);
    pinMode(stepPin[3], OUTPUT);
}

void loop() {
label1:
    readfinger();   //수신호인식
    getDistance();
    char tagkey[] = { 0x3A, 0xC2, 0x82, 0x80 };  //RFID 태그값. { 0x3A, 0xC2, 0x82, 0x80 }
    breakin1 = 0; breakin2 = 0;
    if (breakoff3 == 1 && finger[2] == '0') {
        breakin3 = 1; //breakin3==1 || finger[2]='1'이 참이되게한다.
    }
    if (breakoff3 == 1 && finger[2] == '1') {
        breakoff3 = 0;
        Serial.println("");
        Serial.println("한번 더 굽히면 5초간 꺼짐");
    }
    if (breakoff1 == 1 && finger[0] == '1') {
        breakin1 = 1;
    }
    if (breakoff2 == 1 && finger[1] == '1') {
        breakin2 = 1;
    }
    breakoff1 = 0; breakoff2 = 0;

    int now = millis();   //에어컨
    now_valve = millis();   //valve
    now_locklight = millis();
    now_unlocklight = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();  //섭씨온도. 화씨는 true
    float hic = dht.computeHeatIndex(t, h, false); //체감온도. false는 celsius, true나 생략은 fherenheit다.

    if ((finger[0] == '1') && (breakin1 == 0)) {
        bitSet(shiftregister, 1);
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);
        digitalWrite(light, HIGH); //중앙light on
        printlightstateon_LCD();
    }
    else {
        bitClear(shiftregister, 1);
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);
        digitalWrite(light, LOW); //중앙light off
        printlightstateoff_LCD();
    }
    if ((finger[1] == '1') && (breakin2 == 0)) {
        i = 0;
        dir = 0;  //커튼 열기
        del = 20;
        while (j < 330) {
            aStep((dir ? (200 - j - 1) : j) % 4);
            delay(del);
            j++;
        }
    }
    else {
        j = 0;
        dir = 1;    //커튼 닫기
        del = 20;
        while (i < 500) {
            aStep((dir ? (400 - i - 1) : i) % 4);
            delay(del);
            i++;
    }
}
    if ((h >= 30 && h <= 50) && (hic >= 0&& hic <= 70)) {  //최적은 60<=h<=70, 19<=hic<=23 
        //습도와 온도 설정범위. 꺼져있는게 디폴트. 현재 31% 24도
        if (finger[2] == '1') {
            //Serial.print(h);  Serial.print(" "); Serial.println(hic); // << 습도온도궁금하면출력해보자
            bitSet(shiftregister, 2);
            digitalWrite(latch, LOW);
            shiftOut(data, clock, MSBFIRST, shiftregister);
            digitalWrite(latch, HIGH);
            printairstateon_LCD();
        }
        else {
            //Serial.print(h);  Serial.print(" "); Serial.println(hic); // << 습도온도궁금하면출력해보자
            bitClear(shiftregister, 2);
            digitalWrite(latch, LOW);
            shiftOut(data, clock, MSBFIRST, shiftregister);
            digitalWrite(latch, HIGH);
            printairstateoff_LCD();
        }
    }
    else { // 설정범위 바깥. 켜져있는게 디폴트.
          //toggle로 끄면 꺼지지만 5초뒤에 에어컨 on이 되게하자. 실사용때는 1시간뒤로하면 실용적.
        if (breakin3 == 1 || finger[2] == '1') { //수신호 ON이였으면 그대로 ON. 
         //Serial.print(h);  Serial.print(" "); Serial.println(hic); // << 습도온도궁금하면출력해보자
            bitSet(shiftregister, 2);
            digitalWrite(latch, LOW);
            shiftOut(data, clock, MSBFIRST, shiftregister);
            digitalWrite(latch, HIGH);
            printairstateon_LCD();
            past = now;
            breakin3 = 0;
        }
        else {  //설정범위 바깥이라 에어컨LED ON이 디폴트지만, 수신호3을 끄면 5초간 OFF됨. 
            if (now - past >= 5000) {
                flag = 1;
            }
            if (flag == 1) {    //5초후 실행 
                breakoff3 = 1;
                flag = 0;
            }
            bitClear(shiftregister, 2);
            digitalWrite(latch, LOW);
            shiftOut(data, clock, MSBFIRST, shiftregister);
            digitalWrite(latch, HIGH);
            printairstateoff_LCD();
        }
    }
    if (finger[3] == '1') {
        bitSet(shiftregister, 3);
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);
        printTVstateon_LCD();

        if (tvdistanceMm <= 80) {
            //Serial.println(tvdistanceMm);
            if (finger[0] == '1' || finger[1] == '1') { //영화관모드on. 커튼걷고 조명끔 
                breakoff1 = 1;
                breakoff2 = 1;
            }
        }
    }
    else {
        bitClear(shiftregister, 3);
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);
        printTVstateoff_LCD();
    }

    //---------------------------문닫히면 잠금

    if (valve == 0) {
        bitClear(shiftregister, 4);
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);
        printdoorstateoff_LCD();
    }
    else {
        bitSet(shiftregister, 4);
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);
    }
    if (digitalRead(tactbutton) == 0) {    //문이 닫히면
        past_valve = now_valve;
        event_valve = 1;
    }
    //---------------------------event
    if (event_valve == 1) {
        if (now_valve - past_valve >= 3000) {
            valve = 0;      //잠금
            event_valve = 0;
        }
    }
    if (event_unlocklight == 1) {
        if (now_unlocklight - past_unlocklight >= 3000) {
            bitClear(shiftregister, 6);
            digitalWrite(latch, LOW);
            shiftOut(data, clock, MSBFIRST, shiftregister);
            digitalWrite(latch, HIGH);
            event_unlocklight = 0;
        }
    }
    if (event_locklight == 1) {
        if (now_locklight - past_locklight >= 3000) {
            bitClear(shiftregister, 5);
            digitalWrite(latch, LOW);
            shiftOut(data, clock, MSBFIRST, shiftregister);
            digitalWrite(latch, HIGH);
            event_locklight = 0;
        }
    }
    //------------------------RFID
    if (!rfid.PICC_IsNewCardPresent()) //새로운 카드가 있으면 넘어간다. 아니면 리턴 후 종료. 이것은 idle될때 전과정을 저장한다.
     goto label1;
    if(!rfid.PICC_ReadCardSerial()){  //재확인과정. 새로운 카드(NUID)가 읽히면 넘어간다. 아니면 리턴 후 종료.
     goto label1;
     }
   Serial.print("PICC type: ");
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
   //Serial.println(rfid.PICC_GetTypeName(piccType));  //모니터에 piccType을 출력.
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&   // PICC가 MIFARE방식인지 확인하고 아니면 리턴.
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println("Your tag is not of type MIFARE Classic.");
        goto label1;
        }
  Serial.println("A card has been detected.");
    for (byte i = 0; i < 4; i++) {  // nuidPICC 어레이에 NUID를 저장한다.
        nuidPICC[i] = rfid.uid.uidByte[i];
    }
    Serial.println("The NUID tag is:");
    Serial.print("In hex: ");
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();

    rfid.PICC_HaltA();// PICC 종료

    if (tagkey[0] == nuidPICC[0] ||     //NUID어레이중에 요소가 하나라도 다르면 다른거다.
        tagkey[1] == nuidPICC[1] ||
        tagkey[2] == nuidPICC[2] ||
        tagkey[3] == nuidPICC[3])
    {
        Serial.println("태그값일치");
        valve = 1;  //잠금해제
        printdoorstateon_LCD();
        bitSet(shiftregister, 6);    //unlocklight on
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);

        bitClear(shiftregister, 5);   //locklight off
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);

        past_unlocklight = now_unlocklight;
        event_unlocklight = 1;
    }
    else {
        Serial.print("태그값불일치");
        bitClear(shiftregister, 6);
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);
        bitSet(shiftregister, 5);
        digitalWrite(latch, LOW);
        shiftOut(data, clock, MSBFIRST, shiftregister);
        digitalWrite(latch, HIGH);
        past_locklight = now_locklight;
        event_locklight = 1;
    }
}
//----------------------함수
void readfinger(){        //폈을때:23 23 2 18   굽혔을때:80 59 55 72
    for(count=0; count<4; count++){
      if (mySerial.available()) {
          finger[count] = mySerial.read(); // 한글자씩읽는다.
          Serial.write(finger[count]);
         }
    }
Serial.println("\t\t\t수신중..");
}
void printHex(byte* buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");  //16(dec), 10(hex)보다 작다? " 0"
        Serial.print(buffer[i], HEX);
    }
}
void getDistance() {
again:// goto를 위한 label
    static int count;

    bitClear(shiftregister, 7);
    digitalWrite(latch, LOW);
    shiftOut(data, clock, MSBFIRST, shiftregister);
    digitalWrite(latch, HIGH);

    delayMicroseconds(2);

    bitSet(shiftregister, 7);
    digitalWrite(latch, LOW);
    shiftOut(data, clock, MSBFIRST, shiftregister);
    digitalWrite(latch, HIGH);

    delayMicroseconds(10);

    bitClear(shiftregister, 7);
    digitalWrite(latch, LOW);
    shiftOut(data, clock, MSBFIRST, shiftregister);
    digitalWrite(latch, HIGH);

    double Duration = 0;    //당연히 단위는 us.
    Duration = pulseIn(echoPin, HIGH);  //triggerPin HIGH로 쏘는순간 HIGH, echoPin으로 받으면 LOW되니까 그사이 pulse를 읽는다.
    tvdistanceMm = Duration / 5.88;

    if ((tvdistanceMm > 4000) && (count < 10)) {  // 거리 측정값이 4000mm보다 클 때, 10번까지 다시 측정(0.2*10=20) 
        count++;
        goto again;
    }
    else if (count >= 10) {    // 거리 측정값이 유효하지 않을 때, goto문 무한 루프를 방지
        tvdistanceMm = 9999;
    }
    count = 0;
}
void aStep(int s) {
    switch (s) {
    case 0:
        digitalWrite(stepPin[0], LOW);
        digitalWrite(stepPin[1], HIGH);
        digitalWrite(stepPin[2], HIGH);
        digitalWrite(stepPin[3], LOW);
        break;
    case 1:
        digitalWrite(stepPin[0], LOW);
        digitalWrite(stepPin[1], HIGH);
        digitalWrite(stepPin[2], LOW);
        digitalWrite(stepPin[3], HIGH);
        break;
    case 2:
        digitalWrite(stepPin[0], HIGH);
        digitalWrite(stepPin[1], LOW);
        digitalWrite(stepPin[2], LOW);
        digitalWrite(stepPin[3], HIGH);
        break;
    case 3:
        digitalWrite(stepPin[0], HIGH);
        digitalWrite(stepPin[1], LOW);
        digitalWrite(stepPin[2], HIGH);
        digitalWrite(stepPin[3], LOW);
        break;
    default:  break;
    }
}
void printlightstateon_LCD() {
    lcd.setCursor(0, 0);
    lcd.print("led: on "); //8자
}
void printlightstateoff_LCD() {
    lcd.setCursor(0, 0);
    lcd.print("led:off "); //8자
}
void printairstateon_LCD() {
    lcd.setCursor(8, 0);
    lcd.print("air: on "); //8자
}
void printairstateoff_LCD() {
    lcd.setCursor(8, 0);
    lcd.print("air:off "); //8자
}
void printTVstateon_LCD() {
    lcd.setCursor(0, 1);
    lcd.print("TV: on  "); //8자
}
void printTVstateoff_LCD() {
    lcd.setCursor(0, 1);
    lcd.print("TV:off  "); //8자
}
void printdoorstateon_LCD() {
    lcd.setCursor(8, 1);
    lcd.print("d:unlock"); //8자
}
void printdoorstateoff_LCD() {
    lcd.setCursor(8, 1);
    lcd.print("d:  lock"); //8자
}
