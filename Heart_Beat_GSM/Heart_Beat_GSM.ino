//Heart Rate Monitor with SMS Alert
/*
 * Arduino Uno
 * SIM900
 * AD8232
 * 
 * 
 * 
 * 
 * 
 */
#include <GPRS_Shield_Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

#define PIN_TX 3
#define PIN_RX 4
#define BAUDRATE 9600

#define MESSAGE_LENGTH 160



LiquidCrystal lcd(10, 9, 8, 7, 6, 5); // create lcd//
//Parameters: (rs, enable, d4, d5, d6, d7)
GPRS SIM900(PIN_TX, PIN_RX, BAUDRATE); //RX, TX, BaudRate


unsigned short LO_positive = 11;
unsigned short LO_negative = 12;


char phoneNumber[20] = "+234*************"; //set default number here
char replyBuffer[MESSAGE_LENGTH]  = ""; //the reply messages will be store here


boolean HBR_flag = false;
boolean Low_HBR_flag = false;
boolean High_HBR_flag = false;
String sString;


int UpperThreshold = 518;
int LowerThreshold = 495;

int dataSignal = 0;
int value = 0;
int BPM = 0;

boolean calibrateBeat = false;
boolean intervalFlag = false;

unsigned long lastTime = 0;
unsigned long beatInterval = 0;

unsigned long previousTime = 0;
const unsigned long smsTime = 600000;

boolean sendMsgFlag = false;


const unsigned long HBRDelay = 10;
const unsigned long DispDelay = 1000;

unsigned long previousHBR = 0;
unsigned long previousDisp = 0;
unsigned long previousSMS = 0;


void setup() {
 lcd.begin(16, 2);
 Serial.begin(9600);
 pinMode(LO_positive, INPUT);
 pinMode(LO_negative, INPUT);
 pinMode(LED_BUILTIN, OUTPUT);
 digitalWrite(LED_BUILTIN, LOW);
 lcd.setCursor(0,0);
 lcd.print("Initializing");
 Serial.println(F("***********************************"));
 Serial.println(F("GSM Base Heart Monitoring System"));
 Serial.println(F("***********************************"));
 //Load EEPROM
 delay(1000);
 Serial.println(phoneNumber);
 lcd.setCursor(0,0);
 lcd.print("Initializing.");
 delay(1000);
 lcd.setCursor(0,0);
 lcd.print("Initializing..");
 delay(1000);
 Serial.println(F("Initialize SIM900"));
 while(!SIM900.init()) { // initialize SIM900
  Serial.print("init error\r\n");
  delay(1000);
  lcd.setCursor(0,0);
  lcd.print("GSM Error!     ");
 }
 lcd.setCursor(0,0);
 lcd.print("Initializing...");
 Serial.println("SIM900 initialize successfully");
 delay(1000);
 lcd.setCursor(0,0);
 lcd.print("Device is Ready!");
 //String msg = "The device is ready!!!!";
 //msg.toCharArray(replyBuffer, 100);
 //send_msg(phoneNumber);
 delay(3000);
 lcd.setCursor(0,0);
 lcd.print("_Heartbeat Rate_");
}

void loop() {
  unsigned long currentTime = millis();
  if(HBREvent(HBRDelay, currentTime)){ 
     ReadHeartBeat();
  }
  
  if(DispEvent(DispDelay, currentTime)){
    Display();
  }
  
  if(SMSEvent(smsTime, currentTime)){
    Serial.print(F("Send.....sms"));
    if(sendMsgFlag){
      send_HBR_sms();
    }
  }
}




void ReadHeartBeat(){
  if((digitalRead(LO_positive) ==  1) || (digitalRead(LO_positive) ==  1)){
    //Serial.println("!");
  }
  else{
    value = analogRead(0);
    if(value > UpperThreshold){
      if(calibrateBeat){
        beatInterval = millis() - lastTime;
        BPM = (int)(60/((float)(beatInterval/1000)));
        intervalFlag = false;
        calibrateBeat = false;
      }
      if(!intervalFlag){
        lastTime = millis();
        intervalFlag = true;
      }
    }

    if((value < LowerThreshold) && (intervalFlag)){
      calibrateBeat = true;
    }
  }
   if(BPM > 100){
      High_HBR_flag = true;
      sendMsgFlag = true;
    }
    else{
      if(BPM < 40){
        Low_HBR_flag = true;
        sendMsgFlag = true;
      }
      else{
        Low_HBR_flag = false;
        High_HBR_flag = false;
        sendMsgFlag = false;
      }
    } 

    dataSignal = analogRead(0);

    if(dataSignal > UpperThreshold){
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else{
      digitalWrite(LED_BUILTIN, LOW);
    }
}

void Display(){
  Serial.print(F("The raw value is: "));
  Serial.println(value);
  Serial.print(F("The HBR is: "));
  Serial.println(BPM);
  lcd.setCursor(0, 1);
  lcd.print("  ");
  lcd.print(BPM);
  lcd.print("bpm     ");  
}

void send_msg(char * number) {
  Serial.println(replyBuffer);
  if (strlen (number) > 3)
  {
    if (!SIM900.sendSMS(number, replyBuffer))
    {
      Serial.println(F("Sent!"));
    }
    else
    {
      Serial.println(F("Failed!"));
    }
    delay(3000);
  }
}



void send_HBR_sms() {
  String msg;
  msg = "Emergency!!! ";
  if(Low_HBR_flag){
    msg = "Low Heart Rate " + (int) BPM;
  }
  else{
    if(High_HBR_flag){        
      msg = "High Heart Rate " + (int)BPM;
    }
  }
  msg.toCharArray(replyBuffer, 100);
  
  send_msg(phoneNumber);
}


boolean HBREvent(unsigned long HBRTime, unsigned long currentTime){
  if((currentTime - previousHBR) >= HBRDelay){
    previousHBR = currentTime;
    return true;
     Serial.println("HBR Event True............................");
  }
  else{
    return false;
  }
}

boolean DispEvent(unsigned long DispTime, unsigned long currentTime){
  if((currentTime - previousDisp) >= DispTime){
    previousDisp = currentTime;
    return true;
    Serial.println("Display Event True............................");
  }
  else{
    return false;
  }
}

boolean SMSEvent(unsigned long smsT, unsigned long currentTime){
  if((currentTime - previousSMS) >= smsT){
    previousSMS = currentTime;
    return true;
    Serial.println("SMS Event True............................");
  }
  else{
    return false;
  }
}




