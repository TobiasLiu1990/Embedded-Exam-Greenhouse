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
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "Adafruit_LTR329_LTR303.h"
#include "Adafruit_SHT31.h"
#include <Adafruit_DotStar.h>
#include <Wire.h>
#include <i2cdetect.h>

//Blynk
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
//Blynk

const char *ssid = "Venti_2.4G";
const char *password = "NikitaBoy";
const String endpoint = "https://api.openweathermap.org/data/2.5/weather?q=Oslo,no&APPID=";
const String key = "6759feb4f31aad1b1ace05f93cc6824f";
const String metric = "&units=metric";

StaticJsonDocument<1024> doc;
bool isConnected = false;

//Blynk info
#define BLYNK_TEMPLATE_ID "TMPL4SP7dMP-c"
#define BLYNK_TEMPLATE_NAME "Greenhouse"
#define BLYNK_AUTH_TOKEN "90qZjiZBUZwmDULPB9tlfsDqq3fXuoZf"

#define BLYNK_PRINT Serial

char auth[] = BLYNK_AUTH_TOKEN;     //Blynk token
char ssidBlynk[] = "Venti_2.4G";         //wifi
char pass[] = "NikitaBoy";          //wifi pw

//Each Blynk timer can run up to 16 instances.
BlynkTimer timer;

unsigned long previousMillis = 0;
const long waitInterval = 1500;


// Forward declarations
//
void GetWeatherInfo();
void WeatherInfoToSerial(JsonObject weatherInfo, JsonObject mainInfo);


// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0) {
    int value = param.asInt();  //Save incoming value from virtual pin V0
    digitalWrite(LED_BUILTIN, value);
}

BLYNK_CONNECTED()
{
  // Change Web Link Button message to "Congratulations!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl",  "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

// This function sends Arduino's uptime every second to Virtual Pin 2.
void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V1, millis() / 1000);
}
//
//Forward declarations


void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    delay(2000);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to Wifi...");
        isConnected = false;
    }
    Serial.println("Connected to the Wifi network");
    isConnected = true;
    Serial.println(WiFi.localIP());

    Serial.println("Now connected to WiFi");

    if (isConnected) {
        Blynk.begin(BLYNK_AUTH_TOKEN, ssidBlynk, pass);
        Serial.println("Now connected to Blynk Greenhouse!!!");
    }

    timer.setInterval(5000L, GetWeatherInfo);       // Openweathermap.org API for weather info
    timer.setInterval(1000L, myTimerEvent);
}

//Delay timer but with millis. Runs every 1500ms.
void Countdown() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= waitInterval) {
        //When 1500ms pass, do something in here

        //
        previousMillis = currentMillis;
    }
}


void loop() {
    Blynk.run();
    timer.run();

    // Openweathermap.org API for weather info
    //GetWeatherInfo();
}

void GetWeatherInfo() {
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
            JsonObject weather_0 = doc["weather"][0];
            JsonObject mainInfo = doc["main"];
            WeatherInfoToSerial(weather_0, mainInfo);
        } else {
            Serial.println("Error on HTTP request");
        }
        http.end();
    }
}

// Temp method for printing info to Serial
void WeatherInfoToSerial(JsonObject weatherInfo, JsonObject mainInfo) {
    const char *weatherDescription = weatherInfo["description"];

    float mainInfo_temp = mainInfo["temp"];
    float mainInfo_minTemp = mainInfo["temp_min"];
    float mainInfo_maxTemp = mainInfo["temp_max"];
    int mainInfo_humidity = mainInfo["humidity"];

    Serial.println("Weather status: ");
    Serial.print("Current weather: ");
    Serial.println(weatherDescription);

    Serial.print("Temperature: ");
    Serial.println(mainInfo_temp);

    Serial.print("Lowest temperature today: ");
    Serial.println(mainInfo_minTemp);

    Serial.print("Highest temperature today: ");
    Serial.println(mainInfo_maxTemp);

    Serial.print("Humidity %: ");
    Serial.println(mainInfo_humidity);
}



void GetTime() {
    // currentDate = new Date();
}
