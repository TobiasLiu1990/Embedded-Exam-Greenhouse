#include "RunMotor.h"
#include <Arduino.h>

/*
    https://uknowledge.uky.edu/cgi/viewcontent.cgi?article=1000&context=aeu_reports#:~:text=For%20greenhouses%2C%20one%20complete%20exchange,by%208%20(average%20height).

    Mechanical ventilation
    Optimal ventilation with fans are counted as air change per minute (cfm) (complete air change in greenhouse per min).

    Maximum ventilation - max airflow to limit temperature rise.
    Formula: floor area * 8

    Example: 45m * 10m = 450m.
    450 * 8 = 3600 cfm
*/

// According to article (although for tobacco), lets assume greenhouse area to be ~277m2 (converted 30x100 feet to meters)
// This would need 1 x 36" fan.
// Lets assume the fan used in this project is of that size.
// And assume that the fan only needs to run at 100 rpm to keep temperature stable

//Method is quite empty, but the idea is that multiple fans might be needed in reality.
int RunMotor::setDefaultFanSpeed() {
    return 0;
}