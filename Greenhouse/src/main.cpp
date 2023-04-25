/*
  Uses Openweathermap.org for weather information. Updates every 1h (free version).
  https://stackoverflow.com/questions/46111834/format-curly-braces-on-same-line-in-c-vscode - For changing auto format behaviour
  
    Information about greenhouse.

    I want:
    
    Condition info inside green house:
      Temp + Humidity (SHT-31)        (Important to control temp/humidity)
      Lux (LTR329ALS)

    Other components:
    Fan (act as vent) - ventilation on/off depending on temp
    Warning light (DotStar?) - if something goes wrong

    Date/time with RTC when:
      Update greenhouse information every 10mins.
      Openweathermap updates
      If something goes wrong

    Other info i want to show from OpenWeathermap.org
      Current weather
      curent temp
      min temp
      max temp
      humidity %
      Sunrise
      Sunset

    Maybe also use Air Pollution API
    Maybe use TFT screen (not likely as i want the info to be posted somewhere that can be accessed on ex. phone)


    POST greenhouse info to somewhere.
*/

#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <i2cdetect.h>
#include <Adafruit_DotStar.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_LTR329_LTR303.h"

const char* ssid = "Venti_2.4G";
const char* password = "NikitaBoy";
const String endpoint = "https://api.openweathermap.org/data/2.5/weather?q=Oslo,no&APPID=";
const String key = "6759feb4f31aad1b1ace05f93cc6824f";
const String metric = "&units=metric";

StaticJsonDocument<1024> doc;

void setup() {
  Serial.begin(115200);
  delay(2000);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wifi...");
  }
  Serial.println("Connected to the Wifi network");
  Serial.println(WiFi.localIP());

}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(endpoint + key + metric);
    int httpCode = http.GET();

    // https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.h#L36
    if (httpCode > 0) {     //Code > 0 are standard HTTP return codes
      String payload = http.getString();

      DeserializationError error = deserializeJson(doc, payload);     // parses a JSON input and puts the result in a JsonDocument.
      if (error) {
        Serial.print(F("deserializeJson() failed"));
        Serial.println(error.f_str());                //Same as c_str(), except the string is in Flash memory (only relevant for AVR and ESP8266)
      }

      //Json
      JsonObject weather_0 = doc["weather"][0];
      const char* weather_0_desc = weather_0["description"];

      JsonObject mainInfo = doc["main"];
      float mainInfo_temp = mainInfo["temp"];
      float mainInfo_minTemp = mainInfo["temp_min"];
      float mainInfo_maxTemp = mainInfo["temp_max"];
      int mainInfo_humidity = mainInfo["humidity"];


      Serial.println("Weather status: ");
      Serial.print("Current weather: ");
      Serial.println(weather_0_desc);

      Serial.print("Temperature: ");
      Serial.println(mainInfo_temp);

      Serial.print("Lowest temperature today: ");
      Serial.println(mainInfo_minTemp);

      Serial.print("Highest temperature today: ");
      Serial.println(mainInfo_maxTemp);

      Serial.print("Humidity %: ");
      Serial.println(mainInfo_humidity);


    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  }
  delay(600000);

  Serial.println("test test");
}


void GetTime() {
  //currentDate = new Date();
}
