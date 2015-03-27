// oledOBDgaugesSimpleIrvinedLibTest.ino
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
char rxData[115];
char rxIndex=0;

float value = 0;
unsigned int ratio = 0;
float previousValue = 0;
byte Status;
byte StatusOld;

Elm327 Elm;
byte status;

void setup() {
 status=Elm.begin();
 display.begin(SSD1306_SWITCHCAPVCC);
 display.display();
 display.clearDisplay();
 display.display();


 //did the obd ii init correctly
 display.setCursor(10, 10);
 display.println(status);
 display.display();
 //Elm.runCommand("AT SH 7E0",rxData,110);
 delay(1000);
 display.clearDisplay();

}

void loop() {
	char data[130];
	//Elm.getVoltage(value);
	//Elm.intakeAirTemperature(value);
	//Elm.coolantTemperature(value);
	//value=value*1.8+32;
	//Elm.engineRPM(value);
	//Elm.timingAdvance(value);
	//Elm.o2WRCurrent("34", ratio, value);
	Status = Elm.runCommand("2101",data,130);
	value = ((float)strtol(&data[109],0,16) - 40) * 1.8 + 32;
	display.setCursor(10,10);
	display.setTextColor(BLACK);
	display.println(previousValue);
	display.setCursor(10,20);
	display.println(data[109]);
	display.display();
	display.setCursor(10,10);
	display.setTextColor(WHITE);
	display.println(value);
	display.setCursor(10,20);
	display.println(data[109]);
	display.display();
	delay(100);
	previousValue = value;
	StatusOld = Status;
}

//01F 0: 61 01 00 00 00 00 1: 55 66 55 4C 65 4C 00 2: 00 00 00 00 0D 2A 54 3: 21 21 00 7A 0A A4 13 4: 6C 30 BC 4C 00 z0 00
//01F  0: 61 01 00 00 00 00  1: 55 66 60 4A 65 7E 00  2: 00 00 00 00 09 26 51  3: 21 21 00 41 05zFC 09  4: E5 31 2B 76 00 00 00
//117? 118?  because of extra line feed characters that need to be skipped