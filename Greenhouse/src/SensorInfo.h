#include "Adafruit_SHT31.h" //Temperature and humidity sensor
#include <Arduino.h>

class SensorInfo {
private:
    Adafruit_SHT31 sht31;
    float temperature;
    float humidity;
    float temperatureDifference;        //const later
    float humidityDifference;           //const later
    long waitIntervalSensor;            //const later
    unsigned long previousMillisSensor;

public:
    SensorInfo() {
        sht31 = Adafruit_SHT31();
        temperature = 0;
        humidity = 0;
        temperatureDifference = 33.99 - 24.40;
        humidityDifference = 15.89 - 11;
        waitIntervalSensor = 5000;
        previousMillisSensor = 0;
    }

    void integratedSensorReadings(bool temperatureReady, bool humidityReady);
    bool readTemperature();
    bool readHumidity();
    String errorCheckingSensors();
};
