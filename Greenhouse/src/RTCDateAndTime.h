/*

#include <Arduino.h>
#include <RtcDS3231.h>
#include <Wire.h>

#define countof(arr) (sizeof(arr) / sizeof(arr[0])) // Macro to get number of elements in array

class RTCDateAndTime {
public:
    // Mostly error checking code (Default ones from the Rtc by Makuna library)
    bool wasError(const char *errorTopic = "", RtcDS3231<TwoWire> Rtc);
    void rtcErrorCheckingAndUpdatingDate();
    String printDateTime(const RtcDateTime &date, RtcDS3231<TwoWire> Rtc);
};

*/