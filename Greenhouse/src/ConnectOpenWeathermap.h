#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

class ConnectOpenWeathermap {
public:
    String endpoint;
    String key;
    String metric;
    HTTPClient http;
    StaticJsonDocument<1024> doc;
    JsonObject weather_0;
    JsonObject mainInfo;

    ConnectOpenWeathermap(String endpoint, String key, String metric) {
        this->endpoint = endpoint;
        this->key = key;
        this->metric = metric;
    }

    void connectToOpenWeatherMap();
    void disconnectToOpenWeatherMap();

    void showCurrentWeather();
    String getWeatherInfo();
};