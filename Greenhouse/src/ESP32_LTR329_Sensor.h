#include "Adafruit_LTR329_LTR303.h" //Light sensor. 16bit light (infrared + visible + IR spectrum) 0 - 65k lux.
#include <Arduino.h>

class ESP32_LTR329_Sensor {
public:
    Adafruit_LTR329 ltr329;
    ltr329_gain_t gain;
    uint16_t visibleAndIr;
    uint16_t infrared;
    uint gainCalc;
    float integTimeCalc;

    ESP32_LTR329_Sensor() {
        ltr329 = Adafruit_LTR329();
        integTimeCalc = 4.0;
    }

    unsigned int getFromLightSensor(unsigned int lux);
    bool checkSensorLtr329();
    void setGainForLuxCalculation(uint fruitGainCalc);
};