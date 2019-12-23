#include "HyperDisplay_UG2856KLBAG01.h"
#include <ArduinoBLE.h>

// add voice control
// add bluetooth pairing


 // BLE Battery Service
BLEService batteryService("180F");

// BLE Battery Level Characteristic
BLEUnsignedCharCharacteristic batteryLevelChar("2A19",  // standard 16-bit characteristic UUID
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

int oldBatteryLevel = 0;  // last battery level reading from analog input
long previousMillis = 0;  // last time the battery level was checked, in ms


//////////////////////////
//    Pinout and Config //
//////////////////////////
#define SERIAL_PORT Serial  
#define WIRE_PORT Wire      // Used if USE_SPI == 0
#define SPI_PORT SPI        // Used if USE_SPI == 1

#define RES_PIN 8           // Optional
#define CS_PIN 9            // Used only if USE_SPI == 1
#define DC_PIN 10            // Used only if USE_SPI == 1

#define USE_SPI 1           // Choose your interface. 0 = I2C, 1 = SPI

// END USER SETUP


// Object Declaration. A class exists for each interface option
#if USE_SPI
  UG2856KLBAG01_SPI myTOLED;  // Declare a SPI-based Transparent OLED object called myTOLED
#else
  UG2856KLBAG01_I2C myTOLED;  // Declare a I2C-based Transparent OLED object called myTOLED
#endif /* USE_SPI */


uint8_t color = 0x01;
uint8_t noColor = 0x00;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Example1_DisplayTest: Transparent Graphical OLED"));

#if USE_SPI 
  SPI_PORT.begin();
  myTOLED.begin(CS_PIN, DC_PIN, SPI_PORT);                  // Begin for SPI requires that you provide the CS and DC pin numbers
#else
  WIRE_PORT.begin();
  myTOLED.begin(WIRE_PORT, false, SSD1309_ARD_UNUSED_PIN);  // Begin for I2C has default values for every argument
  Wire.setClock(400000);
#endif /* USSE_SPI */

  pinMode(LED_BUILTIN, OUTPUT);
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");

    while (1);
  }

  BLE.setLocalName("BatteryMonitor");
  BLE.setAdvertisedService(batteryService);
  batteryService.addCharacteristic(batteryLevelChar);
  BLE.addService(batteryService);
  batteryLevelChar.writeValue(oldBatteryLevel);
  
  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");

}

void loop() {
  myTOLED.setCurrentWindowColorSequence((color_t)&color);
  //myTOLED.rectangle(0, 0, 100, 100,true,(color_t)&color);        //font "color"

  // First let's draw a line on the default window (which is equivalent to the whole screen)
  //myTOLED.lineSet(0, 0, 40, 60);

  myTOLED.println("752W");
  delay(1000);
  myTOLED.setTextCursor(0,10);
  myTOLED.println("11.8s");
  delay(1000);
  myTOLED.setTextCursor(0,20);
  myTOLED.println("62.3km/h");
  delay(1000);
  myTOLED.resetTextCursor();

  BLEDevice central = BLE.central();

  if (central) 
  {
  Serial.print("Connected to central: ");
  Serial.println(central.address());
  digitalWrite(LED_BUILTIN, HIGH);
  
  while (central.connected()) {
  
        int battery = analogRead(A0);
        int batteryLevel = map(battery, 0, 1023, 0, 100);
        myTOLED.print("Battery Level % is now: ");
        myTOLED.println(batteryLevel);
        batteryLevelChar.writeValue(batteryLevel);
        delay(200);
  
  }
  digitalWrite(LED_BUILTIN, LOW);
  Serial.print("Disconnected from central: ");
  Serial.println(central.address());
  }
}
