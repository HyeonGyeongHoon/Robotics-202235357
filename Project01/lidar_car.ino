/*
 * Back  :    0°
 * Left  :   90°
 * Front :  180°
 * Right :  270°
*/

#include "Lidar.h"
#include "MotorController.h"

unsigned long lastDecision = 0;
const unsigned long DECISION_INTERVAL = 100;   // ms

Lidar lidar;
MotorController motorController;

void setup() {
    Serial.begin(115200);
    lidar.begin();
    motorController.begin();
}

void loop() {
    lidar.updateDistance();  // 거리값 갱신

    // 라이다센서는 1초에 10바퀴 돌기 때문에 1바퀴 도는 시간 100ms 경과 후 판단
    if (millis() - lastDecision >= DECISION_INTERVAL) {
        lastDecision = millis();

        uint16_t frontDistance = lidar.distances[180];

        // 10cm 거리의 물체 발견 시 정지
        if(frontDistance > 0 && frontDistance < 300) {
            motorController.stop();
            Serial.println("장애물 발견!");
        }
        else {
            motorController.forward();
        }

        Serial.println(frontDistance);  // 거리값 테스트
    }
}