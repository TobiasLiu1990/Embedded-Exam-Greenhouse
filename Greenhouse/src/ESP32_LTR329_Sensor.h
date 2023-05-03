#include "Adafruit_LTR329_LTR303.h" //Light sensor. 16bit light (infrared + visible + IR spectrum) 0 - 65k lux.
#include <Arduino.h>

class ESP32_LTR329_Sensor {
public:
    Adafruit_LTR329 ltr329;
    uint16_t visibleAndIr;
    uint16_t infrared;

    unsigned int lux;
    uint gainCalc;
    float integTimeCalc;

    ESP32_LTR329_Sensor() {
        this->ltr329 = Adafruit_LTR329();
    }

    unsigned int getFromLightSensor();
    bool checkSensorLtr329();
    void setGainForLuxCalculation(uint fruitGainCalc);
    void setGainCalc(uint newGainCalc);
    void setIntegTimeCalc(float newIntegTimeCalc);
};