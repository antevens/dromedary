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


// Holds HR data, maximum length 8bytes
// First bit indicates 8 vs 16 bit format of heart rate
// Heart rate is stored in the next 1/2 bytes
// Energy expended is stored in the next 2 bytes
// RR Interval (HR Variability) is stored in the rest of the bytes
//https://stackoverflow.com/questions/33917836/bluetooth-heartrate-monitor-byte-decoding
//https://stackoverflow.com/questions/17422218/bluetooth-low-energy-how-to-parse-r-r-interval-value
class hrData  {
  const byte long_form = 0b1;
  const byte energy_available = 0b001;
  const byte rr_available = 0b0001;
  int hrDataBytes;
  
  public:
  
    union hrDataStructure
    {
      byte flags: 1;
      uint8_t bytes[8]; // 8 bytes is the longest I've seen, in theory can go to 512 according to BLE standard
      struct
      {
        byte flags;
        uint8_t heartrate;
        uint16_t energy_expended;
        uint16_t rr_interval[];
      } shorter_form;
      struct
      {
        byte flags;
        uint16_t heartrate;
        uint16_t energy_expended;
        uint16_t rr_interval[];
      } longer_form;
    } hrByteData;

    int length() {
      return hrDataBytes;
    }
  
    void begin(byte *hrBytes, uint16_t bytes) {
      hrDataBytes = bytes;
      memcpy( hrByteData.bytes, hrBytes, bytes );
    };

    uint16_t heartRate()
    {
      if (hrByteData.flags & long_form) {
        return hrByteData.longer_form.heartrate;
      } else {
        return (uint16_t)hrByteData.shorter_form.heartrate;
      }
    };
    
    uint16_t energyExpended()
    {
      if (hrByteData.flags & energy_available) {
        if (hrByteData.flags & long_form) {
          return hrByteData.longer_form.energy_expended;
        } else {
          return hrByteData.shorter_form.energy_expended;
        }
      }
    };
    
    void rr_interval(byte *rrBytes)
    {
      if (hrByteData.flags & rr_available) {
        if (hrByteData.flags & long_form) {
          memcpy( hrByteData.longer_form.rr_interval, rrBytes, hrDataBytes - 5  );
        } else {
          memcpy( hrByteData.shorter_form.rr_interval, rrBytes, hrDataBytes - 4  );
        }
      }
    };
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
  
hrData hrStats;
  while (true) {
//Serial.println(hrMeasurment.value());
      hrMeasurment.readValue(*hrStats.hrByteData.bytes);
      Serial.println(hrMeasurment.valueSize());
      Serial.println(hrMeasurment.valueLength());
      Serial.println(hrMeasurment.valueUpdated());
      //hrMeasurment.readValue(*hrBytes.bytes);
      //Serial.println(hrStats.elements.heartrate);
      //Serial.println(hrStats.elements.energy_expended);
      //Serial.println(hrStats.elements.rr_interval);
      for ( int i = 0; i < 6; i++ )
      {
          Serial.print(hrStats.hrByteData.bytes[i]);
      }
      Serial.println();
      delay(5000);
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
