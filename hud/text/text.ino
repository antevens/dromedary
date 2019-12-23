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


void setup() {
  // Start TOLED/SPI communications
  SPI_PORT.begin();
  TOLED.begin(CS_PIN, DC_PIN, SPI_PORT);
  TOLED.setCurrentWindowColorSequence((color_t)&color);

  // Start serial communications
  Serial.begin(9600);
  Serial.println(F("Dromedary: Transparent Graphical OLED HUD"));

  // Start bluetooth communications
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");

    while (1);
  }

  BLE.setLocalName("Harald");
  BLE.setAdvertisedService(batteryService);
  batteryService.addCharacteristic(batteryLevelChar);
  BLE.addService(batteryService);
  batteryLevelChar.writeValue(oldBatteryLevel);
  
  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");

}

void loop() {

  //TOLED.rectangle(0, 0, 100, 100,true,(color_t)&color);        //font "color"

  // First let's draw a line on the default window (which is equivalent to the whole screen)
  //TOLED.lineSet(0, 0, 40, 60);

  TOLED.println("752W");
  delay(1000);
  TOLED.setTextCursor(0,10);
  TOLED.println("11.8s");
  delay(1000);
  TOLED.setTextCursor(0,20);
  TOLED.println("62.3km/h");
  delay(1000);
  TOLED.resetTextCursor();

  BLEDevice central = BLE.central();

  if (central) 
  {
  Serial.print("Connected to central: ");
  Serial.println(central.address());
  digitalWrite(LED_BUILTIN, HIGH);
  
  while (central.connected()) {
  
        int battery = analogRead(A0);
        int batteryLevel = map(battery, 0, 1023, 0, 100);
        TOLED.print("Battery Level % is now: ");
        TOLED.println(batteryLevel);
        batteryLevelChar.writeValue(batteryLevel);
        delay(200);
  
  }
  digitalWrite(LED_BUILTIN, LOW);
  Serial.print("Disconnected from central: ");
  Serial.println(central.address());
  }
}
