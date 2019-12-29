#include "hyperdisplay_custom_conf.h"
#include "HyperDisplay_UG2856KLBAG01.h"
#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>

// add voice control
// add bluetooth pairing

// The relative humidity and temperature sensor is a HTS221
// The barometer sensor is a LPS22HB
// The Gesture sensor is a APDS9960
// The digital microphone is a MP34DT05
// The IMU is a LSM9DS1 

//////////////////////////////
//    Pinout and Hardware   //
//////////////////////////////
#define SERIAL_PORT Serial
#define SPI_PORT SPI
#define CS_PIN 9
#define DC_PIN 10
// END USER SETUP

// Set colors for the TOLED display
uint8_t color = 0x01;
uint8_t noColor = 0x00;

// Set up Transparent Organic LED Display and SPI communications
UG2856KLBAG01_SPI TOLED;

// Create Windows
wind_info_t windowZero, windowOne, windowTwo, windowThree, windowFour, windowFive;  // Create some window objects

void setup() {
  // Start serial communications
  Serial.begin(9600);
  Serial.println(F("Dromedary: Transparent Graphical OLED HUD"));
  
  // Start TOLED/SPI communications
  SPI_PORT.begin();
  TOLED.begin(CS_PIN, DC_PIN, SPI_PORT);

  // Start sensor communications
  IMU.begin();

  TOLED.setWindowDefaults(&windowZero);
  TOLED.setWindowDefaults(&windowOne);
  TOLED.setWindowDefaults(&windowTwo);
  TOLED.setWindowDefaults(&windowThree);
  TOLED.setWindowDefaults(&windowFour);
  TOLED.setWindowDefaults(&windowFive);
   
  TOLED.setWindowColorSequence(&windowZero, (color_t)&color);
  TOLED.setWindowColorSequence(&windowOne, (color_t)&color);
  TOLED.setWindowColorSequence(&windowTwo, (color_t)&color);
  TOLED.setWindowColorSequence(&windowThree, (color_t)&color);
  TOLED.setWindowColorSequence(&windowFour, (color_t)&color);
  TOLED.setWindowColorSequence(&windowFive, (color_t)&color);
  
  windowZero.xMin = 0;
  windowZero.yMin = 0;
  windowZero.xMax = 127;
  windowZero.yMax = 9;
  
  windowOne.xMin = 0;
  windowOne.yMin = 10;
  windowOne.xMax = 127;
  windowOne.yMax = 19;
    
  windowTwo.xMin = 0;
  windowTwo.yMin = 20;
  windowTwo.xMax = 127;
  windowTwo.yMax = 29;
  
  //TOLED.setContrastControl(255)
  TOLED.pCurrentWindow = &windowZero;
  TOLED.println("Pairing");
  //TOLED.setTextCursor(0,10);
  TOLED.resetTextCursor();
  TOLED.pCurrentWindow = &windowOne;
  TOLED.println("Doing");
  TOLED.resetTextCursor();
  TOLED.pCurrentWindow = &windowTwo;
  //TOLED.setTextCursor(0,20);
  TOLED.println("Jumping");
  TOLED.resetTextCursor();
  //delay(3000);
  //TOLED.windowSet();
  delay(3000);
  TOLED.windowClear();

}

float x, y, z;
void loop() {
  //TOLED.rectangle(0, 0, 100, 100,true,(color_t)&color);        //font "color"
  // First let's draw a line on the default window (which is equivalent to the whole screen)
  //TOLED.lineSet(0, 0, 40, 60);
  //IMU.readAcceleration(x, y, z);
  //TOLED.setContrastControl(0);
  Serial.println("stuff");
  //TOLED.pCurrentWindow = &pairingWindow;
  //TOLED.clearDisplay();
  TOLED.pCurrentWindow = &windowZero;
  TOLED.windowClear();
  TOLED.println("752W");
  TOLED.resetTextCursor();
  //TOLED.setTextCursor(0,10);
  //TOLED.println(x);
  //TOLED.println(y);
  //TOLED.println(z);
  //TOLED.pCurrentWindow = &defaultWindow;
  //TOLED.setTextCursor(0,20);
  TOLED.pCurrentWindow = &windowOne;
  TOLED.windowClear();
  TOLED.println("62.3km/h");
  //TOLED.show();
  TOLED.resetTextCursor();
}
