#include <Vector.h>
#include "ClosedCube_TMP116.h"
#include "sSense-HDC2010.h"
#include "ClosedCube_OPT3001.h"
#include <Adafruit_DPS310.h>

ClosedCube_OPT3001 opt3001;
ClosedCube::Sensor::TMP116 tmp116;
Adafruit_DPS310 dps;
HDC2010 ssenseHDC2010(HDC2010_I2C_ADDR);

float LUX_THRESHOLD = 60, TEMP_THRESHOLD = 20, HUM_THRESHOLD = 50, PRESSURE_THRESHOLD = .5;

sensors_event_t temp_event, pressure_event;
float current_temp, current_hum, previous_pressure;
int counter = 0, j;

struct values {
  float temperature;
  float humidity;
};

values buffer[120];
vector<values> vector(buffer);

void setup() {
  Serial.begin(19200);
  delay(2000);

  if (! dps.begin_I2C()) {
    while (1) yield();
  }

  dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);

  tmp116.begin(0x48);
  
  opt3001.begin(0x44);
  configureSensor();
  
  ssenseHDC2010.begin();
  ssenseHDC2010.reset();
  ssenseHDC2010.setMeasurementMode(TEMP_AND_HUMID);
  ssenseHDC2010.setRate(ONE_HZ);
  ssenseHDC2010.setTempRes(FOURTEEN_BIT);
  ssenseHDC2010.setHumidRes(FOURTEEN_BIT);
  ssenseHDC2010.triggerMeasurement();
}

void loop() {
    ++counter;

    if (vector.size() == vector.max_size()) {
    vector.remove(0);
    }

    values tmp = {tmp116.readTemperature(), ssenseHDC2010.readHumidity()};
    vector.push_back(tmp);

    if (counter == 6) {
      float sumH = 0, sumT = 0;
      
      for (j = 0; j < vector.size(); ++j) {
        sumT += vector[j].temperature;
        sumH += vector[j].humidity;
      }
      
      Serial.print(" T");
      current_temp = sumT / vector.size();
      Serial.print(current_temp);

      Serial.print(" t");
      Serial.print(turnHeaterOn(current_temp));

      current_hum = sumH / vector.size();
      Serial.print(" H");
      Serial.print(current_hum);

      Serial.print(" h");
      Serial.print(turnHumidifierOn(current_hum));

      previous_pressure = pressure_event.pressure;
      counter = 0;
  }

    for (j = 0; j < 20; ++j) {
      dps.getEvents(&temp_event, &pressure_event);
      OPT3001 result = opt3001.readResult();
      Serial.print(" w");
      Serial.print(isDoorOpened(pressure_event.pressure));
      Serial.print(" l");
      Serial.print(turnLightOn(result.lux));
      delay(100);
    }
}

int turnLightOn(float intensity) {
  if (intensity < LUX_THRESHOLD) return 1; 
  else return 0;
}  

int isDoorOpened(float current_pressure) {
  if (current_pressure - previous_pressure >= PRESSURE_THRESHOLD) return 1;
  else return 0;
}

int turnHeaterOn(float current_temp) {
  if (current_temp < TEMP_THRESHOLD) return 1;
  else return 0;
}

int turnHumidifierOn(float current_hum) {
  if (current_hum < HUM_THRESHOLD) return 1;
  else return 0;
}

void configureSensor() {
  OPT3001_Config newConfig;

  newConfig.RangeNumber = B1100;
  newConfig.ConvertionTime = B0;
  newConfig.Latch = B1;
  newConfig.ModeOfConversionOperation = B11;

  OPT3001_ErrorCode errorConfig = opt3001.writeConfig(newConfig);
}

void printResult(String text, OPT3001 result) {
  if (result.error == NO_ERROR) {
    Serial.print(text);
    Serial.print(": ");
    Serial.print(result.lux);
    Serial.println(" lux");
  }
  else {
    printError(text,result.error);
  }
}

void printError(String text, OPT3001_ErrorCode error) {
  Serial.print(text);
  Serial.print(": [ERROR] Code #");
  Serial.println(error);
}