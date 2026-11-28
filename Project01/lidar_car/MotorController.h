#pragma once  // 클래스 중복 정의 방지
#include <Arduino.h>  // Arduino 기본 기능 사용

// Front Right
#define FR_IN1 26
#define FR_IN2 25

// Front Left
#define FL_IN1 19
#define FL_IN2 27

// Rear Right
#define RR_IN1 23
#define RR_IN2 12

// Rear Left
#define RL_IN1 0
#define RL_IN2 15

class MotorController {
  private:
    int _motorPins[8] = {
    FR_IN1, FR_IN2,
    FL_IN1, FL_IN2,
    RR_IN1, RR_IN2,
    RL_IN1, RL_IN2
    };

  public:
    // 초기 세팅
    void begin() {
      for (int i = 0; i < 8; i++) {
        pinMode(_motorPins[i], OUTPUT);
        digitalWrite(_motorPins[i], LOW);
      }
    }

    // 전진
    void forward() {
      for (int i = 0; i < 8; i += 2) {
        digitalWrite(_motorPins[i], HIGH);
        digitalWrite(_motorPins[i + 1], LOW);
      }
    }

    // 후진
    void backward() {
      for (int i = 0; i < 8; i += 2) {
        digitalWrite(_motorPins[i], LOW);
        digitalWrite(_motorPins[i + 1], HIGH);
      }
    }
    
    // 정지
    void stop() {
      for (int i = 0; i < 8; i += 2) {
        digitalWrite(_motorPins[i], HIGH);
        digitalWrite(_motorPins[i + 1], HIGH);
      }
    }
};