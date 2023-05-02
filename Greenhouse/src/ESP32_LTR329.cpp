#include "ESP32_LTR329.h"
#include <Arduino.h>

uint16_t visibleAndIr; // Should maybe only measure visible for plants?
uint16_t infrared;     // IR produces heat but does not help photosynthesis...
void ESP32_LTR329::getLightSensorInfo() {
    if (ltr329.newDataAvailable()) {
        bool data = ltr329.readBothChannels(visibleAndIr, infrared); // 1st param = ch0, 2nd param = ch1. Reads both 16-bit channels at once. Put data in argument pointers.
        if (data) {
            String ch0 = String(visibleAndIr);
            String ch1 = String(infrared);
            String lightInformation = "Visible and IR: " + ch0 + ". IR: " + ch1;

            Blynk.virtualWrite(V3, lightInformation);
        }
    }
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