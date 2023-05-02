#include "Adafruit_SHT31.h" //Temperature and humidity sensor
#include <Arduino.h>

class ESP32IntegratedSensor {
public:
    Adafruit_SHT31 sht31;
    float temperature;
    float humidity;

    ESP32IntegratedSensor() {
        sht31 = Adafruit_SHT31();
        temperature = 0;
        humidity = 0;
    }

    float readTemperature();
    float readHumidity();
    float getTemperatureCompensation();
    float getHumidityCompensation();
    float getUpperTemperatureMargin();
    float getLowerTemperatureMargin();

    bool validateNumberReading(float readings);
    bool checkSensorSht31();
};
