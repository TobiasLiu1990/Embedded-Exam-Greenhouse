#include "ConnectOpenWeathermap.h"
#include <Arduino.h>
#include <ArduinoJson.h>

/*
  Uses Openweathermap.org for weather information. 60 calls per hour (free version).
  https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.h#L36 - ERROR CODES
*/

ConnectOpenWeathermap::ConnectOpenWeathermap(String endpoint, String key, String metric) {
    this->endpoint = endpoint;
    this->key = key;
    this->metric = metric;
}

void ConnectOpenWeathermap::connect() {
    http.begin(endpoint + key + metric);
    int httpCode = http.GET();

    if (httpCode > 0) { // Code > 0 are standard HTTP return codes
        String payload = http.getString();
        DeserializationError error = deserializeJson(doc, payload); // parses a JSON input and puts the result in a JsonDocument.

        if (error) {
            Serial.print(F("deserializeJson() failed"));
            Serial.println(error.f_str()); // Same as c_str(), except the string is in Flash memory (only relevant for AVR and ESP8266)
        }

        weather_0 = doc["weather"][0];
        mainInfo = doc["main"];
    } else {
        Serial.print("Error on HTTP request: ");
        Serial.println(httpCode);
    }
}

void ConnectOpenWeathermap::disconnect() {
    http.end();
}

String ConnectOpenWeathermap::getWeatherInfo() {
    const char *weatherDescription = weather_0["description"];
    float mainInfo_temp = mainInfo["temp"];

    return String(weatherDescription) + ". " + mainInfo_temp + "C";
}

