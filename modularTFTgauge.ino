/*modular gauges...
 a new start to the 1.8" tft based gauges
 most things are configured in the SD card config file, "gauges" 
 
 Modular
 3 types of gauge
 many types of sensor
 you choose the order
 you choose the peaks/warns/etc.
 */

/* TODO's
 -peaks displayed and clear by page turn
 -logging done properly
 -accelerometer display
 -page turnning by button
 -background testing of all sensors with "popup" alerting
 */

#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <SD.h>

//pin reference for tft from;
/*http://webshed.org/wiki/18tftbreakout
 eBay Board	 Adafruit Board	Arduino conections
 VCC	 VCC	 5V
 BKL	 LITE	 GND on eBay, 5V on Adafruit
 RESET	 RESET	 8
 RS	 D/C	 9
 MISO	 MISO	 12 (50 for mega)
 MOSI	 MOSI	 11 (51 for mega)
 SCLK	 SCLK	 13 (52 for mega)
 LCD CS	 TFT CS	 10 (53 for mega)
 SD CS	 CARD CS 4	
 GND	 GND	 GND
 */
#define SD_CS   4
#define LCD_CS  53
#define LCD_DC  9
#define LCD_RST 8

int chipSelect = 10; //for adafruit SD shields + tfts
Adafruit_ST7735 tft = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST);

int buttonApin = 13;
int fakeSensor = 0;

File config;
uint16_t background = ST7735_BLACK;
uint16_t outline = ST7735_WHITE;
uint16_t fill = ST7735_BLUE;
uint16_t textdefault = ST7735_RED;
uint16_t alert = ST7735_YELLOW;

//This is a character buffer that will store the data from the serial port
char rxData[30];
char rxIndex=0;

void setup() {
  //read some basic settings from the SD card
  Serial.begin(9600); //debugging to serial console
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(background);
  tft.setRotation(1);

  Serial.print("SD card start\n");
  if (!SD.begin(SD_CS)){
    Serial.println("failed to initialize SD");
    return;
  }
  Serial.println("SD OK");
  config = SD.open("gauges");
  //get name of splash from config file and display it
  String splash = searchFile("splash");
  //Serial.println("splash"+ splash);
  unsigned int splashLen = splash.length()-1;
  char splashc[splashLen];
  splash.toCharArray(splashc, (splashLen));
  config.close();
  bmpDraw(splashc, 0, 0);

  Serial1.begin(9600); //spi for the display, serial for debug, serial1 for OBD II

  //read and assign color configs
  config = SD.open("gauges");
  background = textColorToColor(searchFile("background"));
  outline = textColorToColor(searchFile("outline"));
  fill = textColorToColor(searchFile("fill"));
  textdefault = textColorToColor(searchFile("textdefault"));
  alert = textColorToColor(searchFile("alert"));

  //blank screen
  delay(2000);
  Serial1.println("ATZ");
  delay(2000);
  Serial1.flush();
  pinMode(buttonApin, INPUT);
  Serial.println("end of setup");
  tft.fillScreen(background);
}

void loop() {
  String sensor1, sensor2, sensor3, sensor4;
  String sensor1text, sensor2text, sensor3text, sensor4text;
  unsigned int sensor1pin, sensor2pin, sensor3pin, sensor4pin; //regular sensors get pins...but will check if sensor gets pin of 0...marks obdII
  unsigned int sensor1max, sensor2max, sensor3max, sensor4max;
  unsigned int sensor1alert, sensor2alert, sensor3alert, sensor4alert;
  String sensor1units, sensor2units;
  //read config file for next page
  String pagetype = searchFile("pagetype");
  Serial.println(pagetype);

  if (pagetype.indexOf("twobar") >= 0){//2 sensors displayed in 2 bar charts
    sensor1 = searchFile("sensor1");
    sensor2 = searchFile("sensor2");
    sensor1pin = searchFile("sensor1pin").toInt();
    sensor2pin = searchFile("sensor2pin").toInt();
    sensor1text = searchFile("sensor1text");
    sensor2text = searchFile("sensor2text");
    sensor1max = searchFile("sensor1max").toInt();
    sensor2max = searchFile("sensor2max").toInt();
    sensor1alert = searchFile("sensor1alert").toInt();
    sensor2alert = searchFile("sensor2alert").toInt();
    sensor1units = searchFile("sensor1units");
    sensor2units = searchFile("sensor2units");
    tft.fillScreen(background);
    tft.setTextSize(1);
    tft.setTextColor(textdefault);
    tft.setCursor(0,0);
    tft.println(sensor1text);
    tft.setCursor(0,70);
    tft.println(sensor2text);
    tft.setCursor(145,0);
    tft.println(sensor1units);
    tft.setCursor(145,70);
    tft.println(sensor2units);
    long val1;
    long valOld1 = 0;
    long val2;
    long valOld2 = 0;
    uint16_t barColor1;
    uint16_t barColor2;
    Serial.println("twobar init done");
    while (digitalRead(buttonApin) == LOW){
      Serial.println(buttonApin);
      val1 = getSensorReading(sensor1, sensor1pin);
      val2 = getSensorReading(sensor2, sensor2pin);
      //write value
      tft.setCursor(100,0); //blank old value first
      tft.setTextColor(background);
      tft.println(valOld1);
      tft.setCursor(100,0);
      tft.setTextColor(textdefault);
      tft.println(val1);
      tft.setCursor(100,70); //blank old value first
      tft.setTextColor(background);
      tft.println(valOld2);
      tft.setCursor(100,70);
      tft.setTextColor(textdefault);
      tft.println(val2);
      //pick the bar color1
      if (val1 >= sensor1alert){
        if(barColor1 != alert){
          barColor1 = alert;
          valOld1 = 0;
        }
      }
      if (val1 <= sensor1alert) {
        if(barColor1 != fill){
          barColor1 = fill;
          if(val1 <= valOld1){
            valOld1 = 0;
          }
        }
      }
      //pick the bar color2
      if (val2 >= sensor2alert){
        if(barColor2 != alert){
          barColor2 = alert;
          valOld2 = 0;
        }
      }
      if (val2 <= sensor2alert) {
        if(barColor2 != fill){
          barColor2 = fill;
          if(val2 <= valOld1){
            valOld2 = 0;
          }
        }
      }
      //draw bar1
      if (val1 >= valOld1){//if the bar is longer...add
        tft.fillRect( ( ( (float)160/sensor1max)*valOld1 - 1), 20, ( ((float)160/sensor1max)*(val1-valOld1) + 1), 30, barColor1  );
      }
      if (val1 < valOld1){//if the bar is shorter...erase
        tft.fillRect( ( (float)160/sensor1max*val1), 20, ( (float)160/sensor1max*valOld1), 30, background );
      }
      valOld1 = val1;      
      //draw bar2
      if (val2 >= valOld2){//if the bar is longer...add
        tft.fillRect( ( ( (float)160/sensor2max)*valOld2 - 1), 90, ( ((float)160/sensor2max)*(val2-valOld2) + 1), 30, barColor2  );
      }
      if (val2 < valOld2){//if the bar is shorter...erase
        tft.fillRect( ( (float)160/sensor2max*val2), 90, ( (float)160/sensor2max*valOld2), 30, background );
      }
      valOld2 = val2;
    }
  }

  else if (pagetype.indexOf("onebar") >=0){//1 sensor 1 bar chart...bigger fonts
    Serial.println("onebar");
    sensor1 = searchFile("sensor1");
    sensor1pin = searchFile("sensor1pin").toInt();
    sensor1text = searchFile("sensor1text");
    sensor1max = searchFile("sensor1max").toInt();
    sensor1alert = searchFile("sensor1alert").toInt();
    sensor1units = searchFile("sensor1units");
    //draw template stuff
    tft.fillScreen(background);
    tft.setTextSize(2);
    tft.setCursor(0,5);
    tft.setTextColor(textdefault);
    tft.println(sensor1text);
    tft.setCursor(50,100);
    tft.println(sensor1units);
    long val;
    long valOld = 0;
    uint16_t barColor;
    //loop to show the display and check for the button press
    while  (digitalRead(buttonApin) == LOW){ 
      Serial.println("button low...getting reading");
      //get value
      val = getSensorReading(sensor1, sensor1pin);
      Serial.println("reading is:");
      Serial.println(val);
      //write value
      tft.setCursor(0,100); //blank old value first
      tft.setTextColor(background);
      tft.println(valOld);
      tft.setCursor(0,100);
      tft.setTextColor(textdefault);
      tft.println(val);
      //pick the bar color              ///somehow this isnt right
      if (val >= sensor1alert){
        if(barColor != alert){
          barColor = alert;
          valOld = 0;
        }
      }
      if (val <= sensor1alert) {
        if(barColor != fill){
          barColor = fill;
          if(val <= valOld){
            valOld = 0;
          }
        }
      }
      //draw bar
      if (val >= valOld){//if the bar is longer...add
        tft.fillRect( ( ( (float)160/sensor1max)*valOld - 1), 40, ( ((float)160/sensor1max)*(val-valOld) + 1), 40, barColor  );
      }
      if (val < valOld){//if the bar is shorter...erase
        tft.fillRect( ( (float)160/sensor1max*val), 40, ( (float)160/sensor1max*valOld), 40, background );
      }
      valOld = val;
    } 

  }

  else if (pagetype.indexOf("round") >= 0){
    sensor1 = searchFile("sensor1");
    sensor1pin = searchFile("sensor1pin").toInt();
    sensor1text = searchFile("sensor1text");
    sensor1max = searchFile("sensor1max").toInt();
    sensor1alert = searchFile("sensor1alert").toInt();
    tft.fillScreen(background);
    tft.setTextSize(2);
    tft.setTextColor(textdefault);
    tft.setCursor(0, 2);
    tft.println(sensor1text);
    tft.drawCircle(80, 70, 50, textdefault);
    tft.drawCircle(80, 70, 49, textdefault);
    long val;
    long valOld = 0;
    uint16_t barColor;
    int angle = 0;
    float rad = 0;
    while  (digitalRead(buttonApin) == LOW){
      val = getSensorReading(sensor1, sensor1pin);
      tft.setCursor(65,60);
      tft.setTextColor(background);
      tft.println(valOld);
      tft.setTextColor(textdefault);
      tft.setCursor(65,60);
      tft.println(val);
      if (val >= sensor1max){
        barColor = alert;
      }
      else {
        barColor = fill;
      }
      if (val > valOld){//if the bar is longer...add
        //270/sensor1max gives scaling for a 270 degree gauge
        for(angle = ((float)valOld*270/sensor1max); angle <= (((float)val*270/sensor1max)); angle+=1){ //the "+=1" can be changed for more "fill in" of the arc (gets noticeable at 5)
          rad = angle * PI / 180;
          tft.fillCircle( (80-((int)(sin(rad)*43.0) ) ), (70+( (int)(cos(rad)*43.0))), 5,barColor);
        }
      }
      if (val < valOld){//if the bar is shorter...erase
        for(angle = (((float)valOld*270/sensor1max)); angle >= ((float)val*270/sensor1max); angle-=1){ //the "+=1" can be changed for more "fill in" of the arc (gets noticeable at 5)
          rad = angle * PI / 180;
          tft.fillCircle( (80-((int)(sin(rad)*43.0) ) ), (70+( (int)(cos(rad)*43.0))), 5,background);
        }
      }
      valOld = val;
    }
  }
  
  if (pagetype.indexOf("accelerometer") >= 0){//cross bar type chart for accelerometer
    //special...just show the accelerometer and get the accelerometer data
    tft.fillScreen(background);
    while (digitalRead(buttonApin) == LOW){
      //display and refresh the page here
    }
  }

  //TODO: add logging after 10 seconds
  else if (pagetype.indexOf("logging") >= 0){//up to 4 sensors shown...log everything to file
    /*tft.fillScreen(background);
     tft.setTextColor(textdefault);
     tft.setCursor(40, 5);
     tft.println("Logging will begin in 10 seconds");
     for (int d = 0;d >= 10; d++){
     if (digitalRead(buttonApin) == LOW){
     loop(); //start over again if we aren't logging
     }
     delay(100);
     }*/
    sensor1 = searchFile("sensor1");
    sensor2 = searchFile("sensor2");
    sensor3 = searchFile("sensor3");
    sensor4 = searchFile("sensor4");
    sensor1pin = searchFile("sensor1pin").toInt();
    sensor2pin = searchFile("sensor2pin").toInt();
    sensor3pin = searchFile("sensor3pin").toInt();
    sensor4pin = searchFile("sensor4pin").toInt();
    //close the settings file
    config.close();
    //draw a blank logging page here:
    tft.fillScreen(background);
    tft.setTextColor(textdefault);
    tft.setTextSize(2);
    tft.setCursor(40, 5);
    tft.println("Logging");
    tft.setTextSize(1);
    tft.setCursor(10, 40); 
    tft.println(sensor1);
    tft.setCursor(10, 60); 
    tft.println(sensor2);
    tft.setCursor(10, 80); 
    tft.println(sensor3);
    tft.setCursor(10, 100); 
    tft.println(sensor4);

    //open a new logging file
    long v1, v2, v3, v4;
    long v1o = 0; 
    long v2o = 0; 
    long v3o = 0; 
    long v4o = 0;

    while  (digitalRead(buttonApin) == LOW){ 
      v1 = getSensorReading(sensor1, sensor1pin);
      v2 = getSensorReading(sensor2, sensor2pin);
      v3 = getSensorReading(sensor3, sensor3pin);
      v4 = getSensorReading(sensor4, sensor4pin);

      //show logged stuff
      tft.setTextColor(background);
      tft.setCursor(110,40); 
      tft.println(v1o);
      tft.setCursor(110,60); 
      tft.println(v2o);
      tft.setCursor(110,80); 
      tft.println(v3o);
      tft.setCursor(110,100); 
      tft.println(v4o);
      tft.setTextColor(outline);
      tft.setCursor(110,40); 
      tft.println(v1);
      tft.setCursor(110,60); 
      tft.println(v2);
      tft.setCursor(110,80); 
      tft.println(v3);
      tft.setCursor(110,100); 
      tft.println(v4);

      v1o = v1;
      v2o = v2;
      v3o = v3;
      v4o = v4;
    }
    //close file
    //reopen the config file
    config = SD.open("gauges");    
  }
  
}

long int getOBDIIvalue(String whichSensor){
  long int value = 0;
  if (whichSensor.indexOf("obdspeedkph") >= 0){
    Serial1.println("010D"); //mode 1 0D PID
    getResponse();  //command echoed
    getResponse();  //value
    value = strtol(&rxData[6],0,16) ; //convert the string to integer
  }
  if (whichSensor.indexOf("obdspeedmph") >= 0){
    Serial1.println("010D"); //mode 1 0D PID
    getResponse();  //command echoed
    getResponse();  //value
    value = strtol(&rxData[6],0,16)/1.6 ; //convert the string to integer
  }  
  if (whichSensor.indexOf("obdrpms") >= 0){
    Serial1.println("010C"); //mode 1 0C PID (rpm)
    getResponse();  //command echoed
    getResponse();  //value
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/4; //aka ((A*256)+B)/4 
  }
  if (whichSensor.indexOf("obdcoolant") >=0){
    Serial1.println("O1O5");
    getResponse();
    getResponse();
    value = (strtol(&rxData[6],0,16))-4; //aka A-40
  }
  if (whichSensor.indexOf("obdboost") >= 0){
    Serial1.println("0265"); //intake manifold abs pressure
    getResponse();
    getResponse();
    value = (strtol(&rxData[6],0,16)); //aka A
  }  
  if (whichSensor.indexOf("obdiat") >= 0){
    Serial1.println("O1OF");
    getResponse();
    getResponse();
    value = (strtol(&rxData[6],0,16)); //aka A-40
  }
  if (whichSensor.indexOf("obdcoolant") >=0 ){
    Serial1.println("O1O5");
    getResponse();
    getResponse();
    value = (strtol(&rxData[6],0,16))-4; //aka A-40
  }
  if (whichSensor.indexOf("obdmaf") >= 0){
    Serial1.println("0110");
    getResponse();
    getResponse();
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/100; //aka ((A*256)+B)/100
  }
  if (whichSensor.indexOf("obdvolts") >= 0){
    Serial1.println("0142");
    getResponse();
    getResponse();
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/1000; //aka ((A*256)+B)/100
  }
  if (whichSensor.indexOf("obdoiltempC") >= 0){
    Serial1.println("015C");
    getResponse();
    getResponse();
    value = strtol(&rxData[6],0,16)-40; //aka (A-40)
  }
  if (whichSensor.indexOf("obdoiltempc") >= 0){
    Serial1.println("015C");
    getResponse();
    getResponse();
    value = (strtol(&rxData[6],0,16)-40)*1.8; //aka (A-40)
  }
  if (whichSensor.indexOf("obdbrzoiltempc") >= 0){
    Serial1.println("0110");
    getResponse();
    getResponse();
    value = ( strtol(&rxData[93],0,16) ) - 40; //29th byte - 40 (?)
  }
  if (whichSensor.indexOf("obdbrzoiltempf") >= 0){
    Serial1.println("0110");
    getResponse();
    getResponse();
    value = ( (strtol(&rxData[93],0,16) ) - 40) * 1.8; //29th byte - 40 (?)
  }
  Serial1.flush();
  return value;
}

int getSensorReading(String sensorName, int pinNumber){
  //if the pin number is 0 it is obd II...look it up with the obd II lib
  if (pinNumber == 0){
    return(getOBDIIvalue(sensorName));  
  }
  //else call the appropriate Analog to digital conversion function on the appropirate pin
  else {
    if (sensorName.indexOf("fake") >= 0){
      return lookup_fake_random_sensor(pinNumber);
    }
    if (sensorName.indexOf("oiltemp") >= 0){
      return lookup_oil_temp(pinNumber);
    }
    if (sensorName.indexOf("oilpressure") >= 0){
      return lookup_oil_psi(pinNumber);
    }
    if (sensorName.indexOf("accelx") >= 0){
      return getAccelerometerData(pinNumber);
    }
    if (sensorName.indexOf("accely") >= 0){
      return getAccelerometerData(pinNumber);
    }
    if (sensorName.indexOf("boostpressure") >= 0){
      return lookup_boost(pinNumber);
    }
    if (sensorName.indexOf("temperature") >= 0){
      return lookup_temp(pinNumber);
    }

  }
}

//the following function is from the sparkfun example code: https://github.com/sparkfun/OBD-II_UART/blob/master/Firmware/obdIIUartQuickstart.ino
//The getResponse function collects incoming data from the UART into the rxData buffer
// and only exits when a carriage return character is seen. Once the carriage return
// string is detected, the rxData buffer is null terminated (so we can treat it as a string)
// and the rxData index is reset to 0 so that the next string can be copied.
void getResponse(void){
  char inChar=0;
  //Keep reading characters until we get a carriage return
  while(inChar != '\r'){
    //If a character comes in on the serial port, we need to act on it.
    if(Serial.available() > 0){
      //Start by checking if we've received the end of message character ('\r').
      if(Serial.peek() == '\r'){
        //Clear the Serial buffer
        inChar=Serial.read();
        //Put the end of string character on our data string
        rxData[rxIndex]='\0';
        //Reset the buffer index so that the next character goes back at the beginning of the string.
        rxIndex=0;
      }
      //If we didn't get the end of message character, just add the new character to the string.
      else{
        //Get the new character from the Serial port.
        inChar = Serial.read();
        //Add the new character to the string, and increment the index variable.
        rxData[rxIndex++]=inChar;
      }
    }
  }
}



uint16_t textColorToColor(String color){
  Serial.println(color);
  if (color.indexOf("red") >= 0){
    return ST7735_RED;
  }
  else if (color.indexOf("magenta") >= 0){
    return ST7735_MAGENTA;
  }
  else if (color.indexOf("blue") >= 0){
    return ST7735_BLUE;
  }
  else if (color.indexOf("green") >= 0){
    return ST7735_GREEN;
  }
  else if(color.indexOf("black") >= 0){
    return ST7735_BLACK;
  }
  else  if (color.indexOf("white") >= 0){
    return ST7735_WHITE;
  }
  else if (color.indexOf("yellow") >=0){
    return ST7735_YELLOW;
  }
  else {
    return ST7735_WHITE;
  }
}

String searchFile(String searchFor){ //finds some substring + : and returns the value after the :
  if (!SD.exists("gauges")){
    Serial.println("config file not found try reformatting sd card");
    return("error");
  }
  //set pos to start of file
  //loop and read a line and check
  String line;
  //Serial.println(config.available());
  while (config.available()){
    line = config.readStringUntil('\n');
    if (line.startsWith("#")){ //skip comments
    }
    else if (line.startsWith(searchFor)){
      int colonPos = line.indexOf(":");
      return( line.substring( line.indexOf(":")+1, line.indexOf("\n")-3 ) );
    }
  }
}

//fake sensor for testing
int lookup_fake_random_sensor(int max){
  randomSeed(analogRead(0));
  int mod = (random(-5,5));
  //for increment
  fakeSensor+=mod;
  if (fakeSensor >= max){
    fakeSensor = 1;
  }
  if (fakeSensor < 0){
    fakeSensor = 1;
  }
  Serial.println("lookup_fake_random_sensor");
  Serial.println(fakeSensor);
  return fakeSensor;
}

//sensor code
int getAccelerometerData (int axis){
  int zerog = 512;
  int rc = analogRead(axis);
  int top =( (zerog - rc) ) ; 
  float frtrn = (((float)top/(float)154)*100);  //158Vint jumps are 1g for the ADXL213AE (original accel)
  //154Vint jumps are 1g for the ADXL322 (updated one)
  int rtrn = (int)frtrn;
  return rtrn;
}

//oil temp
long lookup_oil_temp(int oilTempPin){
  long tval = analogRead(oilTempPin);
  tval = tval * 1000; //added an extra 0
  if (tval <= 11500){
    return (9999); 
  }
  if (tval >= 68100){
    return (0);
  }
  if ((tval <= 68000)&&(tval > 39600)){
    return (long)(((tval-134266)*10)/(-473));
  }
  if ((tval <= 39600)&&(tval > 28200)){
    return (long)(((tval-115600)*10)/(-380));
  }
  if ((tval <= 28200)&&(tval > 19700)){
    return (long)(((tval-93366)*10)/(-283));
  }  
  if ((tval <= 19700)&&(tval > 11600)){
    return (long)(((tval-54800)*10)/(-135));
  }  
}

//oil pressure
long lookup_oil_psi(int psiPin){
  long psival = analogRead(psiPin);
  if (psival > 722){
    return (0);
  }
  if (psival < 257){
    return(9999);
  }
  if ((psival <= 722)&&(psival > 619)) {
    return 1747 - (psival*240)/100; 
  } 
  if ((psival <= 619)&&(psival > 520)) {
    return 1802 - (psival*250)/100;
  }
  if ((psival <= 520)&&(psival > 411)) {
    return 1694 - (psival*230)/100;     
  }
  if ((psival <= 411)&&(psival > 257)){
    return 1418 - (psival*160)/100;
  }
} 

long lookup_boost(int boostPin){
  long boost = analogRead(boostPin);
  //boost = ( (boost-106000) / 259000 );
  // boost = ( (( boost * 398) / 1000) + 2); //2 is the y intercept
  //398 changed to 378 for slope...because slope was too steep
  boost = ( (( boost * 378) / 1000) - 4); ///10; //get rid of the divide by ten when adding decimals on display
  return boost;
}

long lookup_temp(int tempPin){
  long tval = analogRead(tempPin);
  tval = tval * 100;
  //tval = (long)(tval - (long)117588);
  //return tval;
  if (tval < 8900){
    return (9999); 
  }
  if (tval > 96000){
    return (0);
  }
  if ((tval <= 96000)&&(tval > 93221)){
    return (((tval-101577)*10)/(-172));
  }
  if ((tval <= 93221)&&(tval > 89610)){
    return (((tval-104201)*10)/(-226));
  }
  if ((tval <= 89610)&&(tval > 85125)){
    return (((tval-107738)*10)/(-280));
  }
  if ((tval <= 85125)&&(tval > 79139)){
    return (((tval-112264)*10)/(-335));
  }
  if ((tval <= 79139)&&(tval > 70799)){
    return (((tval-117588)*10)/(-388));
  }
  if ((tval <= 70799)&&(tval > 62470)){
    return (((tval-121441)*10)/(-421));
  }
  if ((tval <= 62470)&&(tval > 53230)){
    return (((tval-122367)*10)/(-428));
  }
  if ((tval <= 53230)&&(tval > 43707)){
    return (((tval-118651)*10)/(-405));
  }
  if ((tval <= 43707)&&(tval > 36471)){
    return (((tval-111349)*10)/(-366));
  }
  if ((tval <= 36471)&&(tval > 30685)){
    return (((tval-102232)*10)/(-321));
  }
  if ((tval <= 30685)&&(tval > 24800)){
    return (((tval-9078)*10)/(-270));
  }
  if ((tval <= 24800)&&(tval > 20000)){
    return (((tval-78575)*10)/(-220));
  }
  if ((tval <= 20000)&&(tval > 15851)){
    return (((tval-66507)*10)/(-175));
  }
  if ((tval <= 15851)&&(tval > 12380)){
    return (((tval-55300)*10)/(-137));
  }
  if ((tval <= 12380)&&(tval > 9085)){
    return (((tval-41752)*10)/(-94));
  }
}


//bmp drawing code from adafruit
#define BUFFPIXEL 20

void bmpDraw(char *filename, uint8_t x, uint8_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print("File not found");
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print("File size: "); 
    Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print("Image Offset: "); 
    Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print("Header size: "); 
    Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print("Bit Depth: "); 
      Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print("Image size: ");
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
          pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.Color565(r,g,b));
          } // end pixel
        } // end scanline
        Serial.print("Loaded in ");
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println("BMP format not recognized.");
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}



