#include <Arduino.h>

class Fruit {
public:
    float minTemp;
    float maxTemp;
    float idealLowTemp;
    float idealHighTemp;
    float idealLowHumidity;
    float idealHighHumidity;

    float upperTemperatureMargin;
    float lowerTemperatureMargin;

    Fruit();
    
    Fruit(float minTemp, float maxTemp, float idealLowTemp, float idealHighTemp, float idealLowHumidity, float idealHighHumidity) {
        this->minTemp = minTemp;
        this->maxTemp = maxTemp;
        this->idealLowTemp = idealLowTemp;
        this->idealHighTemp = idealHighTemp;
        this->idealLowHumidity = idealLowHumidity;
        this->idealHighHumidity = idealHighHumidity;
    }
};