//obdII gauge that reuses a lot of code from my other projects
//set up to use the SPI 128x32 OLED display from adafruit

//TODO: 
//write a function that periodically scans all monitored values
//test peak recall (fix...isn't really working...wrong voltage range??)
//test warning function (set one low and test that way)

//hardware TODO:
//test tinting plastic to appear red/amber
//desoldier buttons and wire up
//desoldier led backlight
//wire up power to come from OBD II

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ELM327.h>
#define ELM_TIMEOUT 9000
#define ELM_BAUD_RATE 9600
#define ELM_PORT Serial

#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

int buttonV;

Elm327 Elm;
byte Status;

int warnLevels[] = {
  100, 220, 15, 220, 300};
int warnSign[] = {
  1,1,1,1,1};  //1 for high, 0 for low (in cases like oil pressure)
float peaks[] = {
  0,0,0,0,0};
float curValue[] = {
  0,0,0,0,0};
float previousReading[] = {
  0,0,0,0,0};
int mode = 0;
int modes = 4;  //0 is the first of the array

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
  0b00000000, 0b11011111, 0b11100000, 0b00000000, //         ## ########             
  0b00000000, 0b11011111, 0b11100000, 0b00000000, //         ## ########             
  0b00000000, 0b00011100, 0b00000000, 0b00000000, //            ###                  
  0b00000000, 0b00011100, 0b00000000, 0b00000000, //            ###                  
  0b00000000, 0b00011100, 0b00000000, 0b00000000, //            ###                  
  0b00000000, 0b00011111, 0b11000000, 0b00000000, //            #######              
  0b00000000, 0b00011111, 0b11000000, 0b00000000, //            #######              
  0b00000000, 0b00011100, 0b00000000, 0b00000000, //            ###                  
  0b00000000, 0b00011100, 0b00000000, 0b00000000, //            ###                  
  0b00000000, 0b00011100, 0b00000000, 0b00000000, //            ###                  
  0b00000000, 0b00011100, 0b00000000, 0b00000000, //            ###                  
  0b00000000, 0b00011100, 0b00000000, 0b00000000, //            ###                                  
};

static const unsigned char PROGMEM coolant [] = {
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00111111, 0b10000000, 0b00000000, 0b00000000, //   #######                       
  0b00111000, 0b00000011, 0b10000000, 0b00000000, //   ###         ###               
  0b00111000, 0b00000011, 0b10000000, 0b00000000, //   ###         ###               
  0b00111000, 0b00000011, 0b11111110, 0b00000000, //   ###         #########         
  0b00111111, 0b00000011, 0b11111110, 0b00000000, //   ######      #########         
  0b00111000, 0b00000011, 0b11111110, 0b00000000, //   ###         #########         
  0b00111000, 0b00000011, 0b10000000, 0b00000000, //   ###         ###               
  0b00111000, 0b00000011, 0b10000000, 0b00000000, //   ###         ###               
  0b00111000, 0b00000011, 0b11111110, 0b00000000, //   ###         #########         
  0b00000000, 0b00000011, 0b11111110, 0b00000000, //               #########         
  0b00000000, 0b00000011, 0b11111110, 0b00000000, //               #########         
  0b00000000, 0b00000011, 0b10000000, 0b00000000, //               ###               
  0b00000000, 0b00000011, 0b10000000, 0b00000000, //               ###               
  0b00000000, 0b00000011, 0b10000000, 0b00000000, //               ###               
  0b00000000, 0b00000011, 0b10000111, 0b11110000, //               ###    #######    
  0b00000111, 0b11110011, 0b10011100, 0b00011100, //      #######  ###  ###     ###  
  0b00011100, 0b00011111, 0b11110000, 0b00000110, //    ###     #########         ## 
  0b01110000, 0b00000111, 0b11100000, 0b00000011, //  ###         ######           ##
  0b11000000, 0b00000011, 0b10000000, 0b00000001, // ##            ###              #
  0b10000000, 0b00000111, 0b11000000, 0b00000000, // #            #####              
  0b00000000, 0b00000111, 0b11000000, 0b00000000, //              #####              
  0b00000000, 0b00000111, 0b11000000, 0b00000000, //              #####              
  0b00000000, 0b00000111, 0b11000000, 0b00000000, //              #####              
  0b00000111, 0b11110011, 0b10000111, 0b11110000, //      #######  ###    #######    
  0b00011100, 0b00011100, 0b00011100, 0b00011100, //    ###     ###     ###     ###  
  0b01110000, 0b00000110, 0b01110000, 0b00000110, //  ###         ##  ###         ## 
  0b11000000, 0b00000011, 0b11000000, 0b00000011, // ##            ####            ##
  0b10000000, 0b00000001, 0b10000000, 0b00000001, // #              ##              #
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                                               
};

static const unsigned char PROGMEM intake [] = {
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00001100, 0b00000000, //                     ##          
  0b00000000, 0b00000000, 0b01111100, 0b00000000, //                  #####          
  0b00000000, 0b00000011, 0b11110100, 0b00000000, //               ###### #          
  0b00000000, 0b00111111, 0b10000100, 0b00000000, //           #######    #          
  0b00000001, 0b11111000, 0b10000100, 0b00000000, //        ######   #    #          
  0b00001111, 0b01001000, 0b10111111, 0b11111111, //     #### #  #   # ##############
  0b00001100, 0b01111111, 0b11000111, 0b11111111, //     ##   #########   ###########
  0b00001111, 0b11001000, 0b10000111, 0b11111111, //     ######  #   #    ###########
  0b00001100, 0b01001011, 0b11111111, 0b10000011, //     ##   #  # ###########     ##
  0b00001111, 0b11111100, 0b10000111, 0b10011111, //     ##########  #    ####  #####
  0b00001100, 0b01001000, 0b10000111, 0b10011111, //     ##   #  #   #    ####  #####
  0b00001111, 0b11111111, 0b11111111, 0b10011111, //     #####################  #####
  0b00001100, 0b01001000, 0b10000111, 0b10000011, //     ##   #  #   #    ####     ##
  0b00001111, 0b11111000, 0b10000111, 0b10011111, //     #########   #    ####  #####
  0b00001100, 0b01001111, 0b11111111, 0b10011111, //     ##   #  #############  #####
  0b00001111, 0b11001000, 0b10000111, 0b10011111, //     ######  #   #    ####  #####
  0b00001100, 0b01111111, 0b11000111, 0b10011111, //     ##   #########   ####  #####
  0b00001111, 0b01001000, 0b10111111, 0b11111111, //     #### #  #   # ##############
  0b00001100, 0b11111000, 0b10000111, 0b11111111, //     ##  #####   #    ###########
  0b00001110, 0b01001111, 0b10000111, 0b11111111, //     ###  #  #####    ###########
  0b00001111, 0b11001000, 0b11110111, 0b11111111, //     ######  #   #### ###########
  0b00000011, 0b11111100, 0b10001111, 0b11111111, //       ########  #   ############
  0b00000000, 0b01111111, 0b10000100, 0b00000000, //          ########    #          
  0b00000000, 0b00001111, 0b11110100, 0b00000000, //             ######## #          
  0b00000000, 0b00000011, 0b11111100, 0b00000000, //               ########          
  0b00000000, 0b00000000, 0b01111100, 0b00000000, //                  #####          
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                 
  0b00000000, 0b00000000, 0b00000000, 0b00000000, //                                                                 
};

void setup() {
  pinMode(A0, INPUT_PULLUP);
  Status=Elm.begin();
  //3 buttons connected as such: http://tronixstuff.com/2011/01/11/tutorial-using-analog-input-for-multiple-buttons/
  //use 15 Kohm resistors
  //no buttons is 1010
  //button nearest gnd is 300
  //middle button is 454
  //last button is 555
  //Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display();
  display.clearDisplay();
  display.display();

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
   //no button 1013-1017
   //a button 15 (no resistor...just wire resistance)
   //b button 302
   //c button 457
   //d button 556
   } */

  //end temp stuff for testing buttons

  //set up OBD II stuffs
  display.fillRect(8, 21, 2, 2, WHITE);
  display.fillRect(18, 21, 2, 2, WHITE);
  display.display();
  getOBDIIvalue("obdbrzoiltempf"); //do the first reading here because it takes a sec or two  display.fillRect(8, 21, 2, 2, BLACK);
  display.fillRect(8, 21, 2, 2, BLACK);
  display.fillRect(18, 21, 2, 2, BLACK); 
  display.display();
  display.fillRect(4, 21, 2, 2, WHITE);
  display.fillRect(14, 21, 2, 2, WHITE);
  display.display();
  delay(1000);
  display.fillRect(4, 21, 2, 2, BLACK);
  display.fillRect(14, 21, 2, 2, BLACK);
  display.display();
  display.fillRect(8, 22, 2, 2, WHITE);
  display.fillRect(18, 22, 2, 2, WHITE); 
  display.display();
  delay(1000);
  display.fillRect(8, 22, 2, 2, BLACK);
  display.fillRect(18, 22, 2, 2, BLACK);
  display.display();
  display.fillRect(4, 21, 2, 2, WHITE);
  display.fillRect(14, 21, 2, 2, WHITE);
  display.display();
  delay(500);

  //put first mode icon and unit here
  display.clearDisplay();
  display.drawBitmap(0, 0, oil, 32, 32, 1);
  display.display();
}

void loop() {
  buttonV = analogRead(A0);
  while ( (analogRead(A0) <= 430) || (analogRead(A0) >= 490) ){ //not mode button (not between 430 and 490)
    buttonV = analogRead(A0);
    if ( (buttonV >= 500) && (buttonV <= 600) ){ //hold down the peaks button to show the peaks of this mode (555ish)
      while ( (buttonV >= 500) && (buttonV <= 600) ){ //debounce
        //display peaks for this "mode" here
        display.fillRect(48,0,80,32,BLACK); //black area between icon and unit
        display.setTextSize(3);
        display.setTextColor(WHITE);
        display.setCursor(48,10);
        display.println(peaks[mode]);
        display.display();
        buttonV = analogRead(A0);
        delay(50);
      } 
      display.fillRect(48,0,80,32,BLACK); //black area between icon and unit
      getVal();
      updateVal(); //just incase it doesn't draw below because the values stay the same
      display.display();
      buttonV = analogRead(A0);
    }

    //hold reset button here to reset peak of specific mode
    if ( (buttonV <= 325) && (buttonV >= 285) ){ //hold down the reset button to reseet the peaks of this mode (300ish)
      while ( (buttonV <= 325) && (buttonV >= 285) ){ //debounce
        display.fillRect(48,0,80,32,BLACK); //black area between icon and unit
        display.setTextSize(3);
        display.setCursor(48,10);
        display.setTextColor(WHITE);
        display.println("RST");
        display.display();
        buttonV = analogRead(A0);
        delay(50);
      } 
      //reset peak specific to this mode
      display.fillRect(48,0,80,32,BLACK); //black area between icon and unit
      getVal();
      updateVal(); //just incase it doesn't draw below because the values stay the same
      display.display();
      peaks[mode] = 0;
      buttonV = analogRead(A0);
    }

    //display the value here...no conditionals...just print the value
    //only print if it changes to avoid flickering
    getVal();
    if ( abs(curValue[mode]-previousReading[mode]) > 0 ){
      updateVal();
    }

    //TODO: fix this...not working
    //check for warning values here (for this mode only)
    //if ( ( (curValue[mode] > warnLevels[mode]) && (warnSign[mode] == 1) ) || ( (curValue[mode] < warnLevels[mode]) && (warnSign[mode] == 0) ) ){
    if (curValue[mode] >= warnLevels[mode]){
      //do a warning thing here
      display.println("WARN");
      display.display();
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
  display.setTextColor(WHITE);
  if (mode == 0){//oil temp
    display.drawBitmap(0, 0, oil, 32, 32, 1);
    display.display();
  }
  if (mode == 1){//AFR
    display.drawBitmap(0, 0, o2, 32, 32, 1);
    display.display();
  }
  if (mode == 2){//Volts
    display.drawBitmap(0, 0, batt, 32, 32, 1);
    display.display();
  }
  if (mode == 3){//Coolant
    display.drawBitmap(0, 0, coolant, 32, 32, 1);
    display.display();
  }
  if (mode == 4){
    display.drawBitmap(0, 0, intake, 32, 32, 1);
    display.display();
  }
  getVal();
  updateVal(); //if you don't update here and the value hasn't changed you get blank value
}

void warn(){
  //a border around the screen flashes as a warning
  for (int a=1;a>=3;a++){
    display.drawRect(0,0,127,31,WHITE);
    delay(100);
    display.drawRect(0,0,127,31,BLACK);
  }
  return;
} 

void getVal(){
  if (mode == 0){
    curValue[mode] = getOBDIIvalue("obdbrzoiltempf");//modeList[mode]);
  }
  else if (mode == 1){
    curValue[mode] = getOBDIIvalue("obdafr");
  }
  else if (mode == 2){
    curValue[mode] = getOBDIIvalue("obdvolts");
  }
  else if (mode == 3){
    curValue[mode] = getOBDIIvalue("obdcoolantf");
  }
  else if (mode == 4){
    curValue[mode] = getOBDIIvalue("obdiat");
  }
  if (curValue[mode] > peaks[mode]){
    peaks[mode] = curValue[mode];
  }
}

void updateVal(){
  display.setTextSize(3);
  //draw old value
  display.setTextColor(BLACK);
  display.setCursor(50,12);
  if ( (mode == 1) || (mode == 2) ){ //AFR should show the decimal (others could be added to this list)
    display.println(previousReading[mode]);
  }
  else { //everything else should print an int value
    display.println((int)previousReading[mode]);
  }
  display.display();
  //draw new value
  display.setTextColor(WHITE);
  display.setCursor(50,12);
  if ( (mode == 1) || (mode == 2) ){ //AFR should show the decimal (others could be added to this list)
    display.println(curValue[mode]);
  }
  else { //everything else should print an int value
    display.println((int)curValue[mode]);
  }
  display.display();
  previousReading[mode] = curValue[mode]; 
  //delay(100); 
  return;
} 

float getOBDIIvalue(String whichSensor){
  //Serial.flush();
  float value = 0;
  char data[130];
  if (whichSensor.indexOf("obdcoolantf") >=0){
    Elm.coolantTemperature(value);
    value=value*1.8+32;
  }
  if (whichSensor.indexOf("obdafr") >= 0){
    Status = Elm.runCommand("0134",data,20);
    value = ((float)(strtol(&data[6],0,16)*256)+strtol(&data[9],0,16))/32768*14.7;  //(A*256+B)/32768*14.7
  }
  if (whichSensor.indexOf("obdiat") >= 0){ //USED
    Elm.intakeAirTemperature(value);
    value=value*1.8+32;
  }

  if (whichSensor.indexOf("obdvolts") >= 0){ //USED
    Elm.getVoltage(value);
  }

  if (whichSensor.indexOf("obdbrzoiltempf") >= 0){ //works and USED
    Status = Elm.runCommand("2101",data,130);
    value = ((float)strtol(&data[109],0,16) - 40) * 1.8 + 32;
  }

  //delay(100);  
  return value; 
}

