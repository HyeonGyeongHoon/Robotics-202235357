#pragma once  // 클래스 중복 정의 방지
#include <Arduino.h>  // Arduino 기본 기능 사용

/*
 * Byte 0       : Header (0x54)
 * Byte 1       : VerLen (0x2C)
 * Byte 2~3     : 회전 속도
 * Byte 4~5     : 시작 각도
 * Byte 6~41    : 거리 측정값 12개
 *                각 측정점 = 3 byte
 *                distance 2 byte + confidence 1 byte
 * Byte 42~43   : 끝 각도
 * Byte 44~45   : timestamp
 * Byte 46      : CRC
*/

#define LIDAR_RX 16

class Lidar {
  private:
    uint8_t _packet[47];

    uint16_t readUInt16(uint8_t low, uint8_t high) {
      return low | (high << 8);
    }
    
    bool FindHeader() {
      if (!Serial2.available()) {
        return false;
      }

      // packet의 첫 byte는 항상 0x54이다.
      if (Serial2.read() != 0x54) {
        return false;
      }

      // 두번째 byte가 들어올 때까지 기다린다.
      while(!Serial2.available()) {
      }

      // 두번째 byte가 0x2C인지 확인
      if (Serial2.read() != 0x2C) {
        return false;
      }

      return true;
    }

    void updatePacket() {
      _packet[0] = 0x54;
      _packet[1] = 0x2C;

      // header 2바이트를 제외한 나머지 packet 데이터를 읽는다.
      for (int i = 2; i < 47; i++) {
        while (!Serial2.available()) {
        }

        _packet[i] = Serial2.read();
      }
    }

  public:
    uint16_t distances[360] = {0};
    
    // Lidar() {
    //   Serial2.begin(230400, SERIAL_8N1, LIDAR_RX, -1);
    // }
    // Lidar(int baudrate, int rx) {
    //   Serial2.begin(baudrate, SERIAL_8N1, rx, -1); //LiDAR -> RX 시리얼 통신 (TX 사용 안함)
    // }

    // Serial2.begin을 생성자에서 말고 따로 Lidar.begin 함수 구현
    void begin(int baudrate = 230400, int rx = LIDAR_RX) {
        Serial2.begin(baudrate, SERIAL_8N1, rx, -1);
    }

    void updateDistance() {
      if(FindHeader()) {
        updatePacket();

        // 시작 각도와 끝 각도는 각각 2바이트
        uint16_t startRaw = readUInt16(_packet[4], _packet[5]);
        uint16_t endRaw = readUInt16(_packet[42], _packet[43]);

        // 각도 값은 0.01도 단위로 들어오므로 100으로 나누어준다.
        float startAngle = startRaw / 100.0;
        float endAngle = endRaw / 100.0;
        
        // endAngle이 startAngle 보다 작으면 한바퀴 돈 것이므로 360을 더해준다.
        if (endAngle < startAngle) {
          endAngle += 360.0;
        }

        // 한 packet에는 12개의 측정점이 잇으므로 시작점과 끝점 사이를 11구간으로 나눈다.
        float angleStep = (endAngle - startAngle) / 11.0;

        for (int i = 0; i < 12; i++) {
          // 각 측정점은 distance 2바이트 + confidence 1바이트로 구성하기 때문에 6부터 3칸 간격으로 index 지정한다.
          int index = 6 + i * 3;

          // 거리값 2바이트를 mm 단위 정수로 변환.
          uint16_t distance = readUInt16(_packet[index], _packet[index+1]);

          //현재 측정점이 어느 각도에 해당하는 지 계산
          float angle = startAngle + angleStep * i;

          // 각도가 360도를 넘으면 360를 뺀다
          if (angle >= 360.0) {
            angle -= 360.0;
          }

          // float 각도를 가장 가까운 정수 각도로 반올림
          int degree = (int)(angle + 0.5);

          if (degree >= 360) {
            degree = 0;  // 360 -> 0
          }


          // 0은 유효하지 않은 거리값으로 보고 저장하지 않는다.
          if (distance > 0) {
            distances[degree] = distance;
          }
        }
      }
      return;
    }
};