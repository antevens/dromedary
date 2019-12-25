#include "hyperdisplay_custom_conf.h"
#include "HyperDisplay_UG2856KLBAG01.h"
#include <ArduinoBLE.h>

// add voice control
// add bluetooth pairing
uint8_t defaultScreenMem[128*56];    // Reserve 128*56 bytes worth of memory
uint8_t pairingScreenMem[128*56];    // Reserve 128*56 bytes worth of memory

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

// Set up windows
wind_info_t defaultWindow, pairingWindow;
  
void setup() {
  // Configure windows
  TOLED.setWindowDefaults(&defaultWindow);
  TOLED.setWindowDefaults(&pairingWindow);

  pairingWindow.xMin = 110;
  pairingWindow.yMin = 10;
  pairingWindow.xMax = 54;
  pairingWindow.yMax = 54;
  defaultWindow.xMin = 0;
  defaultWindow.yMin = 0;
  defaultWindow.xMax = 128;
  defaultWindow.yMax = 56;
  
  // Start TOLED/SPI communications
  SPI_PORT.begin();
  TOLED.begin(CS_PIN, DC_PIN, SPI_PORT);
  //TOLED.setWindowColorClear()
  //TOLED.setWindowColorSet()
  
  TOLED.setCurrentWindowColorSequence((color_t)&color);
  TOLED.setCurrentWindowMemory((color_t)defaultScreenMem, 128*56);
  
  TOLED.setWindowColorSequence(&pairingWindow, (color_t)&color);
  TOLED.setWindowColorSequence(&defaultWindow, (color_t)&color);

  TOLED.setWindowMemory(&pairingWindow, (color_t)pairingScreenMem, 128*56);
  TOLED.setWindowMemory(&defaultWindow, (color_t)pairingScreenMem, 128*56);

  //TOLED.setContrastControl(0)
  //TOLED.setContrastControl(255)
  
  TOLED.println("Pairing");
  TOLED.pCurrentWindow = &pairingWindow;
  TOLED.buffer(&pairingWindow);
  TOLED.show(&pairingWindow);
  //TOLED.fillWindow((color_t)color);
  TOLED.setTextCursor(0,10);
  TOLED.println("Doing");
  delay(5000);
  TOLED.show(&pairingWindow);
  delay(5000);
  TOLED.pCurrentWindow = &defaultWindow;
  TOLED.show(&defaultWindow);
  //TOLED.buffer();
  TOLED.setTextCursor(0,20);
  TOLED.println("Jumping");
  delay(1000);
  //TOLED.show();
  delay(1000);
  
  // Start serial communications
  Serial.begin(9600);
  Serial.println(F("Dromedary: Transparent Graphical OLED HUD"));
}

void loop() {
  //TOLED.rectangle(0, 0, 100, 100,true,(color_t)&color);        //font "color"
  // First let's draw a line on the default window (which is equivalent to the whole screen)
  //TOLED.lineSet(0, 0, 40, 60);
  Serial.println("stuff");
  //TOLED.pCurrentWindow = &pairingWindow;
  TOLED.clearDisplay();
  TOLED.println("752W");
  TOLED.setTextCursor(0,10);
  TOLED.println("11.8s");
  TOLED.setTextCursor(0,20);
  TOLED.println("62.3km/h");
  //TOLED.pCurrentWindow = &defaultWindow;
  //TOLED.show();
  delay(5000);
  TOLED.resetTextCursor();
}
