#include <SoftwareSerial.h>

//gauge for replacing clock in a brz/frs
//3 buttons
//00 = mode change
//hours = show peak for 10 seconds
//minutes = reset peak for that gauge page
//uses obd II uart adapter
//use the simplest arduino

/* parts list:
 clock 
 arduino (cheaper for smaller one)
 wires
 obd II module 
 7 segment serial device
 */



//This is a character buffer that will store the data from the serial port
char rxData[115];
char rxIndex=0;
int buttonModepin = 13;
int buttonPeakpin = 12;
int buttonResetpin = 11;
String modeList[] = {
  "obdbrzoiltempf", "obdafr", "obdvolts"};
//names must be 4 characters long...some alphas don't print ("m")
String modeNames[] = {
  "xOIL", "xAFx", "xULT"};
String modeNames2[] = {"Fxxx", "xxxx", "xxxx"};
int warnLevels[] = {
  212, 220, 15};
int warnSign[] = {
  1,1,1};  //1 for high, 2 for low (in cases like oil pressure)
int peaks[] = {
  0,0,0};
int mode = 1;
int modes = 2;  //actually this means there are 3 modes...0 is the first of the array

SoftwareSerial LCDSerial(9, 10);

void setup(){
  Serial.begin(9600);
  LCDSerial.begin(9600);
  delay(10);
  LCDSerial.print("v"); //reset the display
  //flash custom title
  LCDSerial.print("xxxx");
  //flash name of gauge 
  LCDSerial.print("v");
  LCDSerial.print("xNOT");
  delay(400);
  LCDSerial.print("xOTx");
  delay(300);
  LCDSerial.print("xTxA");
  delay(300);
  LCDSerial.print("xxAx");
  delay(300);
  LCDSerial.print("xAxC");
  delay(300);
  LCDSerial.print("xxCL");
  delay(300);
  LCDSerial.print("xCLC");
  Serial.println("ATZ");
  getResponse();
  Serial.println("ATDP");
  getResponse();
  Serial.println("ATE0");
  getResponse();
  Serial.println("AT SH 7E0");  
  delay(2000);
  getResponse();
  LCDSerial.print("v");
  Serial.flush();

  pinMode(buttonModepin, INPUT);
  pinMode(buttonPeakpin, INPUT);
  pinMode(buttonResetpin, INPUT);
}

void loop(){
  //loop to control modes, etc
  LCDSerial.print("xxxx");
  LCDSerial.print(modeNames[mode]);
  delay(800);
  LCDSerial.print(modeNames2[mode]);
  delay(800);
  LCDSerial.print("v");
  while ( (digitalRead(buttonModepin) == LOW) ){

    if (digitalRead(buttonPeakpin) == HIGH){ //hold down the peaks button to show the peaks
      LCDSerial.print("xxxx");
      if ( ( (int)peaks[mode]/1000) == 0){
        LCDSerial.print("x");
      }
      if ( ( (int)peaks[mode]/100) == 0){
        LCDSerial.print("x");
      }
      if ( ( (int)peaks[mode]/10) == 0){
        LCDSerial.print("x");
      }
      if ( ( (int)peaks[mode]) == 0){
        LCDSerial.print("x");
      }
      LCDSerial.print(peaks[mode]);
      while ( (digitalRead(buttonPeakpin) == HIGH) ){
      } 
    }

    if (digitalRead(buttonResetpin) == HIGH){
      LCDSerial.print("xxxx");
      LCDSerial.print("RXST");
      peaks[mode] = 0;
      while ( (digitalRead(buttonResetpin) == HIGH) ){//if you hold down the peak reset pin...wait a sec and then hit the peak pin...resets obdii and screen
        if ( digitalRead(buttonPeakpin) == HIGH ){
          Serial.println("ATZ");
          getResponse();
          Serial.println("ATDP");
          getResponse();
          Serial.println("ATE0");
          getResponse();
          delay(2000);
          LCDSerial.print("v");
          Serial.flush();
        }
      }
    }

    int value = getOBDIIvalue(modeList[mode]);
    if (value > peaks[mode]){
      peaks[mode] = value;
    }

    if ( ( (int)value/1000) == 0){
      LCDSerial.print("x");
    }
    if ( ( (int)value/100) == 0){
      LCDSerial.print("x");
    }
    if ( ( (int)value/10) == 0){
      LCDSerial.print("x");
    }
    if ( ( (int)value) == 0){
      LCDSerial.print("x");
    }
    LCDSerial.print(value);

    if ( ( (value > warnLevels[mode]) && (warnSign[mode] == 1) ) || ( (value < warnLevels[mode]) && (warnSign[mode] == 0) ) ){
      LCDSerial.print("xxxx");
      delay(45);
      LCDSerial.print(value);
    }
  }
  while ( (digitalRead(buttonModepin) == HIGH) ){
  }
  if (mode >= modes){
    mode = 0;
  }
  else mode++;
}

//from: https://forum.sparkfun.com/viewtopic.php?f=14&t=38253
void getResponse(void){
  char c;
  do {
    if (Serial.available() > 0)
    {
      c = Serial.read();
      if ((c != '>') && (c != '\r') && (c != '\n')) //Keep these out of our buffer
      {
        rxData[rxIndex++] = c; //Add whatever we receive to the buffer
      }
    }
  } 
  while (c != '>'); //The ELM327 ends its response with this char so when we get it we exit out.
  rxData[rxIndex++] = '\0';  //Converts the array into a string
  //Serial.print("rxData: ");
  //Serial.println(rxData);
  rxIndex = 0; //Set this to 0 so next time we call the read we get a "clean buffer
}

long int getOBDIIvalue(String whichSensor){
  /*what is working:
   speed - works 
   rpms - works 
   coolant - reads in F as 10 and nothing else
   boost - can't test yet
   iat and maf - can't tell/don't really care
   volts - works (maybe add decimal?)
   oiltemp(f/c) - haven't tested (shouldn't work on many cars)
   brz oil temp - in F shows -72 and nothing else
   brz fuel left - shows 1 and nothing else
   */
  Serial.flush();
  long int value = 0;
  if (whichSensor.indexOf("obdspeedkph") >= 0){
    Serial.println("010D"); //mode 1 0D PID
    //getResponse();  //command echoed
    getResponse();  //value
    value = strtol(&rxData[6],0,16) ; //convert the string to integer
  }
  if (whichSensor.indexOf("obdspeedmph") >= 0){
    Serial.println("010D"); //mode 1 0D PID
    getResponse();  //value
    value = strtol(&rxData[6],0,16)/1.6 ; //convert the string to integer
  }  
  if (whichSensor.indexOf("obdrpms") >= 0){
    Serial.println("010C"); //mode 1 0C PID (rpm)
    //Serial.println("getting rpm");
    getResponse();  //value
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/4; //aka ((A*256)+B)/4 
    //Serial.println("rpm =");
    //Serial.println(value);
  }
  if (whichSensor.indexOf("obdcoolantc") >=0){
    Serial.println("O1O5");
    getResponse();
    value = (strtol(&rxData[6],0,16)-40); //aka A-40
  }
  if (whichSensor.indexOf("obdcoolantf") >=0){
    //Serial.println("coolantf");
    Serial.println("O1O5"); //gives raw data of "?"
    getResponse();
    value = ((strtol(&rxData[6],0,16))-40)*1.8+32; //aka A-40
    //Serial.print("value of coolantf ");
    //Serial.print(value);
  }
  if (whichSensor.indexOf("obdboost") >= 0){
    Serial.println("0265"); //intake manifold abs pressure
    getResponse();
    value = (strtol(&rxData[6],0,16)); //aka A
  }  
  if (whichSensor.indexOf("obdafr") >= 0){
    Serial.println("0134"); //afr reading (readings aren't great)
    getResponse();
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/32768*14.7;  //(A*256+B)/32768*14.7
  }
  if (whichSensor.indexOf("obdiat") >= 0){
    Serial.println("O1OF");
    getResponse();
    value = (strtol(&rxData[6],0,16)-40); //aka A-40
  }
  if (whichSensor.indexOf("obdmaf") >= 0){
    Serial.println("0110");
    getResponse();
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/100; //aka ((A*256)+B)/100
  }
  if (whichSensor.indexOf("obdvolts") >= 0){
    Serial.println("0142");
    //Serial.println("getting volts");
    getResponse();
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/1000; //aka ((A*256)+B)/100
    //Serial.println("volts is:");
    //Serial.println(value);
  }
  if (whichSensor.indexOf("obdoiltempc") >= 0){
    Serial.println("015C");
    getResponse();
    value = strtol(&rxData[6],0,16)-40; //aka (A-40)
  }
  if (whichSensor.indexOf("obdoiltempf") >= 0){
    Serial.println("015C");
    getResponse();
    value = (strtol(&rxData[6],0,16)-40)*1.8+32; //aka (A-40) *1.8
  }
  
    //nonstandard/experiemental PIDs
  if (whichSensor.indexOf("obdbrzoiltempc") >= 0){
   // Serial1.println("AT SH 7E0");
    //Serial1.flush();
    Serial.println("2101");  
    getResponse();  
    value = ((float)strtol(&rxData[109],0,16) - 40); //29th byte - 40 (?)
    //Serial1.println("AT D");
    //Serial1.println("AT E0");  
    //delay(40);
    //Serial1.println("ATSP6");
  }
  if (whichSensor.indexOf("obdbrzoiltempf") >= 0){ //works
    //Serial1.println("AT SH 7E0");
    //Serial1.flush();
    //getResponse();
    Serial.println("2101");
    getResponse();
    //Serial.println("brz oil temp");
    delay(40);
    //Serial.println(&rxData[109]);
    //Serial.println((float)strtol(&rxData[109],0,16));
    value = ((float)strtol(&rxData[109],0,16) - 40) * 1.8 + 32;
    //Serial.println(value);
    //Serial1.println("AT D"); //these two lines make the next 2101 reading get cut off
    //Serial1.println("AT E0");
    //Serial1.flush();
    //Serial1.println("ATSP6");//doesn't work and is slow (probing for protocol takes work)
    //getResponse();
    //Serial1.flush();
  }
  
  delay(100);
  return value;
}

