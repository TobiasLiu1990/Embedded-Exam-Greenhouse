#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

class ConnectOpenWeathermap {
private:
    String endpoint;
    String key;
    String metric;
    HTTPClient http;
    StaticJsonDocument<1024> doc;
    JsonObject weather_0;
    JsonObject mainInfo;

public:
    ConnectOpenWeathermap(String endpoint, String key, String metric);

    void connect();
    void disconnect();
    String getWeatherInfo();
};