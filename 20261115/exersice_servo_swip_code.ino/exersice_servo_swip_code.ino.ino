#include <ESP32Servo.h>

#define SERVO_PIN 14

Servo myServo;

int degree = 0;
int weight = 1;

void setup() {
    myServo.attach(SERVO_PIN);
    myServo.write(degree);
}

void loop() {
  for(degree; degree<=180; degree+=weight){
    myServo.write(degree);
    delay(5);
  }
  for(degree; degree>=0; degree-=weight){
    myServo.write(degree);
    delay(5);
  }
}
