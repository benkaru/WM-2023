/*
  Arduino Nano 33 BLE Getting Started
  BLE peripheral with a simple Hello World greeting service that can be viewed
  on a mobile phone
  Adapted from Arduino BatteryMonitor example
*/

#include <VEML6070.h>
#include <ArduinoBLE.h>
#include "RTClib.h"

#define LED D12  //connect LED to digital pin2

char* UV_str[] = { "low level", "moderate level", "high_level", "very high", "extreme" };

const int B = 4275;     // B value of the thermistor
const int R0 = 100000;  // R0 = 100k

BLEService sensorService("1821");  // User defined service

// BLEStringCharacteristic greetingCharacteristic("2A56",        // standard 16-bit characteristic UUID
//                                                BLERead, 20);  // remote clients will only be able to read this

// BLEIntCharacteristic myBLEIntCharacteristic("2AAE",
//                                             BLERead | BLEWrite);

BLEShortCharacteristic tempCharacteristic("2A6E", BLERead);
BLEUnsignedShortCharacteristic uvCharacteristic("2A76", BLERead);

RTC_PCF8523 rtc;


void setup() {
  Serial.begin(57600);

  // initialize serial communication
  if (!rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.start();


  pinMode(LED_BUILTIN, OUTPUT);  // initialize the built-in LED pin

  if (!BLE.begin()) {  // initialize BLE
    Serial.println("starting BLE failed!");
    while (1)
      ;
  }

  delay(1000);
  BLE.setLocalName("Nano33BLE");            // Set name for connection
  BLE.setAdvertisedService(sensorService);  // Advertise service
  // sensorService.addCharacteristic(greetingCharacteristic);  // Add characteristic to service
  // sensorService.addCharacteristic(myBLEIntCharacteristic);  // Add characteristic to service
  sensorService.addCharacteristic(tempCharacteristic);
  sensorService.addCharacteristic(uvCharacteristic);

  BLE.addService(sensorService);  // Add service
  // greetingCharacteristic.writeValue("Hello World!");  // Set greeting string
  // myBLEIntCharacteristic.writeValue(10);
  tempCharacteristic.writeValue(0);
  uvCharacteristic.writeValue(0);

  BLE.advertise();  // Start advertising
  Serial.print("Peripheral device MAC: ");
  Serial.println(BLE.address());
  Serial.println("Waiting for connections...");

  delay(1000);
  VEML.begin();
  pinMode(LED, OUTPUT);
}

void loop() {
  BLEDevice central = BLE.central();  // Wait for a BLE central to connect

  // if a central is connected to the peripheral:
  if (central) {
    Serial.print("Connected to central MAC: ");
    // print the central's BT address:
    Serial.println(central.address());
    // turn on the LED to indicate the connection:
    digitalWrite(LED_BUILTIN, HIGH);

    while (central.connected()) {
      temperature();
      uv();

      delay(5000);
      // greetingCharacteristic.writeValue("Test 123");
      // myBLEIntCharacteristic.writeValue(10);

      digitalWrite(LED, HIGH);  // set the LED on
      delay(3000);              // for 500ms
      digitalWrite(LED, LOW);   // set the LED off
    }                           // keep looping while connected

    // when the central disconnects, turn off the LED:
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Disconnected from central MAC: ");
    Serial.println(central.address());
  }
}

void temperature() {
  int a = analogRead(A6);

  float R = 1023.0 / a - 1.0;
  R = R0 * R;

  float temperature = 1.0 / (log(R / R0) / B + 1 / 298.15) - 273.15;  // convert to temperature via datasheet

  short t = convertFloatToShort(temperature);
  tempCharacteristic.writeValue(t);
  Serial.print("Temperature = ");
  Serial.println(temperature);
}

void uv() {
  uint16_t uvs = VEML.read_uvs_step();

  uvCharacteristic.writeValue(uvs);
  Serial.print("Current UVS is: ");
  Serial.println(uvs);
}


short convertFloatToShort(float x) {
  x = x * 100;
  if (x < -32768) {
    return -32768;
  }
  if (x > 32767) {
    return 32767;
  }
  return (short)round(x);
}
