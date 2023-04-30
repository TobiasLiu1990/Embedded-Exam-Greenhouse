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
 *      * https://www.dpi.nsw.gov.au/agriculture/horticulture/greenhouse/structures-and-technology/ventilation#:~:text=Fans%20are%20the%20key%20method,movement%20when%20venting%20is%20minimal.
 *          - ventilation
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



    Integrated sensor for temperature seems to get too hot.
    Uses DH11 now to check the temp.
        - Been running for about 10mins
        - Current room temperature with DH11: 24.40C
        - Current humidity: 10.00 - 11.00%

        * can do -40 to 80 (0.5C sensitivity)
        * 0 - 100% humidity (2-5% accuracy)


    Comparing to ESP32 integrated. After running for around 15min:
        - Temp: 33.88C
        - Humidity: 15.89%


    After more testing, sometimes the DHT11 would return weird values. Temperatures would sometimes be negative or much lower than actual temperature.
    For example -11C or around 10C lower than actual.
    It also failed to read temperature/humidity here and there, even if rarely, it would be an issue.

    Will use the above data to compensate and use the integrated sensors again.
    This wont be 100% accurate as the chip gets hotter, more compensation might be needed though.
*/

/*
    Components and stuff used:
        Blynk

        ESP32-S3
        DHT11 (Only used to compensate for ESP32 integrated sensors)
        RTC ZS-042 (DS3231)
        Stepper motor 28BYJ-48 5V DC + ULN2003AN

*/
#include <Adafruit_DotStar.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <RtcDS3231.h>
#include <Wire.h>
#include <i2cdetect.h>

// Sensors
#include "Adafruit_LTR329_LTR303.h" //Light sensor. 16bit light (infrared + visible + IR spectrum) 0 - 65k lux.
#include "Adafruit_SHT31.h"         //Temperature and humidity sensor
//  Later maybe add accelerometer to check if it has been flipped (for light sensor)

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

// Blynk info
#define BLYNK_TEMPLATE_ID "TMPL4SP7dMP-c"
#define BLYNK_TEMPLATE_NAME "Greenhouse"
#define BLYNK_AUTH_TOKEN "90qZjiZBUZwmDULPB9tlfsDqq3fXuoZf"
#define BLYNK_PRINT Serial

char auth[] = BLYNK_AUTH_TOKEN;  // Blynk token
char ssidBlynk[] = "Venti_2.4G"; // wifi
char pass[] = "NikitaBoy";       // wifi pw

StaticJsonDocument<1024> doc;
BlynkTimer timer; // Each Blynk timer can run up to 16 instances.

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_LTR329 ltr329 = Adafruit_LTR329();
RtcDS3231<TwoWire> Rtc(Wire);

ltr329_gain_t pineappleGain = LTR3XX_GAIN_1; // GAIN_1 = 1 lux to 64k     (less accuracy but reaches pineapples max of 40k lux measurement)
ltr329_integrationtime_t pineappleIntegTime = LTR3XX_INTEGTIME_100;
ltr329_measurerate_t pineappleMeasurementRate = LTR3XX_MEASRATE_100;

ltr329_gain_t bananaGain = LTR3XX_GAIN_4; // GAIN_4 = 0.25 lux - 16k lux
ltr329_integrationtime_t bananaIntegTime = LTR3XX_INTEGTIME_200;
ltr329_measurerate_t bananaMeasurementRate = LTR3XX_MEASRATE_200;

//---------------------L293D
// Might add fan for later use if possible
// #define enablePin1 D9
// #define pin1A D6

/*
  https://www.canr.msu.edu/news/temperature_variation_within_a_greenhouse

  Air temp floor  -   bench   -   ceiling levels    (NO FANS ON)
           0.7C  than bench (1m)
           1.9C              than ceiling (2.89m)

*/
/*Half-step setup
  360deg - 1 revolution      =  512 sequences
  180deg - 0.5 revolution    =  256 (compensated for rotational slop) -> 257
  90deg  - 0.25 revolution   =  128 (test if compensation needed)
  45deg  - 0.125 revolution  =  64
*/
// Stepper pins
const int motorPin1 = 8;  // Blue
const int motorPin2 = 9;  // Pink
const int motorPin3 = 10; // Yellow
const int motorPin4 = 11; // Orange

int motorSpeed = 5;
bool isWindowOpen = false;
const float tempDiff = 1.9 - 0.7;

//--------Wifi
bool isConnected = false;
unsigned long previousMillis = 0;
const long waitInterval = 10000;

// Using this for FSM
enum Fruits {
    Banana,
    Pineapple
};

Fruits currentState; // Will be set to Bananas by default in Setup()
int stateNumber;     // 0 will default to Bananas
bool isStateChanged = false;
bool runErrorHandlingOnce = true;

String todaysDateAndWeather = "";

const float temperatureDifference = 33.88 - 24.40;
const float humidityDifference = 15.89 - 11;
float temperature = 0;
float humidity = 0;
float minTemp;
float maxTemp;
float idealLowTemp;
float idealHighTemp;
float idealLowHumidity;
float idealHighHumidity;
String BlynkStatusWidgetMessage = "";
String BlynkStatusWidgetColor = ""; // https://htmlcolorcodes.com/colors/shades-of-green/

// Forward declarations
//
void TimerMillis();

void ConnectToOpenWeatherMap();
void ShowTodaysDateAndWeather();
void GetWeatherInfo();

String ErrorCheckingSensors();
void ReadTemperature();
void ReadHumidity();
void SetLightSensor();
void GetLightSensorInfo();
void CheckSensorData();
void CheckTemperatureData();
void CheckHumidityData();

void FruitStateTransition();
void UpdateFruitStateConditions();

void initBlynk();
void ResetBlynkWidget();
void UpdateBlynkWidgetColor(char vp, String color);
void UpdateBlynkWidgetContent(char vp, String message);
void UpdateBlynkWidgetLabel(char vp, String message);

String printDateTime(const RtcDateTime &date);
void RtcErrorCheckingAndUpdatingDate(); // Might need to fix a bit later. Currently copied from Rtc by Makuna example.
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
//
// Blynk

bool wasError(const char *errorTopic = "") {
    uint8_t error = Rtc.LastError();
    if (error != 0) {
        // we have a communications error
        // see https://www.arduino.cc/reference/en/language/functions/communication/wire/endtransmission/
        // for what the number means
        Serial.print("[");
        Serial.print(errorTopic);
        Serial.print("] WIRE communications error (");
        Serial.print(error);
        Serial.print(") : ");

        switch (error) {
        case Rtc_Wire_Error_None:
            Serial.println("(none?!)");
            break;
        case Rtc_Wire_Error_TxBufferOverflow:
            Serial.println("transmit buffer overflow");
            break;
        case Rtc_Wire_Error_NoAddressableDevice:
            Serial.println("no device responded");
            break;
        case Rtc_Wire_Error_UnsupportedRequest:
            Serial.println("device doesn't support request");
            break;
        case Rtc_Wire_Error_Unspecific:
            Serial.println("unspecified error");
            break;
        case Rtc_Wire_Error_CommunicationTimeout:
            Serial.println("communications timed out");
            break;
        }
        return true;
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    Wire.begin(3, 4); // i2c SDC, SCL
    Rtc.Begin();

    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);
    pinMode(motorPin3, OUTPUT);
    pinMode(motorPin4, OUTPUT);
    currentState = Banana;

    // Taken from Rtc by Makuna - DS3231_Simple example. Some minor changes to the error checking code.
    RtcErrorCheckingAndUpdatingDate();

    ErrorCheckingSensors();
    delay(1000);

    WiFi.begin(ssid, password);
    delay(1000);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1500);
        Serial.println("Connecting to Wifi...");
        isConnected = false;
    }
    Serial.println("Connected to the Wifi network");
    isConnected = true;

    if (isConnected) {
        Blynk.begin(BLYNK_AUTH_TOKEN, ssidBlynk, pass);
        Serial.println("Now connected to Blynk Greenhouse!");
        delay(100);
        initBlynk(); // Init Blynk with Banana as default
        delay(1000);
    }

    // Blynk .setInterval can not take a function with arguments
    // timer.setInterval(5000L, M); // Openweathermap.org API for weather info
    timer.setInterval(1000L, ShowTodaysDateAndWeather);
    timer.setInterval(5000L, ReadTemperature);
    timer.setInterval(5000L, ReadHumidity);
    timer.setInterval(5000L, SetLightSensor);
    timer.setInterval(2000L, CheckSensorData);
}

void loop() {
    Blynk.run();
    timer.run();

    if (isStateChanged) {
        Serial.println("Did i enter to change state variables????");
        FruitStateTransition();
        UpdateFruitStateConditions();
        isStateChanged = false;
    }

    if (isWindowOpen == false) {
        if (temperature > (idealHighTemp - tempDiff)) { // This check is so that the windows should open before the current temp at bench level gets to high.
            isWindowOpen = true;
            openWindow();
        }
    } else {
        if (temperature < idealLowTemp + 2) {
            isWindowOpen = false;
            closeWindow();
        }
    }
}

void initBlynk() {
    currentState = Banana; // Will be set to Bananas by default in Setup()
    stateNumber = 0;       // 0 will default to Bananas
    ResetBlynkWidget();
    UpdateFruitStateConditions();
}

void UpdateFruitStateConditions() {
    if (currentState == Banana) {
        // Ideal range 18.5 - 27.7 is the average from 3 refs
        UpdateBlynkWidgetLabel(V0, "Current target: Bananas");
        UpdateBlynkWidgetColor(V0, "#E6D22A"); // Yellow

        Serial.println("Changing to Banana variables");
        minTemp = 15;
        maxTemp = 30;
        idealLowTemp = 18.5;
        idealHighTemp = 27.7;
        idealLowHumidity = 50;
        idealHighHumidity = 100;
    } else if (currentState == Pineapple) {
        // Ideal range 21.5 - 31 is the average from 2 refs
        UpdateBlynkWidgetLabel(V0, "Current target: Pineapples");
        UpdateBlynkWidgetColor(V0, "#87DE24"); // Green-ish

        Serial.println("Changing to Pineapple variables");
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
        Serial.println("Banana state now");
        break;
    case 1:
        currentState = Pineapple;
        Serial.println("pineapple state now");
        break;
    }
}

void CheckSensorData() {
    CheckTemperatureData();
    CheckHumidityData();
}

void CheckTemperatureData() {
    //--Temperature
    if (temperature >= idealLowTemp && temperature <= idealHighTemp) { // Good
        BlynkStatusWidgetMessage = "Temperature is within ideal range (" + String(idealLowTemp) + " - " + String(idealHighTemp) + ")";
        BlynkStatusWidgetColor = "#228B22"; // green
    } else if ((temperature > minTemp && temperature < idealLowTemp) || (temperature > idealHighTemp && temperature < maxTemp)) {
        BlynkStatusWidgetMessage = "Warning, temperature is not within ideal range";
        BlynkStatusWidgetColor = "#FFC300"; // orange
    } else if (temperature < minTemp || temperature > maxTemp) {
        BlynkStatusWidgetMessage = "WARNING, temperature is reaching dangerous levels";
        BlynkStatusWidgetColor = "#C70039"; // red
    }

    UpdateBlynkWidgetColor(V9, BlynkStatusWidgetColor);
    UpdateBlynkWidgetContent(V9, BlynkStatusWidgetMessage);
}

void CheckHumidityData() {
    if (humidity >= idealLowHumidity && humidity <= idealHighHumidity) {
        BlynkStatusWidgetMessage = "Humidity is within ideal range (" + String(idealLowHumidity) + " - " + String(idealHighHumidity) + ")";
        BlynkStatusWidgetColor = "#228B22";
    } else {
        BlynkStatusWidgetMessage = "WARNING, humidity is reaching dangerous levels"; // Humidity more severe outside ideal compared to temperature.
        BlynkStatusWidgetColor = "#C70039";
    }

    UpdateBlynkWidgetColor(V8, BlynkStatusWidgetColor);
    UpdateBlynkWidgetContent(V8, BlynkStatusWidgetMessage);
}

void openWindow() {
    for (int i = 0; i < 64; i++) {
    clockwise();
  }
}

void closeWindow() {
    for (int j = 0; j < 64; j++) {
    counterClockwise();
  }
}

void clockwise(){
 // 1
 digitalWrite(motorPin4, HIGH);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, LOW);
 delay(motorSpeed);
 // 2
 digitalWrite(motorPin4, HIGH);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, LOW);
 delay (motorSpeed);
 // 3
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, LOW);
 delay(motorSpeed);
 // 4
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin1, LOW);
 delay(motorSpeed);
 // 5
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin1, LOW);
 delay(motorSpeed);
 // 6
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin1, HIGH);
 delay (motorSpeed);
 // 7
 digitalWrite(motorPin4, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, HIGH);
 delay(motorSpeed);
 // 8
 digitalWrite(motorPin4, HIGH);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin1, HIGH);
 delay(motorSpeed);
}


void counterClockwise() {
   // 1
 digitalWrite(motorPin1, HIGH);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, LOW);
 delay(motorSpeed);
 // 2
 digitalWrite(motorPin1, HIGH);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, LOW);
 delay (motorSpeed);
 // 3
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, LOW);
 delay(motorSpeed);
 // 4
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, HIGH);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin4, LOW);
 delay(motorSpeed);
 // 5
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin4, LOW);
 delay(motorSpeed);
 // 6
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, HIGH);
 digitalWrite(motorPin4, HIGH);
 delay (motorSpeed);
 // 7
 digitalWrite(motorPin1, LOW);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, HIGH);
 delay(motorSpeed);
 // 8
 digitalWrite(motorPin1, HIGH);
 digitalWrite(motorPin2, LOW);
 digitalWrite(motorPin3, LOW);
 digitalWrite(motorPin4, HIGH);
 delay(motorSpeed);
}



void UpdateFanSettings() {
    // Show fan rpm in Blynk.
    // Try to get fan to run automatically depending on temperatures
    // Greenhouse - important with ventilation so cold/hot air does not get trapped

    // Also an override in Blynk to manually change temperature. Maybe need a bool for true/false for auto/manual.
}

void ReadTemperature() {
    // dht.temperature().getEvent(&sensorEvent);
    // temperature = sensorEvent.temperature;
    temperature = sht31.readTemperature() - temperatureDifference;

    if (!isnan(temperature)) {
        Blynk.virtualWrite(V1, temperature);
        Serial.print(F("Temperature: "));
        Serial.println(temperature);
    } else {
        Serial.println(F("Error, cannot read temperature "));
    }
}

void ReadHumidity() {
    // dht.humidity().getEvent(&sensorEvent);
    // humidity = sensorEvent.relative_humidity;
    humidity = sht31.readHumidity() - humidityDifference;

    if (!isnan(humidity)) {
        Blynk.virtualWrite(V2, humidity);
        Serial.print(F("Humidity: "));
        Serial.println(humidity);
    } else {
        Serial.println(F("Error, cannot read Humidity"));
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

            Blynk.virtualWrite(V3, lightInformation);
        }
    }
}

void ResetBlynkWidget() {
    String white = "#FFFFFF";

    UpdateBlynkWidgetColor(V0, white);                          // Fruit State widget
    UpdateBlynkWidgetColor(V0, white);                          // Greenhouse status color
    UpdateBlynkWidgetContent(V9, "Should show status message"); // Greenhouse status message
    UpdateBlynkWidgetContent(V8, "Should show status message"); // Greenhouse status message
}

void UpdateBlynkWidgetLabel(char vp, String message) {
    Blynk.setProperty(vp, "label", message);
}

void UpdateBlynkWidgetContent(char vp, String message) {
    Blynk.virtualWrite(vp, message);
}

void UpdateBlynkWidgetColor(char vp, String color) {
    Blynk.setProperty(vp, "color", color);
}
/*


*/
String weather = "";
void ShowTodaysDateAndWeather() {
    String now = printDateTime(Rtc.GetDateTime());

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= waitInterval) {
        ConnectToOpenWeatherMap();
        previousMillis = currentMillis;
    }

    Blynk.virtualWrite(V7, now + ". " + weather);
}

#define countof(arr) (sizeof(arr) / sizeof(arr[0])) // Macro to get number of elements in array
String printDateTime(const RtcDateTime &date) {     // Example code from DS3231_Simple (Rtc by Makuna)
    char dateString[20];

    snprintf_P(dateString,                            // buffer
               countof(dateString),                   // max number of bytes (char), written to buffer
               PSTR("%02u/%02u/%04u %02u:%02u:%02u"), // PSTR reads from flash mem. n (...) is for formating
               date.Month(),                          // rest of params to format
               date.Day(),
               date.Year(),
               date.Hour(),
               date.Minute(),
               date.Second());

    return dateString;
}

JsonObject weather_0;
JsonObject mainInfo;

void ConnectToOpenWeatherMap() {
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
            GetWeatherInfo();
        } else {
            Serial.println("Error on HTTP request");
        }
        http.end();
    }
}

void GetWeatherInfo() {
    const char *weatherDescription = weather_0["description"];
    float mainInfo_temp = mainInfo["temp"];

    weather = String(weatherDescription) + ". " + mainInfo_temp + "C";
}

String ErrorCheckingSensors() {
    String checkSensors = "";

    if (!sht31.begin(0x44)) { // default i2c address
        checkSensors = "SHT31 - Cannot find sensor";
        // Send some error to Blynk
        while (1)
            yield();
    }

    if (!ltr329.begin()) {
        checkSensors += "LTR329 - Cannot find sensor";
        // Send some error to Blynk
        while (1)
            yield();
    }

    return checkSensors;
}

void RtcErrorCheckingAndUpdatingDate() {
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) {
        if (!wasError("setup IsDateTimeValid")) {
            // Common Causes:
            //    1) first time you ran and the device wasn't running yet
            //    2) the battery on the device is low or even missing

            Serial.println("RTC lost confidence in the DateTime!");

            // following line sets the RTC to the date & time this sketch was compiled
            // it will also reset the valid flag internally unless the Rtc device is
            // having an issue

            Rtc.SetDateTime(compiled);
        }
    }

    if (!Rtc.GetIsRunning()) {
        if (!wasError("setup GetIsRunning")) {
            Serial.println("RTC was not actively running, starting now");
            Rtc.SetIsRunning(true);
        }
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (!wasError("setup GetDateTime")) {
        if (now < compiled) {
            Serial.println("RTC is older than compile time, updating DateTime");
            Rtc.SetDateTime(compiled);
        } else if (now > compiled) {
            Serial.println("RTC is newer than compile time, this is expected");
        } else if (now == compiled) {
            Serial.println("RTC is the same as compile time, while not expected all is still fine");
        }
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.Enable32kHzPin(false);
    wasError("setup Enable32kHzPin");
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
    wasError("setup SetSquareWavePin");
}

// Delay timer but with millis. Runs every 1500ms.
void TimerMillis() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= waitInterval) {

        previousMillis = currentMillis;
    }
}
