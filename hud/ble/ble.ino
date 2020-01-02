#include <ArduinoBLE.h>

// BLE Service UUIDs
const char* cadence_speed_uuid="1816";
const char* heart_rate_uuid="180d";
const char* power_uuid="1818";
const char* insulin_uuid="183a";
const char* glucose_uuid="1808";

// BLE Service Characteristic UUIDs
const char* heart_rate_measurement_uuid="2a37";
const char* heart_rate_location_uuid="2a38";


// A structure to hold HR data, maximum length 6bytes
// First bit indicates 8 vs 16 bit format
// Heart rate is stored in first 2 bytes
// Energy expended is stored in the next 2 bytes
// RR Interval (HR Variability in the final 2 bytes
https://stackoverflow.com/questions/33917836/bluetooth-heartrate-monitor-byte-decoding
union hrData
{
  struct
  {
    unsigned int heartrate: 1;
    unsigned int energy_expended: 1;
    unsigned int rr_interval: 1;
  } elements;
  uint8_t bytes[6];
};

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
  //BLEDevice cadence_speed_sensor = scanSensorByUuid(cadence_speed_uuid);
  BLEDevice heart_rate_sensor = scanSensorByUuid(heart_rate_uuid);
  //BLEDevice power_sensor = scanSensorByUuid(power_uuid);

  // Connect to devices
  Serial.println(F("Connecting to BLE devices"));
  //connectSensor(cadence_speed_sensor);
  connectSensor(heart_rate_sensor);
  //connectSensor(power_sensor);

  BLEService batteryService = heart_rate_sensor.service("180f");
  if (batteryService) {
    // use the service
    if (batteryService.hasCharacteristic("2a19")) {
      Serial.println("Battery service has battery level characteristic");
    }
  } else {
    Serial.println("Peripheral does NOT have battery service");
  }

  if (heart_rate_sensor.discoverService(heart_rate_uuid)) {
    Serial.println("Discovered HR service");
  }
  
  if (heart_rate_sensor.hasService(heart_rate_uuid)) {
    Serial.println("Has HR service");
  }
  BLEService hrService = heart_rate_sensor.service(heart_rate_uuid);
  //int hrServiceCharCount = hrService.characteristicCount();
  //Serial.println(hrServiceCharCount);
  
  BLECharacteristic hrMeasurment = hrService.characteristic(heart_rate_measurement_uuid);
  //Serial.println(test0.uuid());
  //Serial.println(test0.value());
  
  //BLECharacteristic test1 = hrService.characteristic(1);
  //Serial.println(test1.uuid());
  //Serial.println(test1.value());

  if (hrMeasurment.subscribe()) {
    Serial.println("Subscribed");
  }
  
  union hrData hrStats, hrBytes;
  while (true) {
      //for ( int i = 0; i < 8; i++ )
      //{
      // Serial.print(hrBytes[i]);
      //}
      //Serial.println(hrMeasurment.valueSize());
      //Serial.println(hrMeasurment.valueLength());

      //Serial.println(hrMeasurment.value());
      Serial.println(hrMeasurment.valueUpdated());
      hrMeasurment.readValue(*hrBytes.bytes);
      Serial.println(hrStats.elements.heartrate);
      Serial.println(hrStats.elements.energy_expended);
      Serial.println(hrStats.elements.rr_interval);
      Serial.println();
      delay(500);
  }
  //byte hrValue = 0;
  //hrMeasurment.readValue(hrValue);

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
    delay(10);
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
    delay(10);
  }
}


// characteristic.subscribe !!!!
// characteristic.valueUpdated()
// characteristic.setEventHandler() !!!!!

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
