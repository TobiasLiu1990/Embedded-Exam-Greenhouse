#include <Arduino.h>

class RunMotor {
private:
    int speed;

public:
    RunMotor() {
        speed = 100;
    }

    int getDefaultFanSpeed();
};