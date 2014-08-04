#include <SD.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <SPI.h>


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
 short:
 -logging done properly to file
 -debug obd II stuffs
 -fix 0 glitch on bar charts
 -make check for obd II and if it isn't there then don't freeze up
 long:
 -find an elegant solution to negative warnings (oil pressure)
 -make graphics scalable to the 2.2" tft spi monitors
 -code cleanup
 -more error catching
 -use arrays instead of individual variables...this will clean up the code and might make stuff go faster
 -make s**t fucking awesome
 */

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

/* Other connections
 4 wire for accelerometer 
 10 for tft
 2 wire for uart obd (+1 for gnd)
 obd II db9 - pre-assembled 5' cable (so the obd II uart device should be co-located with the mega)
 Connector styles available:
 spade - 1
 jst - 2
 molex - 4 (but large)
 rj45 - 8 pin
 */

#define SD_CS   4
#define LCD_CS  53
#define LCD_DC  9
#define LCD_RST 8

//1.8" tft
#define LCDx   160
#define LCDy   128

/*
2.2" 240 x 320
 */

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
uint16_t severe = ST7735_YELLOW;

//This is a character buffer that will store the data from the serial port
char rxData[115];
char rxIndex=0;

String sensor1monitor, sensor2monitor, sensor3monitor, sensor4monitor, sensor5monitor, sensor6monitor;
int monitorpin1, monitorpin2, monitorpin3, monitorpin4, monitorpin5, monitorpin6;
int monitoralert1, monitoralert2, monitoralert3, monitoralert4, monitoralert5, monitoralert6;
int monitorsevere1, monitorsevere2, monitorsevere3, monitorsevere4, monitorsevere5, monitorsevere6;
int monitorinterval = 30;
boolean alerting = false;
int a = 0;


void setup() {
  //read some basic settings from the SD card
  Serial.begin(9600); 
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

  Serial1.begin(9600); //spi for the display, serial for debug, serial for OBD II

  //read and assign color configs
  config = SD.open("gauges");
  background = textColorToColor(searchFile("background"));
  outline = textColorToColor(searchFile("outline"));
  fill = textColorToColor(searchFile("fill"));
  textdefault = textColorToColor(searchFile("textdefault"));
  alert = textColorToColor(searchFile("alert"));
  severe = textColorToColor(searchFile("severe"));

  searchFile("allgauges");
  monitorinterval = searchFile("monitorinterval").toInt();
  sensor1monitor = searchFile("sensor1");
  sensor2monitor = searchFile("sensor2");
  sensor3monitor = searchFile("sensor3");
  sensor4monitor = searchFile("sensor4");
  sensor5monitor = searchFile("sensor5");
  sensor6monitor = searchFile("sensor6");
  monitorpin1 = searchFile("sensor1pin").toInt();
  monitorpin2 = searchFile("sensor2pin").toInt();
  monitorpin3 = searchFile("sensor3pin").toInt();
  monitorpin4 = searchFile("sensor4pin").toInt();
  monitorpin5 = searchFile("sensor5pin").toInt();
  monitorpin6 = searchFile("sensor6pin").toInt();
  monitoralert1 = searchFile("sensor1alert").toInt();
  monitoralert2 = searchFile("sensor2alert").toInt();
  monitoralert3 = searchFile("sensor3alert").toInt();
  monitoralert4 = searchFile("sensor4alert").toInt();
  monitoralert5 = searchFile("sensor5alert").toInt();
  monitoralert6 = searchFile("sensor6alert").toInt();  
  monitorsevere1 = searchFile("sensor1severe").toInt();
  monitorsevere2 = searchFile("sensor2severe").toInt();
  monitorsevere3 = searchFile("sensor3severe").toInt();
  monitorsevere4 = searchFile("sensor4severe").toInt();
  monitorsevere5 = searchFile("sensor5severe").toInt();
  monitorsevere6 = searchFile("sensor6severe").toInt();

  config.close();
  config = SD.open("gauges");
  delay(2000);
  Serial1.println("ATZ"); //begin OBDII UART
  getResponse2();
  delay(1000);
  //delay(20);
  //getResponse();
 // Serial1.println("ATDP");//tell which protocol...the brz is ATDPAUTO, ISO 15765-4 (CAN 11/500)
 // getResponse();
 // Serial1.println("ATDPN");//number of the protocol
 // getResponse();
  //Serial1.flush();
  //delay(100);
  Serial1.println("ATE0"); //echo off
  getResponse2();
  delay(1000);
  //Serial1.flush();
  //delay(100);
  Serial1.println("AT SH 7E0"); //header specific to the brz oil temp reading
  getResponse2(); //won't work because of "OK" ?
  delay(1000);
  Serial1.println("0105");
  getResponse2();
  Serial.println((strtol(&rxData[6],0,16))-40); //this works, but reading it down in the code doesn't?!?!
  /*Serial1.println("0-20");
  Serial1.println("0100");  //what pids are supported 01-20
  getResponse();
  Serial1.println("21-40");
  Serial1.println("0120"); //what other pids are supported 20-40
  getResponse();
  Serial1.println("41-60");
  Serial1.println("0140"); //40-60
  getResponse();
  Serial1.println("61-80");
  Serial1.println("0160"); //60-80
  getResponse();*/
  delay(2000);
  Serial1.flush();  //TRY THIS on and off
  pinMode(buttonApin, INPUT);
  Serial.println("end of setup");
  tft.fillScreen(background);
}

void loop() {
  int peaksensor1, peaksensor2, peaksensor3, peaksensor4, peaksensor5, peaksensor6;
  String sensor1, sensor2, sensor3, sensor4, sensor5, sensor6;
  String sensor1text, sensor2text, sensor3text, sensor4text;
  unsigned int sensor1pin, sensor2pin, sensor3pin, sensor4pin, sensor5pin, sensor6pin; //regular sensors get pins...but will check if sensor gets pin of 0...marks obdII
  unsigned int sensor1max, sensor2max, sensor3max, sensor4max;
  unsigned int sensor1alert, sensor2alert, sensor3alert, sensor4alert, sensor5alert, sensor6alert;
  unsigned int sensor1severe, sensor2severe, sensor3severe, sensor4severe, sensor5severe, sensor6severe; 
  String sensor1units, sensor2units;
  //read config file for next page
  String pagetype = searchFile("pagetype");
  Serial.println(pagetype);
  if (pagetype.indexOf("twobar") >= 0){//2 sensors displayed in 2 bar charts
    alerting = false;
    peaksensor1 = 0;
    peaksensor2 = 0;
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
    tft.setCursor(107,0);
    tft.println("|");
    tft.setCursor(107,70);
    tft.println("|");
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
    while ( (digitalRead(buttonApin) == LOW) && (alerting == false) ){  
      //alerting = monitorSensors();

      Serial.println(buttonApin);
      val1 = getSensorReading(sensor1, sensor1pin);
      val2 = getSensorReading(sensor2, sensor2pin);
      //write value
      tft.setCursor(90,0); //blank old value first
      tft.setTextColor(background);
      tft.println(valOld1);
      tft.setCursor(90,0);
      tft.setTextColor(textdefault);
      tft.println(val1);
      tft.setCursor(90,70); //blank old value first
      tft.setTextColor(background);
      tft.println(valOld2);
      tft.setCursor(90,70);
      tft.setTextColor(textdefault);
      tft.println(val2);
      if (peaksensor1 < val1){
        tft.setTextColor(background);
        tft.setCursor(115,0);
        tft.println(peaksensor1);
        tft.setTextColor(textdefault);
        peaksensor1 = val1;
        tft.setCursor(115,0);
        tft.println(peaksensor1);
      }
      if (peaksensor2 < val2){
        tft.setTextColor(background);
        tft.setCursor(115,70);
        tft.println(peaksensor2);
        tft.setTextColor(textdefault);
        peaksensor2 = val2;
        tft.setCursor(115,70);
        tft.println(peaksensor2);
      }
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
        tft.fillRect( ( ( (float)LCDx/sensor1max)*valOld1 - 1), 20, ( ((float)LCDx/sensor1max)*(val1-valOld1) + 1), 30, barColor1  );
      }
      if (val1 < valOld1){//if the bar is shorter...erase
        tft.fillRect( ( (float)LCDx/sensor1max*val1), 20, ( (float)LCDx/sensor1max*valOld1), 30, background );
      }
      valOld1 = val1;      
      //draw bar2
      if (val2 >= valOld2){//if the bar is longer...add
        tft.fillRect( ( ( (float)LCDx/sensor2max)*valOld2 - 1), 90, ( ((float)LCDx/sensor2max)*(val2-valOld2) + 1), 30, barColor2  );
      }
      if (val2 < valOld2){//if the bar is shorter...erase
        tft.fillRect( ( (float)LCDx/sensor2max*val2), 90, ( (float)LCDx/sensor2max*valOld2), 30, background );
      }
      valOld2 = val2;
    }
  }


  else if (pagetype.indexOf("onebar") >=0){//1 sensor 1 bar chart...bigger fonts
    alerting = false;
    Serial.println("onebar");
    peaksensor1 = 0;
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
    tft.setCursor(110,100);
    tft.println(sensor1units);
    tft.setCursor(45,100);
    tft.println("|");
    long val;
    long valOld = 0;
    uint16_t barColor;
    //loop to show the display and check for the button press
    while  ( (digitalRead(buttonApin) == LOW) && (alerting == false) ){ 
      //alerting = monitorSensors();
      //get value
      val = getSensorReading(sensor1, sensor1pin);
      //write value
      tft.setCursor(0,100); //blank old value first
      tft.setTextColor(background);
      tft.println(valOld);
      tft.setCursor(0,100);
      tft.setTextColor(textdefault);
      tft.println(val);
      if (peaksensor1 < val){
        tft.setTextColor(background);
        tft.setCursor(60,100);
        tft.println(peaksensor1);
        tft.setTextColor(textdefault);
        peaksensor1 = val;
        tft.setCursor(60,100);
        tft.println(peaksensor1);
      }
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
        tft.fillRect( ( ( (float)LCDx/sensor1max)*valOld - 1), 40, ( ((float)LCDx/sensor1max)*(val-valOld) + 1), 40, barColor  );
      }
      if (val < valOld){//if the bar is shorter...erase
        tft.fillRect( ( (float)LCDx/sensor1max*val), 40, ( (float)LCDx/sensor1max*valOld), 40, background );
      }
      valOld = val;
    } 

  }

  else if (pagetype.indexOf("round") >= 0){
    alerting = false;
    peaksensor1 = 0;
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
    while  ((digitalRead(buttonApin) == LOW) && (alerting == false)){
      //alerting = monitorSensors();

      val = getSensorReading(sensor1, sensor1pin);
      tft.setTextSize(2);
      tft.setCursor(64,60);
      tft.setTextColor(background);
      tft.println(valOld);
      tft.setTextColor(textdefault);
      tft.setCursor(64,60);
      tft.println(val);
      if (peaksensor1 < val){
        tft.setTextSize(1);
        tft.setTextColor(background);
        tft.setCursor(64,78);
        tft.println(peaksensor1);
        tft.setTextColor(textdefault);
        peaksensor1 = val;
        tft.setCursor(64,78);
        tft.println(peaksensor1);
      }

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


  else if (pagetype.indexOf("accel") >= 0){//cross bar type chart for accelerometer
    alerting = false;
    sensor1 = searchFile("sensor1");
    sensor1pin = searchFile("sensor1pin").toInt();    
    sensor2 = searchFile("sensor2");
    sensor2pin = searchFile("sensor2pin").toInt();
    //long xval, yval;
    long xval = 0;
    long yval = 0;
    long xvalold = 0;
    long yvalold = 0;
    long xpeak = 0; 
    long ypeak = 0;
    tft.fillScreen(background);
    tft.setTextSize(1);
    /*tft.drawRect(60,0,40,LCDy,outline);
     tft.drawRect(0,44,LCDx,40,outline); 
     tft.drawRect(60,44,40,40,background);//empty the center line
     */
    tft.setTextColor(outline);
    tft.setCursor(0,0);
    tft.println("x:");
    tft.setCursor(110,0);
    tft.println("y:");
    tft.setCursor(0,120);
    tft.println("px:");
    tft.setCursor(110,120);
    tft.println("py:"); 

    while ( (digitalRead(buttonApin) == LOW) && (alerting == false) ){
      //alerting = monitorSensors();

      /*xval = getSensorReading(sensor1, sensor1pin);
       yval = getSensorReading(sensor2, sensor2pin);
       */
      xval = (xval *6+getSensorReading(sensor1, sensor1pin))/7;
      yval = (yval *6+getSensorReading(sensor2, sensor2pin))/7;

      tft.setTextColor(background);
      tft.setCursor(10, 0);
      tft.println((float)xvalold/100);
      tft.setTextColor(textdefault);
      tft.setCursor(10, 0);
      tft.println((float)xval/100);
      tft.setTextColor(background);
      tft.setCursor(120, 0);
      tft.println((float)yvalold/100);
      tft.setTextColor(textdefault);
      tft.setCursor(120, 0);
      tft.println((float)yval/100);
      if (abs(xpeak) <= abs(xval)){
        tft.setTextColor(background);
        tft.setCursor(14,120);
        tft.println((float)xpeak/100);
        tft.setTextColor(textdefault);
        xpeak = xval;
        tft.setCursor(14,120);
        tft.println((float)xpeak/100);
      }    
      if (abs(ypeak) <= abs(yval)){
        tft.setTextColor(background);
        tft.setCursor(124,120);
        tft.println((float)ypeak/100);
        tft.setTextColor(textdefault);
        ypeak = yval;
        tft.setCursor(124,120);
        tft.println((float)ypeak/100);
      }

      /*tft.fillCircle(80-(float)80/120*xvalold,64,5,background); //x old ball
       tft.fillCircle(80-(float)80/120*xval,64,5,fill); //x ball
       tft.fillCircle(80,64+(float)64/120*yvalold,5,background); //y old ball      
       tft.fillCircle(80,64+(float)64/120*yval,5,fill); //y ball
       */

      //1 ball...uniball...oneballin...
      tft.fillCircle(80-(float)80/120*xvalold,64+(float)64/120*yvalold,10,background);//oldball
      tft.fillCircle(80-(float)80/120*xval,64+(float)64/120*yval,10,fill);//newball

        xvalold = xval;
      yvalold = yval;
    }
  }

  else if (pagetype.indexOf("logging") >= 0){//up to 4 sensors shown...log everything to file
    bool logging = true;
    alerting = false;
    int ct = 0;
    while ((digitalRead(buttonApin) == HIGH)){
    }
    while ((digitalRead(buttonApin) == LOW) && (ct <= 100)){
      ct++;
      delay(10);
    }
    if (ct <= 100){
      logging = false;
    }
    peaksensor1=0;
    peaksensor2=0;
    peaksensor3=0;
    peaksensor4=0;
    peaksensor5=0;
    peaksensor6=0;
    //close the settings file
    config.close();
    //draw a blank logging page here:
    tft.fillScreen(background);
    tft.setTextColor(textdefault);
    tft.setTextSize(2);
    tft.setCursor(40, 5);
    if (logging == true){
      tft.println("Logging");
    }
    if (logging == false){
      tft.println("Sensors");
    }
    tft.setTextSize(1);
    tft.setCursor(10, 40); 
    tft.println(sensor1monitor);
    tft.setCursor(10, 50); 
    tft.println(sensor2monitor);
    tft.setCursor(10, 60); 
    tft.println(sensor3monitor);
    tft.setCursor(10, 70); 
    tft.println(sensor4monitor);
    tft.setCursor(10, 80);
    tft.println(sensor5monitor);
    tft.setCursor(10, 90);
    tft.println(sensor6monitor);

    //open a new logging file
    long v1, v2, v3, v4, v5, v6;
    long v1o = 0; 
    long v2o = 0; 
    long v3o = 0; 
    long v4o = 0;
    long v5o = 0;
    long v6o = 0;

    while  (digitalRead(buttonApin) == LOW){ 
      v1 = getSensorReading(sensor1monitor, monitorpin1);
      v2 = getSensorReading(sensor2monitor, monitorpin2);
      v3 = getSensorReading(sensor3monitor, monitorpin3);
      v4 = getSensorReading(sensor4monitor, monitorpin4);
      v5 = getSensorReading(sensor5monitor, monitorpin5);
      v6 = getSensorReading(sensor6monitor, monitorpin6);

      //destory old stuff
      tft.setTextColor(background);
      tft.setCursor(100,40); 
      tft.println(v1o);
      tft.setCursor(100,50); 
      tft.println(v2o);
      tft.setCursor(100,60); 
      tft.println(v3o);
      tft.setCursor(100,70); 
      tft.println(v4o);
      tft.setCursor(100,80); 
      tft.println(v5o);
      tft.setCursor(100,90); 
      tft.println(v6o);

      //test for alert and severe levels
      //and print new values

      if (v1 >= monitorsevere1){
        tft.setTextColor(severe);
      }
      else if (v1 >= monitoralert1){
        tft.setTextColor(alert);
      }
      else {
        tft.setTextColor(outline);
      }
      tft.setCursor(100,40); 
      tft.println(v1);
      if (v2 >= monitorsevere2){
        tft.setTextColor(severe);
      }
      else if (v2 >= monitoralert2){
        tft.setTextColor(alert);
      }
      else {
        tft.setTextColor(outline);
      }
      tft.setCursor(100,50); 
      tft.println(v2);
      if (v3 >= monitorsevere3){
        tft.setTextColor(severe);
      }
      else if (v3 >= monitoralert3){
        tft.setTextColor(alert);
      }
      else {
        tft.setTextColor(outline);
      }
      tft.setCursor(100,60); 
      tft.println(v3);
      if (v4 >= monitorsevere4){
        tft.setTextColor(severe);
      }
      else if (v4 >= monitoralert4){
        tft.setTextColor(alert);
      }
      else {
        tft.setTextColor(outline);
      }      
      tft.setCursor(100,70); 
      tft.println(v4);
      if (v5 >= monitorsevere5){
        tft.setTextColor(severe);
      }
      else if (v5 >= monitoralert5){
        tft.setTextColor(alert);
      }
      else {
        tft.setTextColor(outline);
      }
      tft.setCursor(100,80); 
      tft.println(v5);
      if (v6 >= monitorsevere6){
        tft.setTextColor(severe);
      }
      else if (v6 >= monitoralert6){
        tft.setTextColor(alert);
      }
      else {
        tft.setTextColor(outline);
      }
      tft.setCursor(100,90); 
      tft.println(v6);

      if (peaksensor1 < v1){
        tft.setTextColor(background);
        tft.setCursor(125,40);
        tft.println(peaksensor1);
        tft.setTextColor(textdefault);
        peaksensor1 = v1;
        tft.setCursor(125,40);
        tft.println(peaksensor1);
      }
      if (peaksensor2 < v2){
        tft.setTextColor(background);
        tft.setCursor(125,50);
        tft.println(peaksensor2);
        tft.setTextColor(textdefault);
        peaksensor2 = v2;
        tft.setCursor(125,50);
        tft.println(peaksensor2);
      }      
      if (peaksensor3 < v3){
        tft.setTextColor(background);
        tft.setCursor(125,60);
        tft.println(peaksensor3);
        tft.setTextColor(textdefault);
        peaksensor3 = v3;
        tft.setCursor(125,60);
        tft.println(peaksensor3);
      }      
      if (peaksensor4 < v4){
        tft.setTextColor(background);
        tft.setCursor(125,70);
        tft.println(peaksensor4);
        tft.setTextColor(textdefault);
        peaksensor4 = v4;
        tft.setCursor(125,70);
        tft.println(peaksensor4);
      }      
      if (peaksensor5 < v5){
        tft.setTextColor(background);
        tft.setCursor(125,80);
        tft.println(peaksensor5);
        tft.setTextColor(textdefault);
        peaksensor5 = v5;
        tft.setCursor(125,80);
        tft.println(peaksensor5);
      }      
      if (peaksensor6 < v6){
        tft.setTextColor(background);
        tft.setCursor(125,90);
        tft.println(peaksensor6);
        tft.setTextColor(textdefault);
        peaksensor6 = v6;
        tft.setCursor(125,90);
        tft.println(peaksensor6);
      }
      v1o = v1;
      v2o = v2;
      v3o = v3;
      v4o = v4;
      v5o = v5;
      v6o = v6;
      if (logging == true){
        //log stuff here
      }
    }
    //close file
    //reopen the config file
    config = SD.open("gauges");    
  }

}

//called regularly to see if anything went severe...if so jump to the logging page
boolean monitorSensors(){
  a++;
  if ((a == 1) && (getSensorReading(sensor1monitor, monitorpin1) >= monitorsevere1)){
    tft.setTextSize(3);
    tft.fillScreen(severe);
    tft.setCursor(0, 20);
    tft.println("WARNING!");
    delay(1000);
    //go to the logging page
    searchFile("allgauges");
    return (true);
  }
  else if ((a == monitorinterval) && (getSensorReading(sensor2monitor, monitorpin2) >= monitorsevere2)){
    tft.setTextSize(3);
    tft.fillScreen(severe);
    tft.setCursor(0, 20);
    tft.println("WARNING!");
    delay(1000);
    //go to the logging page
    searchFile("allgauges");
    return (true);
  }
  else if ((a == monitorinterval*2) && (getSensorReading(sensor3monitor, monitorpin3) >= monitorsevere3)){
    tft.setTextSize(3);
    tft.fillScreen(severe);
    tft.setCursor(0, 20);
    tft.println("WARNING!");
    delay(1000);
    //go to the logging page
    searchFile("allgauges");
    return (true);
  }
  else if ((a == monitorinterval*3) && (getSensorReading(sensor4monitor, monitorpin4) >= monitorsevere4)){
    tft.setTextSize(3);
    tft.fillScreen(severe);
    tft.setCursor(0, 20);
    tft.println("WARNING!");
    delay(1000);
    //go to the logging page
    searchFile("allgauges");
    return (true);
  }
  else if ((a == monitorinterval*4) && (getSensorReading(sensor5monitor, monitorpin5) >= monitorsevere5)){
    tft.setTextSize(3);
    tft.fillScreen(severe);
    tft.setCursor(0, 20);
    tft.println("WARNING!");
    delay(1000);
    //go to the logging page
    searchFile("allgauges");
    return (true);
  }
  else if ((a == monitorinterval*5) && (getSensorReading(sensor6monitor, monitorpin6) >= monitorsevere6)){
    tft.setTextSize(3);
    tft.fillScreen(severe);
    tft.setCursor(0, 20);
    tft.println("WARNING!");
    delay(1000);
    //go to the logging page
    searchFile("allgauges");
    return (true);
  }
  if (a >= monitorinterval*5){
    a = 0;
    return false;
  }
  else return false;
  //return false;  //for testing you could just return false and not worry about checking everything
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
  //Serial1.flush();
  long int value = 0;
  if (whichSensor.indexOf("obdspeedkph") >= 0){
    Serial1.println("010D"); //mode 1 0D PID
    //getResponse();  //command echoed
    getResponse2();  //value
    value = strtol(&rxData[6],0,16) ; //convert the string to integer
  }
  if (whichSensor.indexOf("obdspeedmph") >= 0){ //works,  but has bleedover issues
    Serial1.println("010D"); //mode 1 0D PID
    //getResponse();
    getResponse2();  //value
    value = strtol(&rxData[6],0,16)/1.6 ; //convert the string to integer
  }  
  if (whichSensor.indexOf("obdrpms") >= 0){ //working
    Serial1.println("010C"); //mode 1 0C PID (rpm)
    //Serial.println("getting rpm");
    //getResponse();  //value
    getResponse2();
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/4; //aka ((A*256)+B)/4 
    //Serial.println("rpm =");
    //Serial.println(value);
  }
  if (whichSensor.indexOf("obdcoolantc") >=0){
    Serial.println("coolantc");
    Serial1.print("O105\r");   
    getResponse2();
    //Serial.println(&rxData[6]);
    //Serial.println((float)strtol(&rxData[6],0,16));
    value = (strtol(&rxData[6],0,16))-40; //aka A-40
    Serial.print("value of coolantc ");
    Serial.print(value);
  }
  if (whichSensor.indexOf("obdcoolantf") >=0){ //not working...05 or 67 both return junk
    Serial.println("coolantf");
    Serial1.print("O105\r"); //gives raw data of "?"
    getResponse2();
    //Serial1.flush();
    //Serial.println(&rxData[6]);
    //Serial.println((float)strtol(&rxData[6],0,16));
    value = ((strtol(&rxData[6],0,16))-40)*1.8+32; //aka A-40
    Serial.print("value of coolantf ");
    Serial.print(value);
  }
  if (whichSensor.indexOf("obdboost") >= 0){
    Serial1.println("0265"); //intake manifold abs pressure ... doesn't work with brz
    getResponse2();
    value = (strtol(&rxData[6],0,16)); //aka A
  }  
  if (whichSensor.indexOf("obdiat") >= 0){ //not supported on the BRZ ecu
    Serial1.println("O1OF");
    getResponse2();
    value = (strtol(&rxData[6],0,16)-40); //aka A-40
  }
  if (whichSensor.indexOf("obdmaf") >= 0){//I think this works
    Serial1.println("0110");
    getResponse2();
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/100; //aka ((A*256)+B)/100
  }
  if (whichSensor.indexOf("obdvolts") >= 0){ //works perfectly
    Serial1.println("0142");
    //Serial.println("getting volts");
    //getResponse();
    getResponse2();
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/1000; //aka ((A*256)+B)/100
    //Serial.println("volts is:");
    //Serial.println(value);
  }
  if (whichSensor.indexOf("obdoiltempc") >= 0){
    Serial1.println("015C");
    getResponse2();
    value = strtol(&rxData[6],0,16)-40; //aka (A-40)
  }
  if (whichSensor.indexOf("obdoiltempf") >= 0){
    Serial1.println("015C");
    getResponse2();
    value = (strtol(&rxData[6],0,16)-40)*1.8+32; //aka (A-40) *1.8
  }

  //nonstandard/experiemental PIDs
  if (whichSensor.indexOf("obdbrzoiltempc") >= 0){
   // Serial1.println("AT SH 7E0");
    //Serial1.flush();
    Serial1.println("2101");  
    getResponse2();  
    value = ((float)strtol(&rxData[109],0,16) - 40); //29th byte - 40 (?)
    //Serial1.println("AT D");
    //Serial1.println("AT E0");  
    //delay(40);
    //Serial1.println("ATSP6");
  }
  if (whichSensor.indexOf("obdbrzoiltempf") >= 0){ //works
    //Serial1.println("AT SH 7E0");
    //Serial1.flush();
    //getResponse2();
    Serial1.println("2101");
    getResponse2();
    Serial.println("brz oil temp");
    //delay(40);
    //Serial.println(&rxData[109]);
    //Serial.println((float)strtol(&rxData[109],0,16));
    value = ((float)strtol(&rxData[109],0,16) - 40) * 1.8 + 32;
    Serial.println(value);
    //Serial1.println("AT D"); //these two lines make the next 2101 reading get cut off
    //Serial1.println("AT E0");
    //Serial1.flush();
    //Serial1.println("ATSP6");//doesn't work and is slow (probing for protocol takes work)
    //getResponse();
    //Serial1.flush();
  }
  /*if (whichSensor.indexOf("obdbrzfuelleft") >= 0){ //not working
    Serial.println("brz fuel left");
    Serial1.println("AT SH 7C0");
    //getResponse();
    getResponse();
    Serial1.println("2129");    
    value = ((float)strtol(&rxData[6],0,16)*13.2)/100; // (A*13.2)/100
    Serial.println(value);
    delay(40);
    Serial1.println("ATSP6");
    //getResponse();
    //Serial1.println("AT D");
    //Serial1.println("AT E0");
  }*/
  delay(100);  
  Serial1.flush();
  memset(rxData,0,sizeof(rxData)); //blank the array
  return value; 
}

int getSensorReading(String sensorName, int pinNumber){
  //if the pin number is 0 it is obd II...look it up with the obd II lib
  if (pinNumber == 0){
    Serial.println("getting OBDII value");
    return(getOBDIIvalue(sensorName));  
  }
  //else call the appropriate Analog to digital conversion function on the appropirate pin
  else {
    Serial.println("fake sensor");
    if (sensorName.indexOf("fakeaccel") >= 0){
      return random(-1500,1500);    
    }
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


//from:  https://forum.sparkfun.com/viewtopic.php?f=14&t=32457&start=60 and https://forum.sparkfun.com/viewtopic.php?f=14&t=38253
void getResponse2(void){
  char c;
   int start=millis();
  //If nothing is currently available do nothing and break after 3 seconds
  while(Serial1.available()==0){if(millis()-start>3000){break;}}
  do {
    if (Serial1.available() > 0)
    {
      c = Serial1.read();
      if ((c != '>') && (c != '\r') && (c != '\n')) //Keep these out of our buffer
      {
        rxData[rxIndex++] = c; //Add whatever we receive to the buffer
      }
    }
  } 
  while (c != '>'); //The ELM327 ends its response with this char so when we get it we exit out.
  rxData[rxIndex++] = '\0';  //Converts the array into a string
  Serial.print("rxData(in getResponse2): ");
  Serial.println(rxData);
  rxIndex = 0; //Set this to 0 so next time we call the read we get a "clean" buffer
}

void getResponse(void){
  char obdIn=0;
  int i=0;
  int start=millis();
  //If nothing is currently available do nothing and break after 3 seconds
  while(Serial1.available()==0){if(millis()-start>3000){break;}}
  while(Serial1.available()){
    //check to see if end of line/message
    if (Serial1.peek()=='\r'){
      obdIn=Serial1.read();
      rxData[i]='\0';
      Serial.println(rxData);
      i=0;
    }
    // The prompt is sometimes the only thing recieved so this needs to be taken care of
    else if(Serial1.peek()=='>'){
      obdIn=Serial1.read();
      //Serial.write(obdIn);
    }
    // Add next character to string
    else{
      obdIn=Serial1.read();
      rxData[i++]=obdIn;
    }
  }
  Serial.print("rxData(in getResponse): ");
  Serial.println(rxData);
  rxIndex=0;
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
  // Serial.println("lookup_fake_random_sensor");
  // Serial.println(fakeSensor);
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

//from the arudino playground: http://playground.arduino.cc/Main/Smooth by Paul 
int smooth(int data, float filterVal, float smoothedVal){


  if (filterVal > 1){      // check to make sure param's are within range
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }

  smoothedVal = (data * (1 - filterVal)) + (smoothedVal  *  filterVal);

  return (int)smoothedVal;
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









