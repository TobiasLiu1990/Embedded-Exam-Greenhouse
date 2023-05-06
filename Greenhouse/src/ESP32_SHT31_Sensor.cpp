#include "ESP32_SHT31_Sensor.h"
#include <Arduino.h>

ESP32_SHT31_Sensor::ESP32_SHT31_Sensor() {
    sht31 = Adafruit_SHT31();
}

float ESP32_SHT31_Sensor::getTemperature() {
    return sht31.readTemperature() - getTemperatureCompensation();
}

float ESP32_SHT31_Sensor::getHumidity() {
    return sht31.readHumidity() - getHumidityCompensation();
}

// Value is a very simple calculation after running ESP32 and DHT11 indoor on a table for 15min each.
//  33.99 - 24.80 (ESP32 - DHT11)
float ESP32_SHT31_Sensor::getTemperatureCompensation() {
    return 9.19;
}

// Value is a very simple calculation after running ESP32 and DHT11 indoor on a table for 15min each.
//  15.89 - 11 (ESP32 - DHT11)
float ESP32_SHT31_Sensor::getHumidityCompensation() {
    return 4.89;
}

// Value is taken from an article for measuing temperature difference from floor - bench level (~1m) - ceiling (~9.8m)
//  1.9 - 0.7. In reality this might not be a good solution by hardcoding the values.
// But could be used here as an example to not go too far past the ideal high temperature.
float ESP32_SHT31_Sensor::getUpperTemperatureMargin(float idealHighTemp) {
    float margin = idealHighTemp - 1.2;
    return margin;
}

float ESP32_SHT31_Sensor::getLowerTemperatureMargin(float idealLowTemp) {
    float margin = idealLowTemp + 1.2;
    return margin;
}

bool ESP32_SHT31_Sensor::validateNumberReading(float readings) {
    if (!isnan(readings)) {
        return true;
    } else {
        return false;
    }
}

bool ESP32_SHT31_Sensor::checkSensorSht31() {
    if (!sht31.begin(0x44)) { // default i2c address
        return true;
        while (1)
            yield();
    }
    return false;
}