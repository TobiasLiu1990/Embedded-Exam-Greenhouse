#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

class ConnectOpenWeathermap {
public:
    String ssid;
    String password;
    String endpoint;
    String key;
    String metric;
    StaticJsonDocument<1024> doc;
    JsonObject weather_0;
    JsonObject mainInfo;

    ConnectOpenWeathermap(String ssid, String password, String endpoint, String key, String metric) {
        this->ssid = ssid;
        this->password = password;
        this->endpoint = endpoint;
        this->key = key;
        this->metric = metric;
    }

    void connectToOpenWeatherMap();
    void disconnectToOpenWeatherMap(HTTPClient http);

    char getJsonWeatherStatusData();
    float getJsonWeatherTemperatureData();

    void showCurrentWeather();
    String getWeatherInfo();
};