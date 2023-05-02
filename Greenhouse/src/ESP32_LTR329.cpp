#include "ESP32_LTR329.h"
#include <Arduino.h>

void ESP32_LTR329::setGain(ltr329_gain_t gain) {
    ltr329.setGain(gain);
}
