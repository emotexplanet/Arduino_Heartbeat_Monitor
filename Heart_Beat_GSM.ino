//Heart Rate Monitor with SMS Alert
/*
 * Arduino Uno
 * SIM900
 * 
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

int heartRate;
char regCommand[] = "NEW";  //command For registering a new number
char pwdCommand[] = "PWD";  //command For registering a new number
char statusCommand[] = "HBR"; //command for get the heart rate

char phoneNumber[16] = "0"; //set default number here
char password[] = "0000"; //default password
char *p;
char replyBuffer[MESSAGE_LENGTH]; //the reply messages will be store here
boolean sendSMSflag = false;
byte flag_init;

String newNumber;
String newPassword;

boolean HBR_flag = false;
boolean Low_HBR_flag = false;
boolean High_HBR_flag = false;
String sString;

unsigned long HBRStartTime = 0; //count time
unsigned long HBRBeatTime = 0;
double BPM = 0; //calculate the beat per minute

boolean calibrat = true;
int lowThresh = 30;
int highThresh = 150;
int dangerCounter = 0;
int deadCouter = 0;



void setup() {
 Serial.begin(9600);
 pinMode(LO_positive, INPUT);
 pinMode(LO_negative, INPUT);
 Serial.println(F("***********************************"));
 Serial.println(F("GSM Base Heart Monitoring System"));
 Serial.println(F("***********************************"));
 //Load EEPROM
 flag_init = EEPROM.read(0);
 if (flag_init != 0x10)   //init eeprom
 {
  init_eeprom(); // store default number and password
 }
 read_init_eeprom();

 Serial.println(F("Initialize SIM900"));
 while(!SIM900.init()) { // initialize SIM900
  Serial.print("init error\r\n");
  delay(1000);
 }
  Serial.println("SIM900 initialize successfully");
 delay(3000);
 lcd.setCursor(0,0);
 lcd.print("Heart Beat Monitor");
}

void loop() { 
  //SerialDebug();
  ReadSIM900();
  ReadHeartBeat();
  Display();
}

//void SerialDebug(){
//  if (Serial.available())
//  {
//    int inChar = (char)Serial.read();
//    sString += (char)inChar;
//    if (inChar == '\n')
//    {
//      SIM900.print(sString);
//      sString = ""; //clear buffer
//    }
//  }
//}


void ReadHeartBeat(){
  if((digitalRead(LO_positive) ==  1) || (digitalRead(LO_positive) ==  1)){
    Serial.println("!");
  }
  else{
    //Send the value through analog input A0
    heartRate = analogRead(A0);
    Serial.println(heartRate);
    if(heartRate > 100){
      High_HBR_flag = true;
    }
    else{
      if(heartRate < 40){
        Low_HBR_flag = true;
      }
      else{
        Low_HBR_flag = false;
        High_HBR_flag = false;
      }
    }
    
  }
}

void Display(){
  lcd.setCursor(1, 0);
  lcd.print("HBR: ");
  lcd.print(heartRate);
  lcd.print("bpm");
}

void ReadSIM900(){
  int messageIndex = SIM900.isSMSunread();
  if(messageIndex > 0){  //if there is an unread sms read it.
    char datetime[24];
    Serial.println(F("Message receive"));
    char message[MESSAGE_LENGTH];
    char phoneNum[16];
    SIM900.readSMS(messageIndex, message, MESSAGE_LENGTH, phoneNum, datetime);
    boolean numberOk = false;
    sendSMSflag = false;
    boolean statusFlag = false;
    String str1, str2;
    str1 = phoneNumber;
    str2 = phoneNum;
    if ((str1 == "0") || (str1 == str2)) {
      numberOk = true;
    }
    else {
      numberOk = false;
    }

    if (check_command(message, regCommand)) {
      newNumber.toCharArray(phoneNumber, 16);
      write_number(phoneNumber, 0x10);
      reply_msg();
    }
    else {
      if (check_password_command(message, pwdCommand)) {
        newPassword.toCharArray(password, 16);
        write_number(phoneNumber, 0x40);
        reply_msgPWD();
      }

      else {
        if (check_command(message, statusCommand)) {
          statusFlag = true;          
        }
      }
    }
    
    if (sendSMSflag && numberOk && !statusFlag) {
      send_msg(phoneNum);
    }
    else {
      if (!sendSMSflag && numberOk && !statusFlag) {
        String msg = "Command Error!!!";
        msg.toCharArray(replyBuffer, 39);                 // Copies String to the buffer
        send_msg(phoneNum);
      }
      else{
        if (sendSMSflag && numberOk && statusFlag) {
          send_HBR_Status();
        }
      }
    }
    
    SIM900.deleteSMS(messageIndex);
    
  }

}



void write_number(char *number, short address) {
  unsigned short i;
  if (strlen (number) > 3) {
    EEPROM.write(0, 0);
    for (i = 0; i < sizeof(number) - 1; i++)
    {
      EEPROM.write(i + address, number[i]);
    }
    EEPROM.write(i + address, 0);
  }
}

void init_eeprom()
{
  Serial.println(F("Init EEPROM"));
  write_number(phoneNumber, 0x10);
  write_number(password, 0x40);

  flag_init = EEPROM.read(0x00);

}

void read_number(char *number, short address) {
  for (int i = 0; i < 19; i++)
  {
    number[i] = EEPROM.read(i + address);
  }
  number[19] = '\0';
  Serial.println(number);
}

void read_password(char *pwd, short address) {
  for (int i = 0; i < 4; i++)
  {
    pwd[i] = EEPROM.read(i + address);
  }
  pwd[4] = '\0';
  Serial.println(pwd);
}

void read_init_eeprom()
{
  read_number(phoneNumber, 0x10);
  read_password(password, 0x40);

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

boolean check_password(String myString)
{
  int index1 = myString.indexOf('*');
  int index2 = myString.indexOf('#', index1 + 1);
  String defPassword = myString.substring(index1 + 1, index2);
  if (defPassword == password)
    return true;
  else
    return false;
}

boolean check_password_number(String myString)
{
  int index1 = myString.indexOf('*');
  int index2 = myString.indexOf('*', index1 + 1);
  int index3 = myString.indexOf('#', index2 + 1);
  String defPassword = myString.substring(index1 + 1, index2);
  newNumber = myString.substring(index2 + 1, index3);
  if (defPassword == password)
    return true;
  else
    return false;
}

boolean check_password_password(String myString)
{
  int index1 = myString.indexOf('*');
  int index2 = myString.indexOf('*', index1 + 1);
  int index3 = myString.indexOf('#', index2 + 1);
  String defPassword = myString.substring(index1 + 1, index2);

  if (defPassword == password)
  {
    newPassword = myString.substring(index2 + 1, index3);
    return true;
  }
  return false;
 
}

boolean check_command(char *fonaInBuffer, const char *command) {
  p = strstr(fonaInBuffer, command);
  if (p) {
    sendSMSflag = true;
    if (check_password_number(p)) {
      return true;
    }
  }
  return false;
}

boolean check_password_command(char *fonaInBuffer, const char *command) {
  p = strstr(fonaInBuffer, command);
  if (p) {
    sendSMSflag = true;
    if (check_password_password(p)) {
      return true;
    }
  }
  return false;
}



void reply_msg() {
  String msg = "The ";
  msg += newNumber;
  msg += " have been successfully registered.";
  msg.toCharArray(replyBuffer, 39);                 // Copies String to the buffer
}

void reply_msgPWD() {
  String msg = "New password have been set successfully.";
  msg.toCharArray(replyBuffer, 39);                 // Copies String to the buffer
}

void send_HBR_sms() {
  String msg;
  if(!Low_HBR_flag && !High_HBR_flag){
    msg = "Normal Heart Rate " + heartRate;
  }
  else{
    msg = "Emergency!!! ";
    if(Low_HBR_flag){
      msg = "Low Heart Rate " + heartRate;
    }
    else{
      if(High_HBR_flag){        
        msg = "High Heart Rate " + heartRate;
      }
    }
  }
  msg.toCharArray(replyBuffer, 100);
  
  send_msg(phoneNumber);
}


void send_HBR_Status() {
  String msg = "The Heart Beat is: " + heartRate;
  
  msg.toCharArray(replyBuffer, 100);
  
  send_msg(phoneNumber);
}
