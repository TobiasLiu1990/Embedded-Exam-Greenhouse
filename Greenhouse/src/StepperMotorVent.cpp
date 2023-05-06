#include "StepperMotorVent.h"
#include <Arduino.h>

/*Half-step setup
  360deg - 1 revolution      =  512 sequences
  180deg - 0.5 revolution    =  256 (compensated for rotational slop) -> 257
  90deg  - 0.25 revolution   =  128 (test if compensation needed)
  45deg  - 0.125 revolution  =  64
*/

StepperMotorVent::StepperMotorVent(int motorPin1, int motorPin2, int motorPin3, int motorPin4, int motorSpeed) {
    this->motorPin1 = motorPin1;
    this->motorPin2 = motorPin2;
    this->motorPin3 = motorPin3;
    this->motorPin4 = motorPin4;
    this->motorSpeed = motorSpeed;
    this->sequences = 0;
}

void StepperMotorVent::setSequencesToAngle(int openAngle) {
    switch (openAngle) {
    case 45:
        sequences = 64;
        break;
    case 90:
        sequences = 128;
        break;
    case 180:
        sequences = 256;
        break;
    case 360:
        sequences = 360;
        break;
    }
}

bool StepperMotorVent::checkSequences() {
    if (sequences >= 0) {
        return true; 
    } else {
        return false;
    }
}

void StepperMotorVent::openWindow() {
    for (int i = 0; i < sequences; i++) {
        turnMotorClockwise();
    }
}

void StepperMotorVent::closeWindow() {
    for (int j = 0; j < sequences; j++) {
        turnMotorCounterClockwise();
    }
}

void StepperMotorVent::setMotorIdle() {
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin4, LOW);
}

void StepperMotorVent::turnMotorClockwise() {
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

void StepperMotorVent::turnMotorCounterClockwise() {
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