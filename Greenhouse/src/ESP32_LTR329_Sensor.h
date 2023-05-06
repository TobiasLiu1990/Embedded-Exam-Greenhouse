#include "Adafruit_LTR329_LTR303.h" //Light sensor. 16bit light (infrared + visible + IR spectrum) 0 - 65k lux.
#include <Arduino.h>

class ESP32_LTR329_Sensor {
private:
    uint16_t visibleAndIr;
    uint16_t infrared;
    uint gainCalc;
    float integTimeCalc;

public:
    Adafruit_LTR329 ltr329;

    ESP32_LTR329_Sensor();

    unsigned int getFromLightSensor();
    bool checkSensorLtr329();
    void setGainForLuxCalculation(uint fruitGainCalc);
    void setGainCalc(uint newGainCalc);
    void setIntegTimeCalc(float newIntegTimeCalc);
};