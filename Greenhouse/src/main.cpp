/*
  Uses Openweathermap.org for weather information. 60 calls per h (free version).
  https://stackoverflow.com/questions/46111834/format-curly-braces-on-same-line-in-c-vscode - For changing auto format behaviour

    Other components:
    Fan (act as vent) - ventilation on/off depending on temp

    Date/time with RTC when:
      Update greenhouse information every 10mins.
      Openweathermap updates
      If something goes wrong

    Ref:
    Light sensor library: https://adafruit.github.io/Adafruit_LTR329_LTR303/html/class_adafruit___l_t_r329.html

*/

#include <Adafruit_DotStar.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <RtcDS3231.h>
#include <Wire.h>
#include <i2cdetect.h>

#include "SensorInfo.h"

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
char ssidBlynk[] = "Venti_2.4G"; // Wifi
char pass[] = "NikitaBoy";       // Wifi pw

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

/*Half-step setup
  360deg - 1 revolution      =  512 sequences
  180deg - 0.5 revolution    =  256 (compensated for rotational slop) -> 257
  90deg  - 0.25 revolution   =  128 (test if compensation needed)
  45deg  - 0.125 revolution  =  64
*/
// Stepper pins
const int motorPin1 = 9;  // Blue
const int motorPin2 = 10; // Pink
const int motorPin3 = 11; // Yellow
const int motorPin4 = 12; // Orange

int motorSpeed = 5;
bool isWindowOpen = false;
const float tempDiff = 1.9 - 0.7;

//--------Wifi
bool isConnected = false;

// Timers
unsigned long previousMillis = 0;
const long waitInterval = 1000;

unsigned long previousMillisWeather = 0;
const long waitIntervalWeather = 120000; // 2min per weather update

unsigned long previousMillisSensor = 0;
const long waitIntervalSensor = 5000;

// Using this for FSM
enum Fruits {
    Banana,
    Pineapple
};

Fruits currentState; // Will be set to Bananas by default
int stateNumber;     // 0 will default to Bananas
int oldStateNumber = stateNumber;

bool isStateChanged = false;
bool runErrorHandlingOnce = true;
bool isConnectedToBlynk = false;

String todaysDateAndWeather = "";

const float temperatureDifference = 33.88 - 24.40; // ESP32 sensor - DHT11 sensor (doen in room temperature, each ran for ~15min)
const float humidityDifference = 15.89 - 11;       // ESP32 sensor - DHT11 sensor (doen in room temperature, each ran for ~15min)
float temperature = 0;
float humidity = 0;
float minTemp;
float maxTemp;
float idealLowTemp;
float idealHighTemp;
float idealLowHumidity;
float idealHighHumidity;

// Forward declarations
//
void TimerMillis();

void ConnectToOpenWeatherMap();
void ShowCurrentDateAndTime();
void ShowCurrentWeather();
String GetWeatherInfo();

String ErrorCheckingSensors();
bool ReadTemperature();
void UploadTemperatureToBlynk();
bool ReadHumidity();
void UploadHumidityToBlynk();
void UploadStatusMessageToBlynk(char vp, String widgetMessage, String widgetColor, float idealLow, float idealHigh);

void SetLightSensor();
void GetLightSensorInfo();
void CheckSensorData();
void CheckTemperatureStatus();
void CheckHumidityStatus();

void FruitStateTransition();
void UpdateFruitStateConditions();

void initBlynk();
void ResetBlynkWidget(String color, String message);
void UpdateBlynkWidgetColor(char vp, String color);
void UpdateBlynkWidgetContent(char vp, String message);
void UpdateBlynkWidgetLabel(char vp, String message);

String printDateTime(const RtcDateTime &date);
void RtcErrorCheckingAndUpdatingDate(); // Might need to fix a bit later. Currently copied from Rtc by Makuna example.

void OpenWindow();
void CloseWindow();
void TurnMotorClockwise();
void TurnMotorCounterClockwise();
void SetMotorIdle();
//
// Forward declarations

// Blynk
//
// Checks Widget for state change on fruits.
BLYNK_WRITE(V0) {
    stateNumber = param.asInt();

    if (oldStateNumber != stateNumber) {
        isStateChanged = true;
    }
    oldStateNumber = stateNumber;
}

// Checks state for window if open/closed.
BLYNK_WRITE(V4) {
    int windowState = param.asInt();

    if (windowState == 0) {
        isWindowOpen = false;
    } else if (windowState == 1) {
        isWindowOpen = true;
    }
}

BLYNK_CONNECTED() {
    // Change Web Link Button message to "Congratulations!"
    // Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
    // Blynk.setProperty(V3, "onImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
    // Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");

    Blynk.syncVirtual(V0, V1, V2, V3, V4, V8, V9); // State, Temp, Humidity, Lux, Greenhouse humidity info, Greenhouse temp info.
    isConnectedToBlynk = true;
}

BLYNK_DISCONNECTED() {
    isConnectedToBlynk = false;
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
    bool isSensorStarted = false;

    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);
    pinMode(motorPin3, OUTPUT);
    pinMode(motorPin4, OUTPUT);

    // Taken from Rtc by Makuna - DS3231_Simple example. Some minor changes to the error checking code.
    RtcErrorCheckingAndUpdatingDate();
    ErrorCheckingSensors();

    WiFi.begin(ssid, password);
    delay(100);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1500);
        Serial.println("Connecting to Wifi...");
        isConnected = false;
    }
    Serial.println("Connected to the Wifi network");
    isConnected = true;

    Blynk.begin(BLYNK_AUTH_TOKEN, ssidBlynk, pass);
    delay(100);

    while (!Blynk.connected()) {
        Serial.println("Connecting hardware to Blynk...");
    }
    initBlynk(); // Init Blynk with Banana as default
    Serial.println("currentDateAndTime connected to Blynk Greenhouse!");

    // Blynk .setInterval can not take a function with arguments

    // These should only read sensor data. NOT UPLOAD TO BLYNK HERE...
    timer.setInterval(1000L, ShowCurrentDateAndTime);
    timer.setInterval(120000L, ShowCurrentWeather);
    timer.setInterval(5000L, SetLightSensor);
    timer.setInterval(1000L, CheckSensorData);
}


SensorInfo sensorInfo;

bool temperatureReady = false;
bool humidityReady = false;
void loop() {
    Blynk.run();
    timer.run();


    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisSensor >= waitIntervalSensor) {
        
        if (ReadTemperature()) {
            UploadTemperatureToBlynk();
            temperatureReady = true;
        }
        if (ReadHumidity()) {
            UploadHumidityToBlynk();
            humidityReady = true;
        }
        previousMillisSensor = currentMillis;
    }

    if (isStateChanged) {
        FruitStateTransition();
        UpdateFruitStateConditions();
        isStateChanged = false;
    }

    if (isWindowOpen == false) {
        if (temperature > (idealHighTemp - tempDiff)) { // This check is so that the windows should open before the current temp at bench level gets to high.
            isWindowOpen = true;
            OpenWindow();
            SetMotorIdle();
            UpdateBlynkWidgetColor(V4, "#87DE24");
            Blynk.virtualWrite(V4, 1);
        }
    } else {
        if (temperature < idealLowTemp + 3) {
            isWindowOpen = false;
            CloseWindow();
            SetMotorIdle();
            UpdateBlynkWidgetColor(V4, "#C70039");
            Blynk.virtualWrite(V4, 0);
        }
    }
}

void initBlynk() {
    currentState = Banana; // Will be set to Bananas by default in Setup()
    stateNumber = 0;       // 0 will default to Bananas

    String color = "#FFFFFF";
    String message = "Loading status...";
    ResetBlynkWidget(color, message);
    UpdateFruitStateConditions();
}

void UpdateFruitStateConditions() {
    if (currentState == Banana) {
        UpdateBlynkWidgetLabel(V0, "Current target: Bananas");
        UpdateBlynkWidgetColor(V0, "#E6D22A"); // Yellow

        Serial.println("Changing to Banana variables");
        minTemp = 15;
        maxTemp = 30;
        idealLowTemp = 20.25;
        idealHighTemp = 27.7;
        idealLowHumidity = 50;
        idealHighHumidity = 100;
    } else if (currentState == Pineapple) {
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
        Serial.println("Banana state currentDateAndTime");
        break;
    case 1:
        currentState = Pineapple;
        Serial.println("pineapple state currentDateAndTime");
        break;
    }
}

bool onStartUpDelay = true;
void CheckSensorData() {
    if (temperatureReady) {
        CheckTemperatureStatus();
    }
    if (humidityReady) {
        CheckHumidityStatus();
    }
}

enum Status {
    Normal,
    Warning,
    Critical
};
Status greenhouseStatus;
// https://htmlcolorcodes.com/colors/shades-of-green/
String colorGreen = "#228B22";
String colorOrange = "#FFC300";
String colorRed = "#C70039";

void CheckTemperatureStatus() {
    String widgetColor = "";

    if (temperature >= idealLowTemp && temperature <= idealHighTemp) {
        greenhouseStatus = Normal;
        widgetColor = colorGreen;
    } else if ((temperature > minTemp && temperature < idealLowTemp) || (temperature > idealHighTemp && temperature < maxTemp)) {
        greenhouseStatus = Warning;
        widgetColor = colorOrange;
    } else if (temperature < minTemp || temperature > maxTemp) {
        greenhouseStatus = Critical;
        widgetColor = colorRed;
    }

    UploadStatusMessageToBlynk(V9, "Temperature", widgetColor, idealLowTemp, idealHighTemp);
}

void CheckHumidityStatus() {
    String widgetColor = "";

    if (humidity >= idealLowHumidity && humidity <= idealHighHumidity) {
        greenhouseStatus = Normal;
        widgetColor = colorGreen;
    } else {
        greenhouseStatus = Critical;
        widgetColor = colorRed;
    }

    UploadStatusMessageToBlynk(V8, "Humidity", widgetColor, idealLowHumidity, idealHighHumidity);
}

void UploadStatusMessageToBlynk(char vp, String widgetMessage, String widgetColor, float idealLow, float idealHigh) {
    String BlynkStatusWidgetMessage = "";
    String BlynkStatusWidgetColor = "";

    switch (greenhouseStatus) {
    case Normal:
        BlynkStatusWidgetMessage = widgetMessage + " is within ideal range (" + String(idealLow) + " - " + String(idealHigh) + ")";
        BlynkStatusWidgetColor = widgetColor;
        break;
    case Warning:
        BlynkStatusWidgetMessage = "Warning, " + widgetMessage + " is not within ideal range.";
        BlynkStatusWidgetColor = widgetColor;
        break;
    case Critical:
        BlynkStatusWidgetMessage = "Critical Warning! " + widgetMessage + " is reaching dangerous levels";
        BlynkStatusWidgetColor = widgetColor;
        break;
    }

    UpdateBlynkWidgetContent(vp, BlynkStatusWidgetMessage);
    UpdateBlynkWidgetColor(vp, BlynkStatusWidgetColor);
}

bool ReadTemperature() {
    temperature = sht31.readTemperature() - temperatureDifference;

    if (!isnan(temperature)) {
        Serial.print(F("Temperature: "));
        Serial.println(temperature);
        return true;
    } else {
        Serial.println(F("Error, cannot read temperature "));
        return false;
    }
}

void UploadTemperatureToBlynk() {
    Blynk.virtualWrite(V1, temperature);
}

bool ReadHumidity() {
    humidity = sht31.readHumidity() - humidityDifference;

    if (!isnan(humidity)) {
        Serial.print(F("Humidity: "));
        Serial.println(humidity);
        return true;
    } else {
        Serial.println(F("Error, cannot read Humidity"));
        return false;
    }
}

void UploadHumidityToBlynk() {
    Blynk.virtualWrite(V2, humidity);
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

void OpenWindow() {
    for (int i = 0; i < 64; i++) {
        TurnMotorClockwise();
    }
}

void CloseWindow() {
    for (int j = 0; j < 64; j++) {
        TurnMotorCounterClockwise();
    }
}

void SetMotorIdle() {
    digitalWrite(motorPin4, LOW);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin2, LOW);
    digitalWrite(motorPin1, LOW);
}

void TurnMotorClockwise() {
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
    delay(motorSpeed);
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
    delay(motorSpeed);
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

void TurnMotorCounterClockwise() {
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
    delay(motorSpeed);
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
    delay(motorSpeed);
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

void ResetBlynkWidget(String color, String message) {

    UpdateBlynkWidgetColor(V0, color);     // V0 - Fruit State widget
    UpdateBlynkWidgetColor(V8, color);     // V8 - Greenhouse Humidity color
    UpdateBlynkWidgetContent(V8, message); // V8 - Greenhouse Humidity message
    UpdateBlynkWidgetColor(V9, color);     // V9 - Greenhouse Temperature color
    UpdateBlynkWidgetContent(V9, message); // V9 - Greenhouse Temperature message
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
    WANT
    TO
    FIX
    WEATHER
    METHODS
    LATER
    FIND BETTER WAY TO DO THIS
*/

void ShowCurrentDateAndTime() {
    String currentDateAndTime = printDateTime(Rtc.GetDateTime());
    Blynk.virtualWrite(V30, currentDateAndTime);
}

void ShowCurrentWeather() {
    ConnectToOpenWeatherMap();
    Blynk.virtualWrite(V31, GetWeatherInfo());

    /*
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisWeather >= waitIntervalWeather) {
        ConnectToOpenWeatherMap();
        weather = GetWeatherInfo();

        previousMillisWeather = currentMillis;
    }
    */
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
        } else {
            Serial.print("Error on HTTP request: ");
            Serial.println(httpCode);
        }
        http.end();
    }
}

String GetWeatherInfo() {
    const char *weatherDescription = weather_0["description"];
    float mainInfo_temp = mainInfo["temp"];

    return String(weatherDescription) + ". " + mainInfo_temp + "C";
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

String ErrorCheckingSensors() {
    String checkSensors = "";

    if (!sht31.begin(0x44)) { // default i2c address
        checkSensors = "SHT31 - Cannot find sensor";
        UpdateBlynkWidgetLabel(V1, "Temperature sensor error");
        while (1)
            yield();
    }
    if (!ltr329.begin()) {
        checkSensors += "LTR329 - Cannot find sensor";
        UpdateBlynkWidgetLabel(V2, "Humidity sensor error");
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
            Serial.println("RTC was not actively running, starting currentDateAndTime");
            Rtc.SetIsRunning(true);
        }
    }

    RtcDateTime currentDateAndTime = Rtc.GetDateTime();
    if (!wasError("setup GetDateTime")) {
        if (currentDateAndTime < compiled) {
            Serial.println("RTC is older than compile time, updating DateTime");
            Rtc.SetDateTime(compiled);
        } else if (currentDateAndTime > compiled) {
            Serial.println("RTC is newer than compile time, this is expected");
        } else if (currentDateAndTime == compiled) {
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
