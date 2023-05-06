#include <Arduino.h>

class Fruit {
private:
    float minTemp;
    float maxTemp;
    float idealLowTemp;
    float idealHighTemp;
    float idealLowHumidity;
    float idealHighHumidity;

    float lowerTemperatureMargin;
    float upperTemperatureMargin;

public:
    Fruit() {};

    Fruit(float minTemp, float maxTemp, float idealLowTemp, float idealHighTemp, float idealLowHumidity, float idealHighHumidity);

    float getMinTemp();
    float getMaxTemp();
    float getIdealLowTemp();
    float getIdealHighTemp();
    float getIdealLowHumidity();
    float getIdealHighHumidity();

    void setLowerTemperatureMargin(float newLowerMargin);
    float getLowerTemperatureMargin();
    
    void setUpperTemperatureMargin(float newUpperMargin);
    float getUpperTemperatureMargin();
};