#include "ESP32IntegratedSensor.h"
#include <Arduino.h>

float ESP32IntegratedSensor::readTemperature() {
    return temperature = sht31.readTemperature() - getTemperatureCompensation();
}

float ESP32IntegratedSensor::readHumidity() {
    return humidity = sht31.readHumidity() - getHumidityCompensation();
}


bool ESP32IntegratedSensor::validateNumberReading(float readings) {
    if (!isnan(readings)) {
        return true;
    } else {
        return false;
    }
}

bool ESP32IntegratedSensor::errorCheckTemperatureSensor() {
    if (!sht31.begin(0x44)) {       //default i2c address
        return true;
        while (1)
            yield();
    }
    return false;
}

// Value is a very simple calculation after running ESP32 and DHT11 indoor on a table for 15min each.
//  33.99 - 24.80 (ESP32 - DHT11)
float ESP32IntegratedSensor::getTemperatureCompensation() {
    return 9.19;
}

// Value is a very simple calculation after running ESP32 and DHT11 indoor on a table for 15min each.
//  15.89 - 11 (ESP32 - DHT11)
float ESP32IntegratedSensor::getHumidityCompensation() {
    return 4.89;
}




/*
MOVE TO LIGHT SENSOR CLASS LATER


    if (!ltr329.begin()) {
        checkSensors += "LTR329 - Cannot find sensor";
        //UpdateBlynkWidgetLabel(V2, "Humidity sensor error");
        while (1)
            yield();
    }

*/