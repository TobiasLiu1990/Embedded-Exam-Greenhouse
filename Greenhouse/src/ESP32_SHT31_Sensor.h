#include "Adafruit_SHT31.h" //Temperature and humidity sensor
#include <Arduino.h>

class ESP32_SHT31_Sensor {
public:
    Adafruit_SHT31 sht31;

    ESP32_SHT31_Sensor() {};

    float getTemperature();
    float getHumidity();
    float getTemperatureCompensation();
    float getHumidityCompensation();
    float getUpperTemperatureMargin(float idealHighTemp);
    float getLowerTemperatureMargin(float idealLowTemp);

    bool validateNumberReading(float readings);
    bool checkSensorSht31();
};
