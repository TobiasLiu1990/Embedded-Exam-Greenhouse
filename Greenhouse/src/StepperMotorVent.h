#include <Arduino.h>

class StepperMotorVent {
private:
    int motorPin1;
    int motorPin2;
    int motorPin3;
    int motorPin4;
    int motorSpeed;
    int sequences;

public:
    StepperMotorVent(int motorPin1, int motorPin2, int motorPin3, int motorPin4, int motorSpeed);

    void openWindow();
    void closeWindow();
    void turnMotorClockwise();
    void turnMotorCounterClockwise();
    void setMotorIdle();
    void setSequencesToAngle(int openAngle);
    void changeSequencesToAngle(int newVentAngle);
    bool checkSequences();
};