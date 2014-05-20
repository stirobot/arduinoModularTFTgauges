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
}






