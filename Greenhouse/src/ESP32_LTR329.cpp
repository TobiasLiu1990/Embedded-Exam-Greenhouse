#include "ESP32_LTR329.h"
#include <Arduino.h>

/*
    LTR329 Appendix A on Lux-formula from github source:
    https://github.com/sensebox/SenseBoxMCU-Lib/blob/master/SenseBoxMCU.cpp

    Corrected solution
    https://github.com/sensebox/node-sketch-templater/issues/49
*/

unsigned int ESP32_LTR329::getFromLightSensor(unsigned int lux) {
    if (ltr329.newDataAvailable()) {
        // 1st param = ch0
        // 2nd param = ch1. Reads both 16-bit channels at once
        bool data = ltr329.readBothChannels(visibleAndIr, infrared);

        if (data) {
            if (visibleAndIr >= 65535 || infrared >= 65535) { // Saturated sensors
                lux = 0;
                return lux;
            }

            if (visibleAndIr == 0 || infrared == 0) {
                lux = 0;
                return lux;
            }

            double ratio = infrared / (visibleAndIr + infrared);

            if (ratio < 0.45) {
                lux = ((1.7743 * visibleAndIr) + (1.1059 * infrared)) / ltr329.getGain() / ltr329.getIntegrationTime();
            } else if (ratio < 0.64) {
                lux = ((4.2785 * visibleAndIr) + (1.9548 * infrared)) / ltr329.getGain() / ltr329.getIntegrationTime();
            } else if (ratio < 0.85) {
                lux = ((0.5926 * visibleAndIr) + (0.1185 * infrared)) / ltr329.getGain() / ltr329.getIntegrationTime();
            } else {
                lux = 0;
            }

            String ch0 = String(visibleAndIr);
            String ch1 = String(infrared);
            String lightInformation = "Visible and IR: " + ch0 + ". IR: " + ch1;

            // Blynk.virtualWrite(V3, lightInformation);
        }
    }
    return lux;
}

bool ESP32_LTR329::checkSensorLtr329() {
    if (!ltr329.begin()) {
        Serial.println("LTR329 - Cannot find sensor");
        return true;
        while (1)
            yield();
    }
    return false;
}