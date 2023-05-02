#include "ESP32_LTR329_Sensor.h"
#include <Arduino.h>

/*
    LTR329 Appendix A on Lux-formula from github source:
    https://github.com/sensebox/SenseBoxMCU-Lib/blob/master/SenseBoxMCU.cpp

    Corrected solution
    https://github.com/sensebox/node-sketch-templater/issues/49
*/

unsigned int ESP32_LTR329_Sensor::getFromLightSensor(unsigned int lux) {
    if (ltr329.newDataAvailable()) {
        // 1st param = ch0
        // 2nd param = ch1. Reads both 16-bit channels at once
        bool data = ltr329.readBothChannels(visibleAndIr, infrared);
        if (data) {
            if (visibleAndIr > 65535 || infrared > 65535) { // Saturated sensors
                return 0;
            }
            if (visibleAndIr == 0 || infrared == 0) {
                return 0;
            }

            double ratio = infrared / (visibleAndIr + infrared);
            Serial.println();
            Serial.print("ch0: ");
            Serial.println(visibleAndIr);
            Serial.print("ch1: ");
            Serial.print(infrared);
            Serial.println();
            Serial.print("gainCalc: ");
            Serial.println(gainCalc);
            Serial.print("integCalc: ");
            Serial.println(integTimeCalc);

            if (ratio < 0.45) {
                Serial.println("ratio 1");
                //lux = ((1.7743 * visibleAndIr) + (1.1059 * infrared)) / gainCalc / integTimeCalc;     // gain 4 (banana)
                lux = ((1.7743 * visibleAndIr) + (1.1059 * infrared)) / 1 / integTimeCalc;       // gain 1 (ananas)
            } else if (ratio < 0.64 && ratio >= 0.45) {
                Serial.println("ratio 2");
                lux = ((4.2785 * visibleAndIr) - (1.9548 * infrared)) / gainCalc / integTimeCalc;
            } else if (ratio < 0.85 && ratio >= 0.64) {
                Serial.println("ratio 3");
                lux = ((0.5926 * visibleAndIr) + (0.1185 * infrared)) / gainCalc / integTimeCalc;
            } else {
                lux = 0;
            }
        }
    }
    return lux;
}

void ESP32_LTR329_Sensor::setGainForLuxCalculation(uint calc) {
    gainCalc = calc;
}



bool ESP32_LTR329_Sensor::checkSensorLtr329() {
    if (!ltr329.begin()) {
        Serial.println("LTR329 - Cannot find sensor");
        return true;
        while (1)
            yield();
    }
    return false;
}