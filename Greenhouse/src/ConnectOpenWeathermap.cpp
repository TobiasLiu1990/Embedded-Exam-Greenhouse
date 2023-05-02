#include "ConnectOpenWeathermap.h"
#include <Arduino.h>
#include <ArduinoJson.h>


/*
  Uses Openweathermap.org for weather information. 60 calls per h (free version).
*/



void ConnectOpenWeathermap::connectToOpenWeatherMap() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        http.begin(endpoint + key + metric);
        int httpCode = http.GET();

        // https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.h#L36
        if (httpCode > 0) { // Code > 0 are standard HTTP return codes
            String payload = http.getString();

            DeserializationError error = deserializeJson(doc, payload); // parses a JSON input and puts the result in a JsonDocument.
            if (error) {
                Serial.print(F("deserializeJson() failed"));
                Serial.println(error.f_str()); // Same as c_str(), except the string is in Flash memory (only relevant for AVR and ESP8266)
            }

            // Json
            weather_0 = doc["weather"][0];
            mainInfo = doc["main"];
        } else {
            Serial.print("Error on HTTP request: ");
            Serial.println(httpCode);
        }
        http.end();
    }
}

void ConnectOpenWeathermap::disconnectToOpenWeatherMap(HTTPClient http) {
    http.end();
}

char ConnectOpenWeathermap::getJsonWeatherStatusData() {
    const char *weatherDesc = weather_0["description"];
    return *weatherDesc;
}

float ConnectOpenWeathermap::getJsonWeatherTemperatureData() {
    float mainInfo_temp =mainInfo["temp"];
    return mainInfo_temp;
}

/*
String ConnectOpenWeathermap::getWeatherInfo() {
    const char *weatherDescription = weather_0["description"];
    float mainInfo_temp = mainInfo["temp"];

    return String(weatherDescription) + ". " + mainInfo_temp + "C";
}
*/