#include "SensorInfo.h"
#include <Arduino.h>

void SensorInfo::integratedSensorReadings(bool temperatureReady, bool humidityReady) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisSensor >= waitIntervalSensor) {
        if (readTemperature()) {
            // UploadTemperatureToBlynk();
            temperatureReady = true;
        }
        if (readHumidity()) {
            // UploadHumidityToBlynk();
            humidityReady = true;
        }
        previousMillisSensor = currentMillis;
    }
}

bool SensorInfo::readTemperature() {
    temperature = sht31.readTemperature() - temperatureDifference;

    if (!isnan(temperature)) {
        Serial.print(F("Temperature: "));
        Serial.println(temperature);
        return true;
    } else {
        Serial.println(F("Error, cannot read temperature "));
        return false;
    }
}

bool SensorInfo::readHumidity() {
    humidity = sht31.readHumidity() - humidityDifference;

    if (!isnan(humidity)) {
        Serial.print(F("Humidity: "));
        Serial.println(humidity);
        return true;
    } else {
        Serial.println(F("Error, cannot read Humidity"));
        return false;
    }
}

String SensorInfo::errorCheckingSensors() {
    String checkSensors = "";

    if (!sht31.begin(0x44)) { // default i2c address
        checkSensors = "SHT31 - Cannot find sensor";
        //UpdateBlynkWidgetLabel(V1, "Temperature sensor error");
        while (1)
            yield();
    }
    return checkSensors;
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