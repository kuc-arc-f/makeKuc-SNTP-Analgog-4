
/*  OLED + Analog Sensor(LM60BIZ )
   + Analog(Moisture Sensor)
 , NTP receive sample. (Atmega328P )
 BETA ver: 0.9.1
*/
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Time.h>
#include <TimeLib.h> 

//define
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
// T1262347200  //noon Jan 1 2010

SoftwareSerial mySerial(5, 6); /* RX:D5, TX:D6 */
const int mMoistPin = 0;
const int mVoutPin = 1;
uint32_t mTimerTmp;
uint32_t mTimerTime;
uint32_t mReceive_Start=0;
int mTemp=0;
int mMoist=0;
String mTimeStr = "";
int mDispMode=0;
int mDispModeTime   =1;
int mDispModeAnalog =2;
//
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };
  
//
void setup() {
  mDispMode= mDispModeTime;
  Serial.begin( 9600 );
  mySerial.begin( 9600 );
  Serial.println("#Start-setup-SS");
  pinMode(mMoistPin, INPUT);
  pinMode(mVoutPin, INPUT);
  //
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.display();
  delay(2000);
  display.clearDisplay(); 
}
//
long convert_Map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
//
// reading Moisture
int getMoisture(){
  int iRet=0;
  float fSen  = 0;
  unsigned long reading  = 0;   
  int iDiv=3;
  for (int i=0; i<iDiv ; i++) {
    int  iTmp = analogRead(mMoistPin);
//Serial.print("get.iTmp=");
//Serial.println(iTmp);
    reading  += iTmp; 
    delay(50);
  }
  int SValue= reading / iDiv;
  int voltage=convert_Map(SValue, 0, 1000, 0,3300);  // V
//Serial.print("SValue=");
//Serial.print(SValue);
//Serial.print(" , voltage=");
//Serial.println(voltage);
  int iMaxVoltage= 2048;
  //float fMoi =(voltage * 100) / iMaxVoltage;
  float fMoi = (float)voltage / (float)iMaxVoltage;  
//Serial.print( "f.fMoi= " );
//Serial.println( fMoi );   
  fMoi= fMoi * 100 ;
  int iMoi= (int)fMoi;  
//Serial.print( "Moisture Value= " );
//Serial.println( iMoi ); 
  iRet= iMoi;
  return iRet;  
}

// reading LM60BIZ
int getTempNum(){
  int iRet=0;
  float fSen  = 0;
  unsigned long reading  = 0;
  int iDiv =3;  
  //for (int i=0; i<10; i++) { 
  for (int i=0; i< iDiv; i++) {
    int  iTmp = analogRead(mVoutPin);
//Serial.print("get.iTmp=");
//Serial.println(iTmp);
    reading  += iTmp; 
    delay(50);
  }
  int SValue= reading / iDiv;
  int voltage=convert_Map(SValue, 0, 1000, 0,3300);  // V
//Serial.print("SValue=");
//Serial.print(SValue);
//Serial.print(" , voltage=");
//Serial.println(voltage);
  int iTemp = (voltage - 424) / 6.25; //電圧値を温度に変換, offset=425
  iRet= iTemp;
  
  return iRet;  
}
//
void display_OLED(String sTime  ){
  //String sBuff ="Temp:"+ sTemp+ "C";
  String sBuff ="T: "+ String(mTemp) + "C";
  String sMoi ="M: " + String(mMoist) + "%";
  display.setTextSize(2);
  display.setCursor(0,0);
  display.setTextColor(WHITE);
  if(mDispMode==mDispModeTime){
      display.println( sTime );
      display.println("");
      mDispMode= mDispModeAnalog;
  }else{
      display.println(sMoi);
      display.println(sBuff);
      mDispMode = mDispModeTime; 
  }
  display.display();
  delay(100);  
  display.clearDisplay();
}
//
boolean Is_validHead(String sHd){
  boolean ret=false;
  if(sHd=="d1"){
    ret= true;
  }else if(sHd=="d2"){
    ret= true;
  }
  return ret;
}
String  digitalClockDisplay(){
  String sRet="";
  // digital clock display of the time
  char cD1[8+1];
  time_t t = now();
  const char* fmtSerial = "%02d:%02d:%02d";
  sprintf(cD1, fmtSerial, hour(t), minute(t), second(t));
//Serial.println(cD1);
  sRet=String(cD1);
  return sRet;  
}
//
void conv_timeSync(String src){
  //String sRet="";
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 
   pctime = src.toInt();
   if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
     setTime(pctime); // Sync Arduino clock to the time received on the serial port
   }  
   //return sRet;
}
String mBuff="";
//
void proc_uart(){
    while( mySerial.available() ){
      char c= mySerial.read();
      mBuff.concat(c );
      if(  mBuff.length() >= 13 ){
        String sHead= mBuff.substring(0,2);
        boolean bChkHd= Is_validHead( sHead );
        if(bChkHd== true){
// Serial.println( "Hd="+ sHead );
Serial.println( "mBuff="+ mBuff );
          String sTmp= mBuff.substring(3,13);
          conv_timeSync(sTmp);
           //send
           char cD1[10+1];
           char cD2[10+1];
           sprintf(cD1 , "d1%08d", mTemp);       
           sprintf(cD2 , "d2%08d", mMoist );       
           mySerial.print( cD1 );
           delay(100 );           
           mySerial.print( cD2 );
           mReceive_Start=millis();
//Serial.print("cD1=");
//Serial.println(cD1  );  
// Serial.println("cD2=" + String(cD2 ));
        }        
        mBuff="";
      }else{
          if(mReceive_Start > millis()+ 10000 ){
            mBuff="";
          }
      }
    } //end_while
}

//
void loop() {
    proc_uart();
    if (millis() > mTimerTmp) {
       mTimerTmp = millis()+ 5000;
       mTemp = getTempNum();
       mMoist =getMoisture();
Serial.println("mTemp="  +String(mTemp ));
Serial.println("mMoist="  +String(mMoist ));            
    }
    if (millis() > mTimerTime) {
       mTimerTime = millis()+ 2000;
       mTimeStr= digitalClockDisplay();
       display_OLED(mTimeStr  );
    }

}











