#include "Fruit.h"
#include <Arduino.h>

Fruit::Fruit() {
    // For empty constructor
}

Fruit::Fruit(float minTemp, float maxTemp, float idealLowTemp, float idealHighTemp, float idealLowHumidity, float idealHighHumidity) {
    this->minTemp = minTemp;
    this->maxTemp = maxTemp;
    this->idealLowTemp = idealLowTemp;
    this->idealHighTemp = idealHighTemp;
    this->idealLowHumidity = idealLowHumidity;
    this->idealHighHumidity = idealHighHumidity;
}

float Fruit::getMinTemp() {
    return minTemp;
}

float Fruit::getMaxTemp() {
    return maxTemp;
}

float Fruit::getIdealLowTemp() {
    return idealLowTemp;
}

float Fruit::getIdealHighTemp() {
    return idealHighTemp;
}

float Fruit::getIdealLowHumidity() {
    return idealLowHumidity;
}

float Fruit::getIdealHighHumidity() {
    return idealHighHumidity;
}

float Fruit::getLowerTemperatureMargin() {
    return lowerTemperatureMargin;
}

void Fruit::setLowerTemperatureMargin(float newLowerMargin) {
    this->lowerTemperatureMargin = newLowerMargin;
}

float Fruit::getUpperTemperatureMargin() {
    return upperTemperatureMargin;
}

void Fruit::setUpperTemperatureMargin(float newUpperMargin) {
    this->upperTemperatureMargin = newUpperMargin;
}
