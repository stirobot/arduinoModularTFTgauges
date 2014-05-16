/*modular gauges...
 a new start to the 1.8" tft based gauges
 most things are configured in the SD card config file, "gauges" 
 
 Modular
 3 types of gauge
 many types of sensor
 you choose the order
 you choose the peaks/warns/etc.
 */

//There is an issue where only one file can be opened at a time...
//so maybe there is a logging page that you can hit the logging button...

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

//int chipSelect = 10; //for adafruit SD shields + tfts
Adafruit_ST7735 tft = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST);

File config;
uint16_t background = ST7735_BLACK;
uint16_t outline = ST7735_WHITE;
uint16_t fill = ST7735_BLUE;
uint16_t textdefault = ST7735_RED;
uint16_t alert = ST7735_YELLOW;

//This is a character buffer that will store the data from the serial port
char rxData[20];
char rxIndex=0;

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
  
  Serial1.begin(9600); //spi for the display, serial for debug, serial1 for OBD II

  //read and assign color configs
  config = SD.open("gauges");
  background = textColorToColor(searchFile("background"));
  outline = textColorToColor(searchFile("outline"));
  fill = textColorToColor(searchFile("fill"));
  textdefault = textColorToColor(searchFile("textdefault"));
  alert = textColorToColor(searchFile("alert"));
  config.close();

  //run diagnostic for sensors?

  //blank screen
  tft.fillScreen(background);
  delay(2000);
  Serial1.println("ATZ");
  delay(2000);
  Serial1.flush();
}

//todo/don't forget:
//-peaks and peak reset
//-add OBD II custom PIDS
//-add more PIDS

void loop() {
  String sensor1, sensor2, sensor3, sensor4;
  String sensor1text, sensor2text, sensor3text, sensor4text;
  unsigned int sensor1pin, sensor2pin, sensor3pin, sensor4pin; //regular sensors get pins...but will check if sensor gets pin of 0...marks obdII
  unsigned int sensor1max, sensor2max, sensor3max, sensor4max;
  unsigned int sensor1alert, sensor2alert, sensor3alert, sensor4alert;
  //read config file for next page
  String pagetype = searchFile("pagetype");
  if (pagetype == "twobar"){//2 sensors displayed in 2 bar charts
    sensor1 = searchFile("sensor1");
    sensor2 = searchFile("sensor2");
    sensor1text = searchFile("sensor1text");
    sensor2text = searchFile("sensor2text");
    sensor1max = searchFile("sensor1max").toInt();
    sensor2max = searchFile("sensor2max").toInt();
    sensor1alert = searchFile("sensor1alert").toInt();
    sensor2alert = searchFile("sensor2alert").toInt();
    //loop to show the display and check for the button press
  }

  else if (pagetype == "onebar"){//1 sensor 1 bar chart...bigger fonts
    sensor1 = searchFile("sensor1");
    sensor1text = searchFile("sensor1text");
    sensor1max = searchFile("sensor1max").toInt();
    sensor1alert = searchFile("sensor1alert").toInt();
    //loop to show the display and check for the button press
  }

  else if (pagetype == "logging"){//up to 4 sensors shown...log everything to file
    sensor1 = searchFile("sensor1");
    sensor2 = searchFile("sensor2");
    sensor3 = searchFile("sensor3");
    sensor4 = searchFile("sensor4");    
  }

  else if (pagetype == "round"){//1 sensor 1 round chart
    sensor1 = searchFile("sensor1");
    sensor1text = searchFile("sensor1text");
    sensor1max = searchFile("sensor1max").toInt();
    sensor1alert = searchFile("sensor1alert").toInt();
    //loop to show the display and check for the button press
  }

  else if (searchFile("pagetype") == "accelerometer"){//cross bar type chart for accelerometer
    //special...just show the accelerometer and get the accelerometer data
  }
}

int getOBDIIvalue(String whichSensor){
  int value = 0;
  if (whichSensor == "obdspeed"){
    Serial1.println("010D"); //mode 1 0D PID
    getResponse();  //command echoed
    getResponse();  //value
    value = strtol(&rxData[6],0,16); //convert the string to integer
  }
  if (whichSensor == "obdrpms"){
    Serial1.println("010C"); //mode 1 0C PID (rpm)
    getResponse();  //command echoed
    getResponse();  //value
    value = ((strtol(&rxData[6],0,16)*256)+strtol(&rxData[9],0,16))/4; //aka ((A*256)+B)/4 
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
    if (sensorName == "oiltemp"){
      return lookup_oil_temp(pinNumber);
    }
    if (sensorName == "oilpressure"){
      return lookup_oil_psi(pinNumber);
    }
    if (sensorName == "accelx"){
      return getAccelerometerData(pinNumber);
    }
    if (sensorName == "accely"){
      return getAccelerometerData(pinNumber);
    }
    if (sensorName == "boostpressure"){
      return lookup_boost(pinNumber);
    }
    if (sensorName == "temperature"){
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
  if (color == "red"){
    return ST7735_RED;
  }
  else if (color == "magenta"){
    return ST7735_MAGENTA;
  }
  else if (color == "blue"){
    return ST7735_BLUE;
  }
  else if (color == "green"){
    return ST7735_GREEN;
  }
  else if(color == "black"){
    return ST7735_BLACK;
  }
  else  if (color == "white"){
    return ST7735_WHITE;
  }
  else if (color == "yellow"){
    return ST7735_YELLOW;
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
  Serial.println(config.available());
  while (config.available()){
    line = config.readStringUntil('\n');
    if (line.startsWith("#")){ //skip comments
    }
    else if (line.startsWith(searchFor)){
      config.close();
      int colonPos = line.indexOf(":");
      return( line.substring( line.indexOf(":")+1, line.indexOf("\n")-2 ) );
    }
  }
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


