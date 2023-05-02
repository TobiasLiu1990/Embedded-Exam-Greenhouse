#include "ESP32_SHT31_Sensor.h"
#include <Arduino.h>

float ESP32_SHT31_Sensor::readTemperature() {
    // return temperature = sht31.readTemperature() - getTemperatureCompensation();
    return temperature = sht31.readTemperature();
}

float ESP32_SHT31_Sensor::readHumidity() {
    return humidity = sht31.readHumidity() - getHumidityCompensation();
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
float ESP32_SHT31_Sensor::getUpperTemperatureMargin() {
    return 1.2;
}

// This value is measued by the average of (ideal + ideal lower)/2 so the window does not close when temperature go too far down.
// One article reported ideal to be 27C. Ideal lower temp used here is 20.25C.
float ESP32_SHT31_Sensor::getLowerTemperatureMargin() {
    return 23.625;
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
