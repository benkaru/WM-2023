#include <VEML6070.h>
#include <Ultrasonic.h>
#include "RTClib.h"

#define LED D12  // connect LED to digital pin2

const int B = 4275;     // B value of the thermistor
const int R0 = 100000;  // R0 = 100k

Ultrasonic ultrasonic(D11);
RTC_PCF8523 rtc;

// Nachts?
bool night = false;
uint8_t last_hour = 0;
uint8_t last_minute = 0;
// pumpeneinstellung
uint8_t runningCycles = 60;
uint8_t runningMinutes = 5;
// wieviele Tage die gleiche Einstellung der Pumpe bleibt
uint8_t intervall = 1;
// manuelle Steuerung?
bool manual = false;

// sensordaten
float temperature;
uint16_t uvs;
long ultrasonic_range;

uint32_t next_measure = 0;
uint32_t delay_between_measurements = 20; // in Sekunden

// Nacht-Faktor = nachts wird nie gegossen
bool isNight(DateTime time) {
  return time.hour() >= 22 || time.hour() < 6;
}

// feste Zahlen müssen durch setValue ersetzt werden
void wateringManual(bool night) {
  if (night) {
    runningCycles = 8 * 60;
  } else {
    runningCycles = 60;
    runningMinutes = 5;
  }
}

// hier noch die Angaben mit temp, uv, ultrasonic ergänzen
void wateringAuto(uint16_t uvs, short t, long ultra) {
  if (ultra < 25 && ultra >= 0) {
    if (t > 28) {
      highMaintenanceWater();
    } else if (t < 28 && t >= 20) {
      standardWater();
    } else {
      lowMaintenanceWater();
    }
  } else if (ultra > 25) {
    lowMaintenanceWater();
  }
  // falls irgendeine Einstellung nicht passt
  else {
    standardWater();
  }
}

void standardWater() {
  runningCycles = 60;
  runningMinutes = 5;
}

void lowMaintenanceWater() {
  runningCycles = 90;
  runningMinutes = 3;
}

void highMaintenanceWater() {
  runningCycles = 60;
  runningMinutes = 7;
}


void setup() {
  Serial.begin(57600);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.start();

  VEML.begin();
  pinMode(LED, OUTPUT);
}

void loop() {
  DateTime time = rtc.now();

  if (manual) {
    Serial.println("Umsetzung WIP");
  } else {
    wateringAuto(uvs, temperature, ultrasonic_range);
  }

  uint32_t ts = time.unixtime();

  if (ts > next_measure) {
    measureTime();
    measureTemperature();
    measureUv();
    measureUltraSonic();
    next_measure = ts + delay_between_measurements;
  }

  uint8_t min = time.second();

  // Skip night
  if (isNight(time)) {
    return;
  }

  if (last_minute != min) {
    // update to the new seconds value
    last_minute = min;
    Serial.print(time.hour(), DEC);
    Serial.print(":");
    Serial.print(time.minute(), DEC);
    Serial.print(":");
    Serial.print(time.second(), DEC);
    Serial.println();
    // at 0 minute the relays turn ON
    if (min == 0) {
      startWater();
    }
    // after runningMinutes the relays turn OFF
    else if (min == runningMinutes) {
      stopWater();
    }
  }
}

void startWater() {
  Serial.println("Relay ON");
  digitalWrite(LED, HIGH);
}

void stopWater() {
  Serial.println("Relay OFF");
  digitalWrite(LED, LOW);
}

void measureTime() {
  DateTime now = rtc.now();
  Serial.print("Time = ");
  Serial.println(now.unixtime());
}

void measureTemperature() {
  int a = analogRead(A6);

  float R = 1023.0 / a - 1.0;
  R = R0 * R;

  temperature = 1.0 / (log(R / R0) / B + 1 / 298.15) - 273.15;  // convert to temperature via datasheet

  Serial.print("Temperature = ");
  Serial.println(temperature);
}

void measureUv() {
  uvs = VEML.read_uvs_step();

  Serial.print("UVS = ");
  Serial.println(uvs);
}

void measureUltraSonic() {
  ultrasonic_range = ultrasonic.MeasureInCentimeters();

  Serial.print("Ultrasonic = ");
  Serial.println(ultrasonic_range);
}
