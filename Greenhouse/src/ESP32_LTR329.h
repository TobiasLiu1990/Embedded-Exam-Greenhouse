#include "Adafruit_LTR329_LTR303.h" //Light sensor. 16bit light (infrared + visible + IR spectrum) 0 - 65k lux.
#include <Arduino.h>

class ESP32_LTR329 {
public:
    Adafruit_LTR329 ltr329;
    ltr329_gain_t gain;

    ESP32_LTR329() {
        ltr329 = Adafruit_LTR329();
        ltr329.setIntegrationTime(LTR3XX_INTEGTIME_400);
        ltr329.setMeasurementRate(LTR3XX_MEASRATE_500);
    }

    void ESP32_LTR329::setGain(ltr329_gain_t gain);
};