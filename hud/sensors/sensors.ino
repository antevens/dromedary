#include "hyperdisplay_conf.h"
#include <HyperDisplay_UG2856KLBAG01.h>
#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>
#include <Arduino_LPS22HB.h>
#include <Arduino_HTS221.h>
#include <Arduino_APDS9960.h>

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

// Set up Transparent Organic LED Display and SPI communications
UG2856KLBAG01_SPI TOLED;

// Create Windows
wind_info_t windowZero, windowOne, windowTwo, windowThree, windowFour, windowFive;  // Create some window objects

// variables for button
const int buttonPin = 2;
int oldButtonState = LOW;

void controlLed(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting ...");

  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
    }
  // retrieve the LED characteristic
  BLECharacteristic ledCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1214");

  if (!ledCharacteristic) {
    Serial.println("Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  } else if (!ledCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable LED characteristic!");
    peripheral.disconnect();
    return;
  }

  while (peripheral.connected()) {
    // while the peripheral is connection

    // read the button pin
    int buttonState = digitalRead(buttonPin);

    if (oldButtonState != buttonState) {
      // button changed
      oldButtonState = buttonState;

      if (buttonState) {
        Serial.println("button pressed");
      } else {
        Serial.println("button released");
      }
    }
  }

  Serial.println("Peripheral disconnected");
}

void setup() {
  // Start serial communications
  Serial.begin(9600);
  Serial.println(F("Dromedary: Transparent Graphical OLED HUD"));

  // Start TOLED/SPI communications
  SPI_PORT.begin();
  TOLED.begin(CS_PIN, DC_PIN, SPI_PORT);

  // Start sensor communications
  IMU.begin();
  BARO.begin();
  HTS.begin();
  APDS.begin();
  BLE.begin();

  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    // discovered a peripheral, print out address, local name, and advertised service
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();

    // stop scanning
    BLE.stopScan();

    controlLed(peripheral);

    // peripheral disconnected, start scanning again
    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
  }

  APDS.setGestureSensitivity(100);
  //APDS.setLEDBoost(); 0/1/2/3

  TOLED.setWindowDefaults(&windowZero);
  TOLED.setWindowDefaults(&windowOne);
  TOLED.setWindowDefaults(&windowTwo);
  TOLED.setWindowDefaults(&windowThree);
  TOLED.setWindowDefaults(&windowFour);
  TOLED.setWindowDefaults(&windowFive);

  TOLED.setWindowColorSet(&windowZero);
  TOLED.setWindowColorSet(&windowOne);
  TOLED.setWindowColorSet(&windowTwo);
  TOLED.setWindowColorSet(&windowThree);
  TOLED.setWindowColorSet(&windowFour);
  TOLED.setWindowColorSet(&windowFive);

  windowZero.xMin = 0;
  windowZero.yMin = 0;
  windowZero.xMax = 127;
  windowZero.yMax = 9;
  windowZero.clearCharacterArea = true;

  windowOne.xMin = 0;
  windowOne.yMin = 10;
  windowOne.xMax = 127;
  windowOne.yMax = 19;
  windowOne.clearCharacterArea = true;

  windowTwo.xMin = 0;
  windowTwo.yMin = 20;
  windowTwo.xMax = 127;
  windowTwo.yMax = 29;
  windowTwo.clearCharacterArea = true;

  windowThree.xMin = 0;
  windowThree.yMin = 30;
  windowThree.xMax = 127;
  windowThree.yMax = 39;
  windowThree.clearCharacterArea = true;

  windowFour.xMin = 0;
  windowFour.yMin = 40;
  windowFour.xMax = 127;
  windowFour.yMax = 49;
  windowFour.clearCharacterArea = true;

  windowFive.xMin = 0;
  windowFive.yMin = 50;
  windowFive.xMax = 127;
  windowFive.yMax = 59;
  windowFive.clearCharacterArea = true;

  //TOLED.setContrastControl(255)
  TOLED.pCurrentWindow = &windowZero;
  TOLED.println("Dromedary");
  Serial.println(F("Dromedary"));
  TOLED.resetTextCursor();
  TOLED.pCurrentWindow = &windowOne;
  TOLED.println("starting ...");
  Serial.println(F("starting ..."));
  TOLED.resetTextCursor();
  TOLED.pCurrentWindow = &windowTwo;
  TOLED.println("finding sensors ...");
  Serial.println(F("finding sensors .."));
  TOLED.resetTextCursor();
  TOLED.windowClear();
}

float xa, ya, za;
float xg, yg, zg;
void loop() {
  IMU.readAcceleration(xa, ya, za);
  IMU.readGyroscope(xg, yg, zg);
  float pressure = BARO.readPressure();
  float temperature = HTS.readTemperature();
  float humidity = HTS.readHumidity();
  /*
  APDS9960 - Gesture Sensor

  Gesture directions are as follows:
  - UP:    from USB connector towards antenna
  - DOWN:  from antenna towards USB connector
  - LEFT:  from analog pins side towards digital pins side
  - RIGHT: from digital pins side towards analog pins side
  */

  String accelData = "X " + String(xa, 1) + " Y " + String(ya, 1) + " Z " + String(za, 1);
  String gyroData = "X " + String(xg, 1) + " Y " + String(yg, 1) + " Z " + String(zg, 1);
  TOLED.pCurrentWindow = &windowZero;
  TOLED.println(accelData);
  TOLED.resetTextCursor();
  TOLED.pCurrentWindow = &windowOne;
  TOLED.println(gyroData);
  TOLED.resetTextCursor();
  TOLED.pCurrentWindow = &windowTwo;
  TOLED.println(String(pressure, 2) + " kPa");
  TOLED.resetTextCursor();
  TOLED.pCurrentWindow = &windowThree;
  TOLED.println(String(temperature, 2) + " C");
  TOLED.resetTextCursor();
  TOLED.pCurrentWindow = &windowFour;
  TOLED.println(String(humidity, 2) + " %");
  TOLED.resetTextCursor();
  TOLED.pCurrentWindow = &windowFive;
  if (APDS.gestureAvailable()) {
    // Detected a guesture
    TOLED.println("Gesture: " + String(APDS.readGesture()));
    TOLED.resetTextCursor();
  } else {
    TOLED.println("Proximity: " + String(APDS.readProximity()));
    TOLED.resetTextCursor();
  }

  //TOLED.rectangle(0, 0, 100, 100,true,(color_t)&color);        //font "color"
  // First let's draw a line on the default window (which is equivalent to the whole screen)
  //TOLED.lineSet(0, 0, 40, 60);
  //IMU.readAcceleration(x, y, z);
  //TOLED.setContrastControl(0);
  //Serial.println("stuff");
  //TOLED.pCurrentWindow = &pairingWindow;
  //TOLED.clearDisplay();
  //TOLED.pCurrentWindow = &windowZero;
  //TOLED.windowClear();
  //TOLED.println("752W");
  //TOLED.resetTextCursor();
  //TOLED.setTextCursor(0,10);
  //TOLED.println(x);
  //TOLED.println(y);
  //TOLED.println(z);
  //TOLED.pCurrentWindow = &defaultWindow;
  //TOLED.setTextCursor(0,20);
  //TOLED.pCurrentWindow = &windowOne;
  //TOLED.windowClear();
  //TOLED.println("62.3km/h");
  //TOLED.show();
  //TOLED.resetTextCursor();
}
