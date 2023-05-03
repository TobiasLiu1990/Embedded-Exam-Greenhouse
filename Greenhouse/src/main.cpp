/*
  https://stackoverflow.com/questions/46111834/format-curly-braces-on-same-line-in-c-vscode - For changing auto format behaviour
  https://htmlcolorcodes.com/colors/shades-of-green/

  Blynk event limit: 5 (free)
  Blynk notificaton limit: 100 per day
*/

#include "ConnectOpenWeathermap.h"
#include "ESP32_LTR329_Sensor.h"
#include "ESP32_SHT31_Sensor.h"
#include "StepperMotorVent.h"
#include <Arduino.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <RtcDS3231.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>

// Wifi
const char *ssid = "Venti_2.4G";
const char *password = "NikitaBoy";

// Blynk
#define BLYNK_TEMPLATE_ID "TMPL4SP7dMP-c"
#define BLYNK_TEMPLATE_NAME "Greenhouse"
#define BLYNK_AUTH_TOKEN "90qZjiZBUZwmDULPB9tlfsDqq3fXuoZf"
#define BLYNK_PRINT Serial

BlynkTimer timer;               // Each Blynk timer can run up to 16 instances.
char auth[] = BLYNK_AUTH_TOKEN; // Blynk token
char ssidBlynk[] = "Venti_2.4G";
char pass[] = "NikitaBoy";

// Openweathermap
const String endpoint = "https://api.openweathermap.org/data/2.5/weather?q=Oslo,no&APPID=";
const String key = "6759feb4f31aad1b1ace05f93cc6824f";
const String metric = "&units=metric";
ConnectOpenWeathermap openWeathermap(endpoint, key, metric);

RtcDS3231<TwoWire> Rtc(Wire);
ESP32_SHT31_Sensor sht31;
ESP32_LTR329_Sensor ltr;
uint ALS_GAIN[8] = {1, 2, 4, 48, 96};
float ALS_INT[8] = {1.0, 0.5, 2.0, 4.0, 1.5, 2.5, 3.0, 3.5};

//---------------------L293D
// Might add fan for later use if possible
// #define enablePin1 D9
// #define pin1A D6

// Stepper Motor
const int motorPin1 = 9;  // Blue
const int motorPin2 = 10; // Pink
const int motorPin3 = 11; // Yellow
const int motorPin4 = 12; // Orange
int motorSpeed = 7;
int ventOpenAngle[4] = {45, 90, 180, 360};
StepperMotorVent vent(ventOpenAngle[0], motorPin1, motorPin2, motorPin3, motorPin4, motorSpeed);

// Timers
unsigned long previousMillis = 0;
const long waitInterval = 1000;
unsigned long previousMillisSensor = 0;
const long waitIntervalSensor = 5000;

// FSM For fruits
enum Fruits {
    Banana,
    Pineapple
};
Fruits currentState; // Will be set to Bananas by default
int stateNumber;     // 0 will default to Bananas
int oldStateNumber = stateNumber;

// For different warning messages to Blynk.
enum Status {
    Normal,
    Warning,
    Critical
};
Status greenhouseStatus;

bool isWindowOpen = false;
bool isStateChanged = false;
bool runErrorHandlingOnce = true;
bool isConnectedToBlynk = false;

float temperature;
float humidity;
bool temperatureReady;
bool humidityReady;
float minTemp;
float maxTemp;
float idealLowTemp;
float idealHighTemp;
float idealLowHumidity;
float idealHighHumidity;
float upperTemperatureMargin = idealHighTemp - sht31.getUpperTemperatureMargin();
float lowerTemperatureMargin = idealLowTemp + sht31.getLowerTemperatureMargin();

String colorGreen = "#228B22";
String colorGreen2 = "#87DE24";
String colorOrange = "#FFC300";
String colorRed = "#C70039";
String colorWhite = "#FFFFFF";
String colorYellow = "#E6D22A";

// Forward declarations
void sht31StartupCheck();
void rtcErrorCheckingAndUpdatingDate();

void fruitStateTransition();
void updateFruitStateConditions();

void showCurrentDateAndTime();
String printDateTime(const RtcDateTime &date);
void showCurrentWeather();

void checkSensorData();
void checkTemperatureStatus();
void checkHumidityStatus();
void getLux();
void setLtrSettings(ltr329_gain_t gain, ltr329_integrationtime_t integTime, ltr329_measurerate_t measRate, uint als_gain, float als_int);

void uploadTemperatureToBlynk();
void uploadHumidityToBlynk();
void uploadStatusMessageToBlynk(char vp, String widgetMessage, String widgetColor, float idealLow, float idealHigh);

void initBlynk();
void resetBlynkWidget(String color, String message);
void updateBlynkWidgetColor(char vp, String color);
void updateBlynkWidgetContent(char vp, String message);
void updateBlynkWidgetLabel(char vp, String message);
// Forward declarations

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
    Blynk.syncVirtual(V0, V1, V2, V3, V4, V8, V9); // State, Temp, Humidity, Lux, Greenhouse humidity info, Greenhouse temp info.
    isConnectedToBlynk = true;
}

BLYNK_DISCONNECTED() {
    isConnectedToBlynk = false;
}

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

    rtcErrorCheckingAndUpdatingDate(); // Taken from Rtc by Makuna - DS3231_Simple example. Some minor changes to the error checking code.
    sht31StartupCheck();

    WiFi.begin(ssid, password);
    delay(100);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1500);
        Serial.println("Connecting to Wifi...");
    }
    Serial.println("Connected to the Wifi network");

    Blynk.begin(BLYNK_AUTH_TOKEN, ssidBlynk, pass);
    delay(100);

    while (!Blynk.connected()) {
        Serial.println("Connecting hardware to Blynk...");
        delay(1000);
    }
    Serial.println("currentDateAndTime connected to Blynk Greenhouse!");
    initBlynk(); // Init Blynk with Banana as default

    // Blynk .setInterval can not take a function with arguments
    timer.setInterval(1000L, showCurrentDateAndTime);
    timer.setInterval(300000L, showCurrentWeather); // every 5mins
    timer.setInterval(5000L, getLux);
    timer.setInterval(1000L, checkSensorData);
}

void loop() {
    Blynk.run();
    timer.run();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisSensor >= waitIntervalSensor) {
        temperature = sht31.readTemperature();
        humidity = sht31.readHumidity();
        temperatureReady = false;
        humidityReady = false;

        if (sht31.validateNumberReading(temperature)) {
            uploadTemperatureToBlynk();
            temperatureReady = true;
            // Serial.print(F("Temperature: ")); // debugging for now
            // Serial.println(temperature);      // debugging for now
        } else {
            Serial.println(F("Error, cannot read temperature ")); // debugging for now
        }

        if (sht31.validateNumberReading(humidity)) {
            uploadHumidityToBlynk();
            humidityReady = true;
            // Serial.print(F("Humidity: ")); // debugging for now
            // Serial.println(humidity);      // debugging for now
        } else {
            Serial.println(F("Error, cannot read Humidity")); // debugging for now
        }
        previousMillisSensor = currentMillis;
    }

    if (isStateChanged) {
        fruitStateTransition();
        updateFruitStateConditions();
        isStateChanged = false;
    }

    if (isWindowOpen == false) {
        if (temperature > (upperTemperatureMargin)) { // This check is so that the windows should open before the current temp at bench level gets to high.
            isWindowOpen = true;
            vent.openWindow();
            vent.setMotorIdle();
            updateBlynkWidgetColor(V4, "#87DE24");
            updateBlynkWidgetLabel(V4, "Vent: Open");
            Blynk.virtualWrite(V4, 1);
        }
    } else {
        if (temperature < lowerTemperatureMargin) {
            isWindowOpen = false;
            vent.closeWindow();
            vent.setMotorIdle();
            updateBlynkWidgetColor(V4, "#C70039");
            updateBlynkWidgetLabel(V4, "Vent: Closed");
            Blynk.virtualWrite(V4, 0);
        }
    }
}

void initBlynk() {
    // On startup/reset - need to use fresh data.
    // Setting default to Banana.
    String message = "Loading status...";
    resetBlynkWidget(colorWhite, message);
    stateNumber = 0;
    fruitStateTransition();
    updateFruitStateConditions();
}

void fruitStateTransition() {
    switch (stateNumber) {
    case 0:
        currentState = Banana;
        break;
    case 1:
        currentState = Pineapple;
        break;
    }
}

// uint ALS_GAIN[8] = {1, 2, 4, 48, 96};
// float ALS_INT[8] = {1.0, 0.5, 2.0, 4.0, 1.5, 2.5, 3.0, 3.5};

void setLtrSettings(ltr329_gain_t gain, ltr329_integrationtime_t integTime, ltr329_measurerate_t measRate, uint als_gain, float als_int) {
    ltr.ltr329.setGain(gain); // Banana Gain 4, 0.25 - 16k
    ltr.ltr329.setIntegrationTime(integTime);
    ltr.ltr329.setMeasurementRate(measRate);
    ltr.setGainCalc(als_gain);
    ltr.setIntegTimeCalc(als_int);
}

void updateFruitStateConditions() {
    if (currentState == Banana) {
        setLtrSettings(LTR3XX_GAIN_4, LTR3XX_INTEGTIME_400, LTR3XX_MEASRATE_500, ALS_GAIN[0x02], ALS_INT[0x03]);

        updateBlynkWidgetLabel(V0, "Current target: Bananas");
        updateBlynkWidgetColor(V0, colorYellow);
        minTemp = 15;
        maxTemp = 30;
        idealLowTemp = 20.25;
        idealHighTemp = 27.7;
        idealLowHumidity = 50;
        idealHighHumidity = 100;
    } else if (currentState == Pineapple) {
        setLtrSettings(LTR3XX_GAIN_1, LTR3XX_INTEGTIME_400, LTR3XX_MEASRATE_500, ALS_GAIN[0x00], ALS_INT[0x03]);

        updateBlynkWidgetLabel(V0, "Current target: Pineapples");
        updateBlynkWidgetColor(V0, colorGreen2);
        minTemp = 10;
        maxTemp = 35;
        idealLowTemp = 21.5;
        idealHighTemp = 31;
        idealLowHumidity = 40;
        idealHighHumidity = 60;
    }
}

void getLux() {
    unsigned int lux = ltr.getFromLightSensor();

    Serial.print("Lux: ");
    Serial.println(lux);

    if (lux == 0) {
        Serial.println("Could not read lux");
    } else {
        Blynk.virtualWrite(V3, lux);
    }
}

void checkSensorData() {
    if (temperatureReady) {
        checkTemperatureStatus();
    }
    if (humidityReady) {
        checkHumidityStatus();
    }
}

void checkTemperatureStatus() {
    String logEventMessage = "";
    String widgetColor = "";

    if (temperature > idealLowTemp && temperature < idealHighTemp) {
        greenhouseStatus = Normal;
        widgetColor = colorGreen;
    } else {
        if (temperature >= minTemp && temperature <= idealLowTemp) {
            greenhouseStatus = Warning;
            widgetColor = colorOrange;
            logEventMessage = "Warning, Temperature is under ideal lower temperature of:" + String(idealLowTemp);

        } else if (temperature >= idealHighTemp && temperature <= maxTemp) {
            greenhouseStatus = Warning;
            widgetColor = colorOrange;
            logEventMessage = "Warning, Temperature is over ideal upper temperature of:" + String(idealHighTemp);

        } else if (temperature < minTemp) {
            greenhouseStatus = Critical;
            widgetColor = colorRed;
            logEventMessage = "Warning, Temperature is under min. temperature of:" + String(minTemp);

        } else if (temperature > maxTemp) {
            logEventMessage = "Warning, Temperature is over max. temperature of:" + String(maxTemp);
            greenhouseStatus = Critical;
            widgetColor = colorRed;
        }
    }

    if (greenhouseStatus != Normal) {
        Blynk.logEvent("temp_alert", logEventMessage);
    }

    uploadStatusMessageToBlynk(V9, "Temperature", widgetColor, idealLowTemp, idealHighTemp);
}

void checkHumidityStatus() {
    String widgetColor = "";

    if (humidity >= idealLowHumidity && humidity <= idealHighHumidity) {
        greenhouseStatus = Normal;
        widgetColor = colorGreen;
    } else {
        greenhouseStatus = Critical;
        widgetColor = colorRed;
        Blynk.logEvent("temp_alert");
    }

    uploadStatusMessageToBlynk(V8, "Humidity", widgetColor, idealLowHumidity, idealHighHumidity);
}

void uploadStatusMessageToBlynk(char vp, String widgetMessage, String widgetColor, float idealLow, float idealHigh) {
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

    updateBlynkWidgetContent(vp, BlynkStatusWidgetMessage);
    updateBlynkWidgetColor(vp, BlynkStatusWidgetColor);
}

void showCurrentDateAndTime() {
    String currentDateAndTime = printDateTime(Rtc.GetDateTime());
    Blynk.virtualWrite(V30, currentDateAndTime);
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

void showCurrentWeather() {
    openWeathermap.connectToOpenWeatherMap();
    String weatherInfo = openWeathermap.getWeatherInfo();

    Serial.print("Weather info: ");
    Serial.println(weatherInfo);

    Blynk.virtualWrite(V31, weatherInfo);
    openWeathermap.disconnectToOpenWeatherMap();
}

void uploadTemperatureToBlynk() {
    Blynk.virtualWrite(V1, temperature);
}

void uploadHumidityToBlynk() {
    Blynk.virtualWrite(V2, humidity);
}

void updateBlynkWidgetLabel(char vp, String message) {
    Blynk.setProperty(vp, "label", message);
}

void updateBlynkWidgetContent(char vp, String message) {
    Blynk.virtualWrite(vp, message);
}

void updateBlynkWidgetColor(char vp, String color) {
    Blynk.setProperty(vp, "color", color);
}

void resetBlynkWidget(String color, String message) {
    updateBlynkWidgetColor(V0, color);     // V0 - Fruit State widget
    updateBlynkWidgetColor(V8, color);     // V8 - Greenhouse Humidity color
    updateBlynkWidgetContent(V8, message); // V8 - Greenhouse Humidity message
    updateBlynkWidgetColor(V9, color);     // V9 - Greenhouse Temperature color
    updateBlynkWidgetContent(V9, message); // V9 - Greenhouse Temperature message
}

void sht31StartupCheck() {
    if (sht31.checkSensorSht31()) {
        Serial.println("SHT31 - Cannot find sensor");
        updateBlynkWidgetLabel(V1, "SHT31 sensor error");
        updateBlynkWidgetLabel(V2, "SHT31 sensor error");
    };

    if (ltr.checkSensorLtr329()) {
        Serial.println("LTR329 - Cannot find sensor");
        updateBlynkWidgetLabel(V3, "LTR329 sensor error");
    }
}

void rtcErrorCheckingAndUpdatingDate() {
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