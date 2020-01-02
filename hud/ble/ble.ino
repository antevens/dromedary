#include <ArduinoBLE.h>

const String cadence_speed_uuid="1816";
const String heart_rate_uuid="180d";
const String power_uuid="1818";
const String insulin_uuid="183a";
const String glucose_uuid="1808";



void setup() {
  // Start serial communications
  Serial.begin(9600);
  while (!Serial) {
    delay(100);
  }
  
  Serial.println(F("Dromedary OLED HUD"));

  // Start BLE communications
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  // Scan for devices
  Serial.println(F("Scanning for BLE devices"));
  //BLEDevice cadence_speed = scanSensorByUuid(cadence_speed_uuid);
  BLEDevice heart_rate = scanSensorByUuid(heart_rate_uuid);
  //BLEDevice power = scanSensorByUuid(power_uuid);

  // Connect to devices
  Serial.println(F("Connecting to BLE devices"));
  //connectSensor(cadence_speed);
  connectSensor(heart_rate);
  //connectSensor(power);

  BLEService batteryService = heart_rate.service("180f");
  if (batteryService) {
    // use the service
    if (batteryService.hasCharacteristic("2a19")) {
      Serial.println("Battery service has battery level characteristic");
    }
  } else {
    Serial.println("Peripheral does NOT have battery service");
  }

}

void loop() {
  
 Serial.println("looping");
 delay(1000);

}

// Scan for a sensor by type
BLEDevice scanSensorByUuid(String uuid) {
  Serial.println(F("Scanning for BLE device"));
  BLEDevice peripheral;
  while (true) {
    BLE.scanForUuid(uuid);
    peripheral = BLE.available();
    if (peripheral) {
        Serial.print(F("Found BLE device: "));
        Serial.println(uuid);;  
        BLE.stopScan();
        return peripheral;
    }
    Serial.print(".");
    delay(1000);
  }
}

// Connect to a sensor
void connectSensor(BLEDevice peripheral) {
  while (true) {
    if (peripheral.connect()) {
      Serial.print("Connected to sensor ");
      if (peripheral.hasLocalName()) {
        Serial.println(peripheral.localName());
      }
      break;
    }
    Serial.print(".");
    delay(1000);
  }
}



// BLE.stopScan();
// peripheral.connect()
// peripheral.discoverAttributes()
// peripheral.disconnect();
// peripheral.deviceName()
//Serial.println(peripheral.appearance(), HEX);
//    BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");

  // loop the services of the peripheral and explore each
  //for (int i = 0; i < peripheral.serviceCount(); i++) {
  //  BLEService service = peripheral.service(i);
  //  exploreService(service);
  //}

// while (peripheral.connected()) {

//service.uuid()


 // retrieve the simple key characteristic
 //  BLECharacteristic simpleKeyCharacteristic = peripheral.characteristic("ffe1");
