//obdII gauge that reuses a lot of code from my other projects
//set up to use the SPI 128x32 OLED display from adafruit

//TODO: 
//0 reading for oil temp? (quickly flashes a real value for a sec)
//move oil value left
//can't draw text on value area after
//  reset on any mode

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13

/* Uncomment this block to use hardware SPI
#define OLED_DC     6
#define OLED_CS     7
#define OLED_RESET  8
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);
*/

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

//This is a character buffer that will store the data from the serial port
char rxData[115];
char rxIndex=0;
int buttonV;

String modeList[] = {"obdbrzoiltempf", "obdafr", "obdvolts"};
int warnLevels[] = {212, 220, 15};
int warnSign[] = {1,1,1};  //1 for high, 0 for low (in cases like oil pressure)
int peaks[] = {0,0,0};
int curValue[] = {0,0,0};
int previousReading[] = {0,0,0};
int mode = 0;
int modes = 2;  //actually this means there are 3 modes...0 is the first of the array


       //store other bmps here:
static const unsigned char PROGMEM robothead [] = {
  0b00000000, 0b00111100, 0b00000000, 0b00000000, //           ####           
  0b00000000, 0b01111110, 0b00000000, 0b00000000, //          ######          
  0b00000000, 0b01111110, 0b00000000, 0b00000000, //          ######          
  0b00000000, 0b01111110, 0b00000000, 0b00000000, //          ######          
  0b00000000, 0b00111100, 0b00000000, 0b00000000, //           ####           
  0b00000000, 0b00011000, 0b00000000, 0b00000000, //            ##            
  0b00000000, 0b00011000, 0b00000000, 0b00000000, //            ##            
  0b00000000, 0b00011000, 0b00000000, 0b00000000, //            ##            
  0b00000000, 0b00011000, 0b00000000, 0b00000000, //            ##            
  0b00000000, 0b00011000, 0b00000000, 0b00000000, //            ##            
  0b00000000, 0b00011000, 0b00000000, 0b00000000, //            ##            
  0b00000000, 0b00011000, 0b00000000, 0b00000000, //            ##            
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
  0b11110000, 0b00111100, 0b00001111, 0b10000000, // ####      ####      #####
  0b11110000, 0b00111100, 0b00001111, 0b10000000, // ####      ####      #####
  0b11110000, 0b00111100, 0b00001111, 0b10000000, // ####      ####      #####
  0b11110000, 0b00111100, 0b00001111, 0b10000000, // ####      ####      #####
  0b11110000, 0b00111100, 0b00001111, 0b10000000, // ####      ####      #####
  0b11110000, 0b00111100, 0b00001111, 0b10000000, // ####      ####      #####
  0b11110000, 0b00111100, 0b00001111, 0b10000000, // ####      ####      #####
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
  0b11100000, 0b00000000, 0b00000111, 0b10000000, // ###                  ####
  0b11100000, 0b00000000, 0b00000111, 0b10000000, // ###                  ####
  0b11111111, 0b11111111, 0b11111111, 0b10000000, // #########################
};

static const unsigned char PROGMEM batt [] = {
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000001, 0b11110000, 0b00011111, 0b00000000, //        #####       #####        
  0b00000010, 0b00001000, 0b00100000, 0b10000000, //       #     #     #     #       
  0b00000010, 0b00001000, 0b00100000, 0b10000000, //       #     #     #     #       
  0b00000010, 0b00001000, 0b00100000, 0b10000000, //       #     #     #     #       
  0b00000010, 0b00001000, 0b00100000, 0b10000000, //       #     #     #     #       
  0b00000010, 0b00001000, 0b00100000, 0b10000000, //       #     #     #     #       
  0b00011111, 0b11111111, 0b11111111, 0b11110000, //    #########################    
  0b00011110, 0b00000000, 0b00000000, 0b11110000, //    ####                 ####    
  0b00011110, 0b00000000, 0b00000000, 0b11110000, //    ####                 ####    
  0b00011110, 0b00000000, 0b00000000, 0b11110000, //    ####                 ####    
  0b00010000, 0b01000000, 0b00000000, 0b00010000, //    #     #                 #    
  0b00010000, 0b01000000, 0b00000000, 0b00010000, //    #     #                 #    
  0b00010000, 0b01000000, 0b00000000, 0b00010000, //    #     #                 #    
  0b00010011, 0b11111000, 0b00111111, 0b10010000, //    #  #######     #######  #    
  0b00010000, 0b01000000, 0b00000000, 0b00010000, //    #     #                 #    
  0b00010000, 0b01000000, 0b00000000, 0b00010000, //    #     #                 #    
  0b00010000, 0b01000000, 0b00000000, 0b00010000, //    #     #                 #    
  0b00010000, 0b00000000, 0b00000000, 0b00010000, //    #                       #    
  0b00011110, 0b00000000, 0b00000000, 0b11110000, //    ####                 ####    
  0b00011110, 0b00000000, 0b00000000, 0b11110000, //    ####                 ####    
  0b00011110, 0b00000000, 0b00000000, 0b11110000, //    ####                 ####    
  0b00011111, 0b11111111, 0b11111111, 0b11110000, //    #########################    
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
};

static const unsigned char PROGMEM o2 [] = {
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000001, 0b11111000, 0b00000000, 0b00000000, //        ######                   
  0b00000011, 0b11111110, 0b00000000, 0b00000000, //       #########                 
  0b00000111, 0b00000111, 0b00000000, 0b00000000, //      ###     ###                
  0b00001100, 0b00000011, 0b00000000, 0b00000000, //     ##        ##                
  0b00001100, 0b00000011, 0b00000000, 0b00000000, //     ##        ##                
  0b00011000, 0b00000001, 0b10000000, 0b00000000, //    ##          ##               
  0b00011000, 0b00000001, 0b11111000, 0b00000000, //    ##          ######           
  0b00011000, 0b00000001, 0b11111100, 0b00000000, //    ##          #######          
  0b00011000, 0b00000001, 0b10001110, 0b00000000, //    ##          ##   ###         
  0b00011000, 0b00000001, 0b10000110, 0b00000000, //    ##          ##    ##         
  0b00001100, 0b00000011, 0b00000110, 0b00000000, //     ##        ##     ##         
  0b00001100, 0b00000011, 0b00000110, 0b00000000, //     ##        ##     ##         
  0b00001110, 0b00001110, 0b00000100, 0b00000000, //     ###     ###      #          
  0b00000111, 0b11111100, 0b00001100, 0b00000000, //      #########      ##          
  0b00000001, 0b11111000, 0b00011000, 0b00000000, //        ######      ##           
  0b00000000, 0b00000000, 0b00110000, 0b00000000, //                   ##            
  0b00000000, 0b00000000, 0b01100000, 0b00000000, //                  ##             
  0b00000000, 0b00000000, 0b11000000, 0b00000000, //                 ##              
  0b00000000, 0b00000001, 0b10000000, 0b00000000, //                ##               
  0b00000000, 0b00000011, 0b11111111, 0b00000000, //               ##########        
  0b00000000, 0b00000011, 0b11111111, 0b00000000, //               ##########        
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //  
};


static const unsigned char PROGMEM oil [] = {
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b01000000, 0b00001110, 0b00000000, 0b00000000, //  #          ###                 
  0b01100000, 0b00011111, 0b00000000, 0b11100000, //  ##        #####        ###     
  0b01111000, 0b00001110, 0b00000011, 0b10000000, //  ####       ###       ###       
  0b01111100, 0b00000100, 0b00001101, 0b00000000, //  #####       #      ## #        
  0b01111111, 0b11111111, 0b11111110, 0b01000000, //  ######################  #      
  0b00010111, 0b11111111, 0b11111110, 0b00010000, //    # ##################    #    
  0b00001111, 0b11111111, 0b11111100, 0b00010000, //     ##################     #    
  0b00001111, 0b11111111, 0b11111100, 0b00011000, //     ##################     ##   
  0b00000111, 0b11111111, 0b11111000, 0b00111100, //      ################     ####  
  0b00000111, 0b11111111, 0b11110000, 0b00111100, //      ###############      ####  
  0b00000111, 0b11111111, 0b11110000, 0b00111100, //      ###############      ####  
  0b00000111, 0b11111111, 0b11100000, 0b00011000, //      ##############        ##   
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                
};


void setup() {
 pinMode(A0, INPUT_PULLUP);
 //3 buttons connected as such: http://tronixstuff.com/2011/01/11/tutorial-using-analog-input-for-multiple-buttons/
 //use 15 Kohm resistors
 //no buttons is 1010
 //button nearest gnd is 300
 //middle button is 454
 //last button is 555
 Serial.begin(9600);
 display.begin(SSD1306_SWITCHCAPVCC);
 display.display();
 display.clearDisplay();

 //show flash screen
 display.drawBitmap(0, 0, robothead, 32, 32, 1);
 display.display();

 display.setCursor(36,0);
 display.setTextSize(2);
 display.setTextColor(WHITE);
 display.println("Not A");
 display.setCursor(36,15);
 display.println("Clock");
 display.display();

 //shifty eyes
/* for (int i; i < 8; i++){
   display.fillRect(8, 22, 2, 2, WHITE);
   display.fillRect(18, 22, 2, 2, WHITE);
   display.display();
   delay(500);
   display.fillRect(8, 22, 2, 2, BLACK);
   display.fillRect(18, 22, 2, 2, BLACK); 
   display.display();
   display.fillRect(4, 22, 2, 2, WHITE);
   display.fillRect(14, 22, 2, 2, WHITE);
   display.display();
   delay(500);
   display.fillRect(4, 22, 2, 2, BLACK);
   display.fillRect(14, 22, 2, 2, BLACK); 
   display.display();
 }
*/
  //temporary stuff to test button setup
/*  
  display.clearDisplay();
  display.display();
  int tempVal;
  while(true){
    tempVal = analogRead(0);
    display.setCursor(36,0);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.println(tempVal);
    display.display();
    delay(100);
    display.setCursor(36,0);
    display.setTextSize(2);
    display.setTextColor(BLACK);
    display.println(tempVal);
    display.display();
  } 
*/
  //end temp stuff for testing buttons

 //set up OBD II stuffs
   display.fillRect(8, 22, 2, 2, WHITE);
   display.fillRect(18, 22, 2, 2, WHITE);
   display.display();
 Serial.println("ATZ");
 getResponse();
 delay(1000);
   display.fillRect(8, 22, 2, 2, BLACK);
   display.fillRect(18, 22, 2, 2, BLACK); 
   display.display();
   display.fillRect(4, 22, 2, 2, WHITE);
   display.fillRect(14, 22, 2, 2, WHITE);
   display.display();
 Serial.println("ATDP");
 getResponse();
 delay(1000);
   display.fillRect(8, 22, 2, 2, WHITE);
   display.fillRect(18, 22, 2, 2, WHITE);
   display.display();
 Serial.println("ATE0");
 getResponse();
 delay(1000);
   display.fillRect(8, 22, 2, 2, BLACK);
   display.fillRect(18, 22, 2, 2, BLACK); 
   display.display();
   display.fillRect(4, 22, 2, 2, WHITE);
   display.fillRect(14, 22, 2, 2, WHITE);
   display.display();
 Serial.println("AT SH 7E0");  
 getResponse();
 delay(1000);
 Serial.flush();
 display.clearDisplay();
 display.display();

 //put first mode icon and unit here
  display.drawBitmap(0, 0, oil, 32, 32, 1);
  display.setCursor(110,12);
  display.println("F"); //dunno if the deg symbol will work
  display.display();
}

void loop() {
  buttonV = analogRead(A0);
  while ( (analogRead(A0) <= 430) || (analogRead(A0) >= 490) ){ //not mode button
    buttonV = analogRead(A0);
    if ( (buttonV <= 500) && (buttonV >= 600) ){ //hold down the peaks button to show the peaks of this mode (545)
      while ( (buttonV <= 500) && (buttonV >= 600) ){ //debounce
        //display peaks for this "mode" here
        display.fillRect(48,0,52,32,BLACK); //black area between icon and unit
        display.setTextSize(3);
        display.setTextColor(WHITE);
        display.setCursor(48,10);
        display.println(peaks[mode]);
        display.display();
        buttonV = analogRead(A0);
        delay(50);
      } 
      display.fillRect(50,0,54,32,BLACK); //black area between icon and unit
      display.display();
    }

    //hold reset button here to reset peak of specific mode (300)
    if ( (buttonV <= 325) && (buttonV >= 285) ){ //hold down the reset button to reseet the peaks of this mode (3??)
      while ( (buttonV <= 325) && (buttonV >= 285) ){ //debounce
        display.fillRect(48,0,52,32,BLACK); //black area between icon and unit
        display.setTextSize(3);
        display.setCursor(48,10);
        display.setTextColor(WHITE);
        display.println("RST");
        display.display();
        buttonV = analogRead(A0);
        delay(50);
      } 
      //reset peak specific to this mode
      display.fillRect(48,0,52,32,BLACK); //black area between icon and unit
      display.display();
      peaks[mode] = 0;
      buttonV = analogRead(A0);
    }

    curValue[mode] = getOBDIIvalue(modeList[mode]);
    if (curValue[mode] > peaks[mode]){
      peaks[mode] = curValue[mode];
    }

    //display the value here...no conditionals...just print the value (for now??)
    //only print if it changes to avoid flickering
    if ( abs(curValue[mode]-previousReading[mode]) > 0 ){
     updateVal();
    }
    //delay(100);
    //check for warning values here (for this mode only)
    if ( ( (curValue[mode] > warnLevels[mode]) && (warnSign[mode] == 1) ) || ( (curValue[mode] < warnLevels[mode]) && (warnSign[mode] == 0) ) ){
      //do a warning thing here
      warn();
    }
  }
  while ( (analogRead(A0) >= 430) && (analogRead(A0) <= 490) ){ //mode button
  }
  if (mode == modes){
      mode = 0;
  }
  else {
    mode++;
  }
    //upon switching modes blank screen and display that mode's icon
    //also print the unit of measurement if used
    display.clearDisplay();
    display.setTextSize(3);
    if (mode == 0){//oil temp
      display.drawBitmap(0, 0, oil, 32, 32, 1);
      display.setCursor(110,12);
      display.println("F"); //dunno if the deg symbol will work
      display.display();
    }
    if (mode == 1){//AFR
      display.drawBitmap(0, 0, o2, 32, 32, 1);
      display.display();
    }
    if (mode == 2){//Volts
      display.drawBitmap(0, 0, batt, 32, 32, 1);
      display.setCursor(110,12);
      display.println("V"); //dunno if the deg symbol will work
      display.display();
    }
}

void warn(){
  //a border around the screen flashes as a warning
  for (int a=1;a<=3;a++){
    display.drawRect(0,0,128,32,WHITE);
    delay(100);
    display.drawRect(0,0,128,32,BLACK);
  }
  return;
} 

void updateVal(){
  display.setTextSize(3);
  //draw old value
  display.setTextColor(BLACK);
  display.setCursor(60,12);
  display.println(previousReading[mode]);
  display.display();
  //draw new value
  display.setTextColor(WHITE);
  display.setCursor(60,12);
  display.println(curValue[mode]);
  display.display();
  previousReading[mode] = curValue[mode];  
  return;
} 

//from: https://forum.sparkfun.com/viewtopic.php?f=14&t=38253
void getResponse(void){
  char c;
  int start=millis();
  //If nothing is currently available do nothing and break after 3 seconds
  while(Serial.available()==0){if(millis()-start>3000){break;}}
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
  rxIndex = 0; //Set this to 0 so next time we call the read we get a "clean" buffer
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
  //delay(40);
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
  Serial.flush();
  memset(rxData,0,sizeof(rxData)); //blank the array
  return value; 
}
