#include <Arduino.h>

class StepperMotorVent {
public:
    int openAngle;
    int motorPin1;
    int motorPin2;
    int motorPin3;
    int motorPin4;
    int motorSpeed;
    int sequences;

    StepperMotorVent(int openAngle, int motorPin1, int motorPin2, int motorPin3, int motorPin4, int motorSpeed) {
        this->openAngle = openAngle;
        this->motorPin1 = motorPin1;
        this->motorPin2 = motorPin2;
        this->motorPin3 = motorPin3;
        this->motorPin4 = motorPin4;
        this->motorSpeed = motorSpeed;

        setSequencesToAngle();
    }

    void openWindow();
    void closeWindow();
    void turnMotorClockwise();
    void turnMotorCounterClockwise();
    void setMotorIdle();
    void setSequencesToAngle();
    void changeSequencesToAngle(int newVentAngle);
};