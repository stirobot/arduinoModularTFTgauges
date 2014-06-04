#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>


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

Adafruit_ST7735 tft = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST);

uint16_t background = ST7735_BLACK;
uint16_t outline = ST7735_WHITE;
uint16_t fill = ST7735_BLUE;
uint16_t textdefault = ST7735_RED;
uint16_t alert = ST7735_YELLOW;

void setup(){
  Serial.begin(9600); //debugging to serial console
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(background);
  tft.setRotation(1);
  tft.fillScreen(background); 
}

void loop(){
  /*
  //logging layout
  tft.setCursor(40, 5);
  tft.setTextColor(outline);
  tft.setTextSize(2);
  tft.println("Logging");
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.println("obdoiltemp:");
  tft.setCursor(110, 40);
  tft.println("210");
  tft.setCursor(10, 60);
  tft.println("obdvolts:");
  tft.setCursor(110, 60);
  tft.println("13.3");
  tft.setCursor(10, 80);
  tft.println("oilPsi:");
  tft.setCursor(110, 80);
  tft.println("85");
  tft.setCursor(10, 100);
  tft.println("obdrpms:");
  tft.setCursor(110, 100);
  tft.println("2503");
  */
  
  /*
  //single bar layout
  tft.setCursor(0, 5);
  tft.setTextColor(outline);
  tft.setTextSize(2);
  tft.println("Oil Temp");
  tft.fillRect(0, 40, 68, 40, fill);
  tft.setCursor(0, 100);
  tft.println("210");
  tft.setCursor(50,100);
  tft.println("F");
  */

  /*
  //double bar
  tft.setTextSize(1);
  tft.setTextColor(textdefault);
  tft.setCursor(0,0);
  tft.println("Oil Temp");
  tft.setCursor(0,70);
  tft.println("Coolant TEmp");
  tft.fillRect(0,20,68,30, fill);
  tft.fillRect(0,90,30,30, fill);
  
  tft.setCursor(100,0);
  tft.println("120");
  tft.setCursor(100,70);
  tft.println("79");
  */
  
  //accelerometer
  tft.setTextSize(1);
  tft.drawRect(60,0,40,128,outline);
  tft.drawRect(0,44,160,40,outline); 
  tft.drawRect(60,44,40,40,background);//empty the center line
  tft.fillCircle(115,64,17,fill);//x ball
  tft.fillCircle(80,70,17,fill);//y ball
  
  tft.setCursor(0,0);
  tft.println("x: 0.5");
  tft.setCursor(110,0);
  tft.println("y: 0.3");
  tft.setCursor(0,100);
  tft.println("px: 0.5");
  tft.setCursor(110,100);
  tft.println("py: -0.8"); //will have to use abs() compare to get peaks here
  
  //round
  /*tft.setTextSize(2);
  tft.setTextColor(textdefault);
  tft.setCursor(0, 2);
  tft.println("Oil Temp");
  tft.drawCircle(80, 70, 50, textdefault);
  tft.drawCircle(80, 70, 49, textdefault);
  tft.setCursor(65,60);
  tft.println("210");
  
  tft.fillCircle(80,113,5,fill); //no calc for the zero position
  float rad = 0.0;
  for (int i = 0; i <= 270; i+=5){
      rad = i * PI / 180;
      //works for lower left quadrant
      //upper left is 80-sin(i)*43, 70-cos(i)*43
      //upper right is 80+sin(i)*43, 70+cos(i)*43
      //need special cases for 0, 90, 180 deg (because sin(0)=0
      tft.fillCircle( (80-((int)(sin(rad)*43.0) ) ), (70+( (int)(cos(rad)*43.0))), 5 , fill );
  } 
 */ 
  delay(10000);
  
  //accel cross
  
}







