/*
  Uses Openweathermap.org for weather information. Updates every 1h (free version).
  https://stackoverflow.com/questions/46111834/format-curly-braces-on-same-line-in-c-vscode - For changing auto format behaviour

    Other components:
    Fan (act as vent) - ventilation on/off depending on temp

    Date/time with RTC when:
      Update greenhouse information every 10mins.
      Openweathermap updates
      If something goes wrong

    Other info i want to show from OpenWeathermap.org
      Current weather
      curent temp

    Maybe also use Air Pollution API


    Ref:
    https://learn.adafruit.com/adafruit-sht31-d-temperature-and-humidity-sensor-breakout/wiring-and-test
    https://adafruit.github.io/Adafruit_LTR329_LTR303/html/class_adafruit___l_t_r329.html

*/

/** Placeholder - information on greenhouse
 *      * https://www.growspan.com/news/understanding-greenhouse-lighting/#:~:text=Greenhouses%20generally%20require%20six%20hours,promote%20crop%20growth%20and%20yield
 *          - 6h of direkt or full spectrum light each day in a greenhouse is good.
 *
 *      * https://www.mrhouseplant.com/blog/what-is-bright-indirect-light-for-plants/#:~:text=Most%20house%20plants%20need%20bright,will%20be%20happier%20and%20healthier
 *          - Bright indirect light or indirect light is > 3000 lux
 *          - Good for indoor plants.
 *          - Stronger indirect light over 10000 lux - 15000 lux will increase photosynthesis, speed up growth. Better.
 *
 *
 *
 *
 *      * https://www.mrhouseplant.com/blog/caring-for-a-pineapple-plant-101-ananas-comosus-tips-tricks/    (NOT GREENHOUSE)
 *          - Pineapple  min 10000 lux - max 40000 lux *
 *          - in bright indirect light (3000 lux), they can grow but very slow. High chance to not produce fruit.
 *
 *      * https://www.thespruce.com/how-to-grow-a-pineapple-7091045
 *          - Humidity 40% - 60%
 *
 *      * https://wintergardenz.co.nz/growing-advice-and-tips/growing-pineapples-in-a-greenhouse/
 *          - Sensitive to temp, for every 1C above or below ideal can damage growth by 6%.
 *          - optimal temp 20 - 32 (night to day)
 *          - Max = 35C and above can lead to sunburn damage
 *
 *      * https://www.plantlexicon.com/pineapple/#Temperature   (optimal growth seems to require a lot of different temps in each phase of the growth...)
 *          - min temp 10C
 *          - ideal from 23C - 30C
 *
 *
 *
 *
 *
 *
 *
 *      * https://solarinnovations.com/news/blog/banana-tree-growing-tips/#:~:text=Banana%20trees%20are%20a%20must,they%20take%20up%20less%20space
 *          - 12h sun per day
 *          - 19.4 C at night
 *          - 29.4 C during day
 *          - >= humidity 50%      http://www.agritech.tnau.ac.in/expert_system/banana/cli.html#:~:text=A%20humidity%20of%20at%20least,will%20damage%20the%20banana%20leaves.
 *
 *      * https://www.rhs.org.uk/plants/banana/growing-guide
 *          - min 15C
 *          - ideal 27C
 *
 *      * https://www.gardenguides.com/75976-grow-bananas-greenhouse.html
 *          - ideal: 21.1 - 26.6
 */

/*
    General role for a greenhouse seems to be that 6h of direkt light per day is good.
    Bright indirect aka. indirect light is over 3000lux.

    Pineapples and bananas has different ideal conditions. Se info above.
*/

#include <Adafruit_DotStar.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <i2cdetect.h>

// Sensors
#include "Adafruit_LTR329_LTR303.h" //Light sensor. 16bit light (infrared + visible + IR spectrum) 0 - 65k lux.
#include "Adafruit_SHT31.h"         //Temperature and humidity sensor
// Later maybe add accelerometer to check if it has been flipped (for light sensor)

// Blynk
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <WiFiClient.h>

// Openweathermap
const char *ssid = "Venti_2.4G";
const char *password = "NikitaBoy";
const String endpoint = "https://api.openweathermap.org/data/2.5/weather?q=Oslo,no&APPID=";
const String key = "6759feb4f31aad1b1ace05f93cc6824f";
const String metric = "&units=metric";

// Json
StaticJsonDocument<1024> doc;

// Blynk info
#define BLYNK_TEMPLATE_ID "TMPL4SP7dMP-c"
#define BLYNK_TEMPLATE_NAME "Greenhouse"
#define BLYNK_AUTH_TOKEN "90qZjiZBUZwmDULPB9tlfsDqq3fXuoZf"
#define BLYNK_PRINT Serial

char auth[] = BLYNK_AUTH_TOKEN;  // Blynk token
char ssidBlynk[] = "Venti_2.4G"; // wifi
char pass[] = "NikitaBoy";       // wifi pw

BlynkTimer timer; // Each Blynk timer can run up to 16 instances.
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_LTR329 ltr329 = Adafruit_LTR329();

ltr329_gain_t pineappleGain = LTR3XX_GAIN_1; // GAIN_1 = 1 lux to 64k     (less accuracy but reaches pineapples max of 40k lux measurement)
ltr329_integrationtime_t pineappleIntegTime = LTR3XX_INTEGTIME_100;
ltr329_measurerate_t pineappleMeasurementRate = LTR3XX_MEASRATE_100;

ltr329_gain_t bananaGain = LTR3XX_GAIN_4; // GAIN_4 = 0.25 lux - 16k lux
ltr329_integrationtime_t bananaIntegTime = LTR3XX_INTEGTIME_200;
ltr329_measurerate_t bananaMeasurementRate = LTR3XX_MEASRATE_200;

float temperature;
float humidity;

bool isConnected = false;
unsigned long previousMillis = 0;
const long waitInterval = 1500;

// Using this for FSM
enum Fruits {
    Banana,
    Pineapple
};

Fruits currentState; // Will be set to Bananas by default in Setup()
int stateNumber = 0; // 0 will default to Bananas
bool isStateChanged = false;

float minTemp;
float maxTemp;
float idealLowTemp;
float idealHighTemp;
float idealLowHumidity;
float idealHighHumidity;
String BlynkGreenhouseLabel = "";
String blynkGreenhouseString = "";

// Forward declarations
//
String ErrorCheckingSensors();
void GetWeatherInfo();
void WeatherInfoToSerial(JsonObject weatherInfo, JsonObject mainInfo);
void ReadTemperature();
void ReadHumidity();
void SetLightSensor();
void GetLightSensorInfo();
void FruitStateTransition();
void UpdateFruitStateConditions();
void CheckSensorData();
//
// Forward declarations

// Blynk
//
// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0) {
    stateNumber = param.asInt();
    isStateChanged = true;
}

BLYNK_CONNECTED() {
    // Change Web Link Button message to "Congratulations!"
    // Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
    // Blynk.setProperty(V3, "onImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
    // Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

// This function sends Arduino's uptime every second to Virtual Pin 2.
void UptimeCounter() {
    // You can send any value at any time.
    // Please don't send more that 10 values per second.
    Blynk.virtualWrite(V1, millis() / 1000);
}
//
// Blynk

void setup() {
    Serial.begin(115200);
    Wire.begin(3, 4); // i2c SDC, SCL
    pinMode(LED_BUILTIN, OUTPUT);
    currentState = Banana;
    delay(1000);

    ErrorCheckingSensors();
    delay(2000);

    WiFi.begin(ssid, password);
    delay(1000);

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

    // Blynk .setInterval can not take a function with arguments
    // timer.setInterval(5000L, GetWeatherInfo); // Openweathermap.org API for weather info
    timer.setInterval(1000L, UptimeCounter);
    timer.setInterval(5000L, ReadTemperature);
    timer.setInterval(5000L, ReadHumidity);
    timer.setInterval(5000L, SetLightSensor);
    timer.setInterval(2000L, CheckSensorData);
}

void loop() {
    Blynk.run();
    timer.run();

    if (isStateChanged) {
        FruitStateTransition();
        UpdateFruitStateConditions();
        isStateChanged = false;
    } else {
        CheckSensorData();
    }

    /*
        Checks a method that revceives info from Blynk:
            Check numbers for example
            0 -> Change state to Banana
              -> Set label to Banana Mode
              -> Settings should follow conditions for bananas (temps, humidity, light measuing for error checking etc. Fans etc.)
    */
}

void UpdateFruitStateConditions() {
    if (currentState == Banana) {
        Blynk.setProperty(V0, "label", "Current target: Bananas");
        // Ideal range 18.5 - 27.7 is the average from 3 refs
        minTemp = 15;
        maxTemp = 30;
        idealLowTemp = 18.5;
        idealHighTemp = 27.7;
        idealLowHumidity = 50;
        idealHighHumidity = 100;
    } else if (currentState == Pineapple) {
        Blynk.setProperty(V0, "label", "Current target: Pineapples");
        // Ideal range 21.5 - 31 is the average from 2 refs
        minTemp = 10;
        maxTemp = 35;
        idealLowTemp = 21.5;
        idealHighTemp = 31;
        idealLowHumidity = 40;
        idealHighHumidity = 60;
    }
}

void FruitStateTransition() {
    switch (stateNumber) {
    case 0:
        currentState = Banana;
        break;
    case 1:
        currentState = Pineapple;
        break;
    }
}


void CheckSensorData() {
    if (temperature >= idealLowTemp && temperature <= idealHighTemp) {
        BlynkGreenhouseLabel = "Temperature is within ideal range of: ";
        blynkGreenhouseString = String(idealLowTemp) + " - " + String(idealHighTemp);
    } else {
        BlynkGreenhouseLabel = "Temperature is at a dangerous level of: " + String(temperature) + "\n";
    }

    if (humidity >= idealLowHumidity && humidity <= idealHighHumidity) {
        BlynkGreenhouseLabel += "Humidity is within ideal range of" + String(idealLowHumidity) + " - " + String(idealHighHumidity);
    } else {
        BlynkGreenhouseLabel += "Humidity is at a dangerous level of: " + String(humidity) + "\n";
    }

    Blynk.setProperty(V8, "label", BlynkGreenhouseLabel);
    Blynk.virtualWrite(V8, "noooo");
}


void UpdateFanSettings() {
    //Show fan rpm in Blynk.
    //Try to get fan to run automatically depending on temperatures
    //Greenhouse - important with ventilation so cold/hot air does not get trapped

    //Also an override in Blynk to manually change temperature. Maybe need a bool for true/false for auto/manual.
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

String ErrorCheckingSensors() {
    String checkSensors = "";

    if (!sht31.begin(0x44)) { // default i2c address
        checkSensors = "SHT31 - Cannot find sensor";
        // Send some error to Blynk
        while (1)
            delay(1);
    }
    if (!ltr329.begin()) {
        checkSensors += "LTR329 - Cannot find sensor";
        // Send some error to Blynk
        while (1)
            delay(10);
    }

    if (checkSensors == "") {
        checkSensors = "Sensors found";
    }
    return checkSensors;
}

// Should write to Virtual Pin, V2 on Blynk
void ReadTemperature() {
    temperature = sht31.readTemperature();

    if (!isnan(temperature)) {
        Blynk.virtualWrite(V2, temperature);
    } else {
        // error! send error message to blynk
    }
}

// Should write to Virtual Pin, V3 on Blynk
void ReadHumidity() {
    humidity = sht31.readHumidity();

    if (!isnan(humidity)) {
        Blynk.virtualWrite(V3, humidity);
    } else {
        // error! send error message to blynk
    }
}

void SetLightSensor() {
    ltr329.setGain(LTR3XX_GAIN_4);
    ltr329.setIntegrationTime(LTR3XX_INTEGTIME_200); // Amount of time available to obtain a measurement during which there is essentially no change in the level of the signal
    ltr329.setMeasurementRate(LTR3XX_MEASRATE_200);  // Number of measurement values generated per second

    GetLightSensorInfo();
}

uint16_t visibleAndIr; // Should maybe only measure visible for plants?
uint16_t infrared;     // IR produces heat but does not help photosynthesis...
void GetLightSensorInfo() {
    if (ltr329.newDataAvailable()) {
        bool data = ltr329.readBothChannels(visibleAndIr, infrared); // 1st param = ch0, 2nd param = ch1. Reads both 16-bit channels at once. Put data in argument pointers.
        if (data) {
            String ch0 = String(visibleAndIr);
            String ch1 = String(infrared);
            String lightInformation = "Visible and IR: " + ch0 + ". IR: " + ch1;

            Blynk.virtualWrite(V7, lightInformation);
        }
    }
}

void GetTime() {
    // currentDate = new Date();
}

// Delay timer but with millis. Runs every 1500ms.
void TimerMillis() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= waitInterval) {
        // When 1500ms pass, do something in here

        //
        previousMillis = currentMillis;
    }
}

//----------QUESTIONS---------------
// Check if ok to borrow error code jenschr:

/*
 * varför yield()?
 * och varför delay efter?
 *
 * ltr readbothchannels. Vad är det man får? Lux eller nm för våglängd av ljus?
 *
 */

/*
  if (! lis.begin(0x18)) {
    Serial.println("Couldn't find LIS3DH sensor!");
    while (1) yield();
  }
  Serial.println("Found LIS3DH sensor!");

  if (! sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31 sensor!");
    while (1) delay(1);
  }
  Serial.println("Found SHT31 sensor!");

  if ( ! ltr.begin() ) {
    Serial.println("Couldn't find LTR sensor!");
    while (1) delay(10);
  }

*/